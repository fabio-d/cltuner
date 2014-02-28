#include "clhelpers/cl.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

#include <unistd.h>

static bool isdigit_string(const char *str)
{
	if (*str == '\0')
		return false; // Non accetta stringhe vuote

	while (*str && isdigit(*str)) str++;

	return *str == '\0'; // Accetta se siamo arrivati alla fine, ovvero tutti i caratteri erano numeri
}

int main(int argc, char **argv)
{
	const size_t groupSize[] = { 16, 16 };
	size_t fullGlobalSize[2];

	int platform_index = 0;
	int device_index = 0;

	// Parsing degli eventuali ultimi due argomenti numerici, da destra verso
	// sinistra
	if (argc >= 3 && isdigit_string(argv[argc - 1]))
		platform_index = atoi(argv[--argc]);
	if (argc >= 3 && isdigit_string(argv[argc - 1]))
	{
		device_index = platform_index;
		platform_index = atoi(argv[--argc]);
	}

	if (argc != 2 || (fullGlobalSize[0] = fullGlobalSize[1] = atoi(argv[1])) <= 0
		|| (fullGlobalSize[0] % groupSize[0]) != 0)
	{
		fprintf(stderr, "Uso: %s <lato> [cl-platform-num [cl-device-num]]\n", argv[0]);
		fprintf(stderr, "Specificare lato della griglia (deve essere multiplo di %d)\n", groupSize[0]);
		fprintf(stderr, "e gli indici della piattaforma e del dispositivo OpenCL da utilizzare\n");
		return EXIT_FAILURE;
	}

	cl_platform_id platform = clhSelectPlatform(platform_index);
	fprintf(stderr, "CL platform: %s\n", clhGetPlatformFriendlyName(platform).c_str());

	cl_device_id device = clhSelectDevice(platform, device_index);
	fprintf(stderr, "CL device: %s\n", clhGetDeviceFriendlyName(device).c_str());

	cl_context context = clhCreateContextSingleDevice(platform, device);
	cl_command_queue command_queue = clhCreateCommandQueue(context, device, true /* profiling abilitato */);

	clhEmptyNvidiaCache();

	cl_program program = clhBuildProgram(context, device, "tools/triangle_grid.cl");
	cl_kernel k_schema_naive = clhCreateKernel(program, "schema_naive");
	cl_kernel k_schema_triangolo_tutticalcolano = clhCreateKernel(program, "schema_triangolo_tutticalcolano");
	cl_kernel k_schema_triangolo_solounocalcola = clhCreateKernel(program, "schema_triangolo_solounocalcola");

	cl_int err;
	cl_mem v_dummyBuffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(cl_int), NULL, &err);
	CL_CHECK_ERR("clCreateBuffer", err);

	cl_event naive_evt, triangolo_tutti_evt, triangolo_solo_evt;

	fprintf(stderr, "Workgroup size: %dx%d\n", groupSize[0], groupSize[1]);

	// Launch k_schema_naive
	fprintf(stderr, "Launching k_schema_naive (grid %dx%d)...\n",
		fullGlobalSize[0] / groupSize[0], fullGlobalSize[1] / groupSize[1]);
	CL_CHECK_ERR("clSetKernelArg", clSetKernelArg(k_schema_naive, 0, sizeof(cl_mem), &v_dummyBuffer));
	CL_CHECK_ERR("clEnqueueNDRangeKernel", clEnqueueNDRangeKernel(command_queue,
		k_schema_naive,
		2,
		NULL,
		fullGlobalSize,
		groupSize,
		0,
		NULL,
		&naive_evt
	));

	// Launch k_schema_triangolo_tutticalcolano e k_schema_triangolo_solounocalcola
	const int n_blocchi = fullGlobalSize[0] / groupSize[0];
	const size_t triangleGlobalSize[] = { n_blocchi * (n_blocchi + 1) / 2 * groupSize[0] , groupSize[1] };

	fprintf(stderr, "Launching k_schema_triangolo_tutticalcolano (grid %dx%d)...\n",
		triangleGlobalSize[0] / groupSize[0], triangleGlobalSize[1] / groupSize[1]);
	CL_CHECK_ERR("clSetKernelArg", clSetKernelArg(k_schema_triangolo_tutticalcolano, 0, sizeof(cl_mem), &v_dummyBuffer));
	CL_CHECK_ERR("clEnqueueNDRangeKernel", clEnqueueNDRangeKernel(command_queue,
		k_schema_triangolo_tutticalcolano,
		2,
		NULL,
		fullGlobalSize,
		groupSize,
		1,
		&naive_evt,
		&triangolo_tutti_evt
	));

	fprintf(stderr, "Launching k_schema_triangolo_solounocalcola (grid %dx%d)...\n",
		triangleGlobalSize[0] / groupSize[0], triangleGlobalSize[1] / groupSize[1]);
	CL_CHECK_ERR("clSetKernelArg", clSetKernelArg(k_schema_triangolo_solounocalcola, 0, sizeof(cl_mem), &v_dummyBuffer));
	CL_CHECK_ERR("clEnqueueNDRangeKernel", clEnqueueNDRangeKernel(command_queue,
		k_schema_triangolo_solounocalcola,
		2,
		NULL,
		fullGlobalSize,
		groupSize,
		1,
		&triangolo_tutti_evt,
		&triangolo_solo_evt
	));

	fprintf(stderr, "k_schema_naive: %g ms\n", clhEventWaitAndGetDuration(naive_evt));
	fprintf(stderr, "k_schema_triangolo_tutticalcolano: %g ms\n", clhEventWaitAndGetDuration(triangolo_tutti_evt));
	fprintf(stderr, "k_schema_triangolo_solounocalcola: %g ms\n", clhEventWaitAndGetDuration(triangolo_solo_evt));

	CL_CHECK_ERR("clReleaseEvent", clReleaseEvent(naive_evt));
	CL_CHECK_ERR("clReleaseEvent", clReleaseEvent(triangolo_tutti_evt));
	CL_CHECK_ERR("clReleaseEvent", clReleaseEvent(triangolo_solo_evt));

	CL_CHECK_ERR("clReleaseMemObject", clReleaseMemObject(v_dummyBuffer));

	CL_CHECK_ERR("clReleaseKernel", clReleaseKernel(k_schema_naive));
	CL_CHECK_ERR("clReleaseKernel", clReleaseKernel(k_schema_triangolo_tutticalcolano));
	CL_CHECK_ERR("clReleaseKernel", clReleaseKernel(k_schema_triangolo_solounocalcola));
	CL_CHECK_ERR("clReleaseProgram", clReleaseProgram(program));

	CL_CHECK_ERR("clReleaseCommandQueue", clReleaseCommandQueue(command_queue));
	CL_CHECK_ERR("clReleaseContext", clReleaseContext(context));
	return EXIT_SUCCESS;
}
