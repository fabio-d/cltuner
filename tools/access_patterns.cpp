#include "clhelpers/cl.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

#include <unistd.h>

#define _STRINGIFY(arg)	#arg
#define STRINGIFY(arg)	_STRINGIFY(arg)
#define _CONCAT(a, b)	a ## b
#define CONCAT(a, b)	_CONCAT(a, b)

// Tipo di dato OpenCL utilizzato per gli accessi
#define CL_DEVICE_DATATYPE	int // <-- modificabile!
#define CL_DEVICE_DATATYPE_STR	STRINGIFY(CL_DEVICE_DATATYPE)
#define CL_HOST_DATATYPE	CONCAT(cl_, CL_DEVICE_DATATYPE)

static bool isdigit_string(const char *str)
{
	if (*str == '\0')
		return false; // Non accetta stringhe vuote

	while (*str && isdigit(*str)) str++;

	return *str == '\0'; // Accetta se siamo arrivati alla fine, ovvero tutti i caratteri erano numeri
}

int main(int argc, char **argv)
{
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

	if (argc != 1)
	{
		fprintf(stderr, "Uso: %s [cl-platform-num [cl-device-num]]\n", argv[0]);
		fprintf(stderr, "Specificare gli indici della piattaforma e del dispositivo\n");
		fprintf(stderr, "OpenCL da utilizzare\n");
		return EXIT_FAILURE;
	}

	cl_platform_id platform = clhSelectPlatform(platform_index);
	fprintf(stderr, "CL platform: %s\n", clhGetPlatformFriendlyName(platform).c_str());

	cl_device_id device = clhSelectDevice(platform, device_index);
	fprintf(stderr, "CL device: %s\n", clhGetDeviceFriendlyName(device).c_str());

	cl_context context = clhCreateContextSingleDevice(platform, device);
	cl_command_queue command_queue = clhCreateCommandQueue(context, device, true /* profiling abilitato */);

	clhEmptyNvidiaCache();

	cl_program program = clhBuildProgram(context, device, "tools/access_patterns.cl", "-DDATATYPE=" CL_DEVICE_DATATYPE_STR);
	cl_kernel k_same_location = clhCreateKernel(program, "same_location");
	cl_kernel k_aligned_sequential = clhCreateKernel(program, "aligned_sequential");
	cl_kernel k_aligned_nonsequential = clhCreateKernel(program, "aligned_nonsequential");
	cl_kernel k_misaligned_sequential = clhCreateKernel(program, "misaligned_sequential");

	size_t groupSize = 32;
	size_t globalSize = groupSize;

	cl_int err;
	cl_mem v_bufferRO = clCreateBuffer(context, CL_MEM_READ_ONLY, 64 * sizeof(CL_HOST_DATATYPE), NULL, &err);
	CL_CHECK_ERR("clCreateBuffer", err);
	cl_mem v_dummyBuffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(CL_HOST_DATATYPE), NULL, &err);
	CL_CHECK_ERR("clCreateBuffer", err);

	// Launch k_same_location
	fprintf(stderr, "Launching k_same_location...\n");
	CL_CHECK_ERR("clSetKernelArg", clSetKernelArg(k_same_location, 0, sizeof(cl_mem), &v_bufferRO));
	CL_CHECK_ERR("clSetKernelArg", clSetKernelArg(k_same_location, 1, sizeof(cl_mem), &v_dummyBuffer));
	CL_CHECK_ERR("clEnqueueNDRangeKernel", clEnqueueNDRangeKernel(command_queue,
		k_same_location,
		1,
		NULL,
		&globalSize,
		&groupSize,
		0,
		NULL,
		NULL
	));
	CL_CHECK_ERR("clFinish", clFinish(command_queue));

	// Launch k_aligned_sequential
	fprintf(stderr, "Launching k_aligned_sequential...\n");
	CL_CHECK_ERR("clSetKernelArg", clSetKernelArg(k_aligned_sequential, 0, sizeof(cl_mem), &v_bufferRO));
	CL_CHECK_ERR("clSetKernelArg", clSetKernelArg(k_aligned_sequential, 1, sizeof(cl_mem), &v_dummyBuffer));
	CL_CHECK_ERR("clEnqueueNDRangeKernel", clEnqueueNDRangeKernel(command_queue,
		k_aligned_sequential,
		1,
		NULL,
		&globalSize,
		&groupSize,
		0,
		NULL,
		NULL
	));
	CL_CHECK_ERR("clFinish", clFinish(command_queue));

	// Launch k_aligned_nonsequential
	fprintf(stderr, "Launching k_aligned_nonsequential...\n");
	CL_CHECK_ERR("clSetKernelArg", clSetKernelArg(k_aligned_nonsequential, 0, sizeof(cl_mem), &v_bufferRO));
	CL_CHECK_ERR("clSetKernelArg", clSetKernelArg(k_aligned_nonsequential, 1, sizeof(cl_mem), &v_dummyBuffer));
	CL_CHECK_ERR("clEnqueueNDRangeKernel", clEnqueueNDRangeKernel(command_queue,
		k_aligned_nonsequential,
		1,
		NULL,
		&globalSize,
		&groupSize,
		0,
		NULL,
		NULL
	));
	CL_CHECK_ERR("clFinish", clFinish(command_queue));

	// Launch k_misaligned_sequential
	fprintf(stderr, "Launching k_misaligned_sequential...\n");
	CL_CHECK_ERR("clSetKernelArg", clSetKernelArg(k_misaligned_sequential, 0, sizeof(cl_mem), &v_bufferRO));
	CL_CHECK_ERR("clSetKernelArg", clSetKernelArg(k_misaligned_sequential, 1, sizeof(cl_mem), &v_dummyBuffer));
	CL_CHECK_ERR("clEnqueueNDRangeKernel", clEnqueueNDRangeKernel(command_queue,
		k_misaligned_sequential,
		1,
		NULL,
		&globalSize,
		&groupSize,
		0,
		NULL,
		NULL
	));
	CL_CHECK_ERR("clFinish", clFinish(command_queue));

	CL_CHECK_ERR("clReleaseMemObject", clReleaseMemObject(v_bufferRO));
	CL_CHECK_ERR("clReleaseMemObject", clReleaseMemObject(v_dummyBuffer));

	CL_CHECK_ERR("clReleaseKernel", clReleaseKernel(k_same_location));
	CL_CHECK_ERR("clReleaseKernel", clReleaseKernel(k_aligned_sequential));
	CL_CHECK_ERR("clReleaseKernel", clReleaseKernel(k_aligned_nonsequential));
	CL_CHECK_ERR("clReleaseProgram", clReleaseProgram(program));

	CL_CHECK_ERR("clReleaseCommandQueue", clReleaseCommandQueue(command_queue));
	CL_CHECK_ERR("clReleaseContext", clReleaseContext(context));
	return EXIT_SUCCESS;
}
