#include "clhelpers/cl.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

#include <unistd.h>

#define MAX_MEGABYTES 256
#define MAX_ITERAZIONI 16

static bool isdigit_string(const char *str)
{
	if (*str == '\0')
		return false; // Non accetta stringhe vuote

	while (*str && isdigit(*str)) str++;

	return *str == '\0'; // Accetta se siamo arrivati alla fine, ovvero tutti i caratteri erano numeri
}

int main(int argc, char **argv)
{
	int megabytes;
	int num_iterazioni;

	int platform_index = 0;
	int device_index = 0;

	// Parsing degli eventuali ultimi due argomenti numerici, da destra verso
	// sinistra
	if (argc >= 4 && isdigit_string(argv[argc - 1]))
		platform_index = atoi(argv[--argc]);
	if (argc >= 4 && isdigit_string(argv[argc - 1]))
	{
		device_index = platform_index;
		platform_index = atoi(argv[--argc]);
	}

	if (argc != 3 || (megabytes = atoi(argv[1])) <= 0 || (num_iterazioni = atoi(argv[2])) <= 0)
	{
		fprintf(stderr, "Uso: %s <MiB> <num-iterazioni> [cl-platform-num [cl-device-num]]\n", argv[0]);
		fprintf(stderr, "Specificare la quantita' di RAM video da utilizzare per i test,\n");
		fprintf(stderr, "il numero di iterazioni da eseguire e gli indici della piattaforma\n");
		fprintf(stderr, "e del dispositivo OpenCL da utilizzare\n");
		return EXIT_FAILURE;
	}

	if (megabytes > MAX_MEGABYTES)
	{
		fprintf(stderr, "Troppi MiB richiesti!\n");
		fprintf(stderr, "Per evitare di rendere instabile il sistema, questo programma\n");
		fprintf(stderr, "impedisce di allocare piu' di %d MiB di RAM video\n", MAX_MEGABYTES);
		return EXIT_FAILURE;
	}

	if (num_iterazioni > MAX_ITERAZIONI)
	{
		fprintf(stderr, "Troppe iterazioni richieste (max %d)!\n", MAX_ITERAZIONI);
		return EXIT_FAILURE;
	}

	cl_platform_id platform = clhSelectPlatform(platform_index);
	fprintf(stderr, "CL platform: %s\n", clhGetPlatformFriendlyName(platform).c_str());

	cl_device_id device = clhSelectDevice(platform, device_index);
	fprintf(stderr, "CL device: %s\n", clhGetDeviceFriendlyName(device).c_str());

	cl_context context = clhCreateContextSingleDevice(platform, device);
	cl_command_queue command_queue = clhCreateCommandQueue(context, device, true /* profiling abilitato */);

	clhEmptyNvidiaCache();

	cl_program program = clhBuildProgram(context, device, "tools/cl_speed.cl");
	cl_kernel k_mem_copy = clhCreateKernel(program, "mem_copy");

	const int bytes_half = megabytes * 1024 * 1024 / 2;

	cl_int err;
	cl_mem v_buffer1 = clCreateBuffer(context, CL_MEM_READ_ONLY, bytes_half, NULL, &err);
	CL_CHECK_ERR("clCreateBuffer", err);
	cl_mem v_buffer2 = clCreateBuffer(context, CL_MEM_WRITE_ONLY, bytes_half, NULL, &err);
	CL_CHECK_ERR("clCreateBuffer", err);

	size_t groupSize = 256;
	size_t globalSize = bytes_half / sizeof(cl_int);
	CL_CHECK_ERR("clSetKernelArg", clSetKernelArg(k_mem_copy, 0, sizeof(cl_mem), &v_buffer2));
	CL_CHECK_ERR("clSetKernelArg", clSetKernelArg(k_mem_copy, 1, sizeof(cl_mem), &v_buffer1));

	float secs = 0;
	for (int i = 0; i < num_iterazioni; i++)
	{
		cl_event kernel_evt;
		CL_CHECK_ERR("clEnqueueNDRangeKernel", clEnqueueNDRangeKernel(command_queue,
			k_mem_copy,
			1,
			NULL,
			&globalSize,
			&groupSize,
			0,
			NULL,
			&kernel_evt
		));

		secs += clhEventWaitAndGetDuration(kernel_evt);
		clReleaseEvent(kernel_evt);
	}

	clFinish(command_queue);

	printf("Bandwidth: %g MiB/s\n", num_iterazioni * megabytes / secs);

	CL_CHECK_ERR("clReleaseMemObject", clReleaseMemObject(v_buffer1));
	CL_CHECK_ERR("clReleaseMemObject", clReleaseMemObject(v_buffer2));

	CL_CHECK_ERR("clReleaseKernel", clReleaseKernel(k_mem_copy));
	CL_CHECK_ERR("clReleaseProgram", clReleaseProgram(program));

	CL_CHECK_ERR("clReleaseCommandQueue", clReleaseCommandQueue(command_queue));
	CL_CHECK_ERR("clReleaseContext", clReleaseContext(context));
	return EXIT_SUCCESS;
}
