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

// Leggo e scrivo sempre su buffer di questa dimensione
#define MEMSIZE_MB	1 // <-- modificabile!
#define MEMSIZE_KB	(MEMSIZE_MB * 1024)
#define MEMSIZE_BYTES	(MEMSIZE_KB * 1024)

// Tipo di dato OpenCL utilizzato per gli accessi
#define CL_DEVICE_DATATYPE	int2 // <-- modificabile!
#define CL_DEVICE_DATATYPE_STR	STRINGIFY(CL_DEVICE_DATATYPE)
#define CL_HOST_DATATYPE	CONCAT(cl_, CL_DEVICE_DATATYPE)

static cl_uint num_iterazioni;

static float runTest(cl_command_queue queue, cl_kernel kernel, cl_mem buffer, cl_mem dummyBuffer)
{
	size_t groupSize = 256;
	size_t globalSize = MEMSIZE_BYTES / sizeof(CL_HOST_DATATYPE);

	//printf("%d iterazioni * %d KiB/iterazione = %d KiB coinvolti\n", num_iterazioni, MEMSIZE_KB, num_iterazioni * MEMSIZE_KB);
	//printf("lancio %d workgroups con %d workitems ciascuno = %d workitem totali\n\n", globalSize / groupSize, groupSize, globalSize);

	CL_CHECK_ERR("clSetKernelArg", clSetKernelArg(kernel, 0, sizeof(cl_mem), &buffer));
	CL_CHECK_ERR("clSetKernelArg", clSetKernelArg(kernel, 1, sizeof(cl_int), &num_iterazioni));
	CL_CHECK_ERR("clSetKernelArg", clSetKernelArg(kernel, 2, sizeof(cl_mem), &dummyBuffer));

	cl_event kernel_evt;
	CL_CHECK_ERR("clEnqueueNDRangeKernel", clEnqueueNDRangeKernel(queue,
		kernel,
		1,
		NULL,
		&globalSize,
		&groupSize,
		0,
		NULL,
		&kernel_evt
	));

	const float secs = clhEventWaitAndGetDuration(kernel_evt);
	CL_CHECK_ERR("clReleaseEvent", clReleaseEvent(kernel_evt));

	return secs;
}

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

	if (argc != 2 || (num_iterazioni = atoi(argv[1])) <= 0)
	{
		fprintf(stderr, "Uso: %s <num-iterazioni> [cl-platform-num [cl-device-num]]\n", argv[0]);
		fprintf(stderr, "Specificare il numero di iterazioni da %d MiB desiderate e gli\n", MEMSIZE_MB);
		fprintf(stderr, "indici della piattaforma e del dispositivo OpenCL da utilizzare\n");
		return EXIT_FAILURE;
	}

	cl_platform_id platform = clhSelectPlatform(platform_index);
	fprintf(stderr, "CL platform: %s\n", clhGetPlatformFriendlyName(platform).c_str());

	cl_device_id device = clhSelectDevice(platform, device_index);
	fprintf(stderr, "CL device: %s\n", clhGetDeviceFriendlyName(device).c_str());

	cl_context context = clhCreateContextSingleDevice(platform, device);
	cl_command_queue command_queue = clhCreateCommandQueue(context, device, true /* profiling abilitato */);

	clhEmptyNvidiaCache();

	cl_program program = clhBuildProgram(context, device, "tools/cl_speed.cl", "-DDATATYPE=" CL_DEVICE_DATATYPE_STR);
	cl_kernel k_mem_read = clhCreateKernel(program, "mem_read");
	cl_kernel k_mem_write = clhCreateKernel(program, "mem_write");

	int megabytes = num_iterazioni * MEMSIZE_MB;

	cl_int err;
	cl_mem v_bufferRO = clCreateBuffer(context, CL_MEM_READ_ONLY, MEMSIZE_BYTES, NULL, &err);
	CL_CHECK_ERR("clCreateBuffer", err);
	cl_mem v_bufferWO = clCreateBuffer(context, CL_MEM_WRITE_ONLY, MEMSIZE_BYTES, NULL, &err);
	CL_CHECK_ERR("clCreateBuffer", err);
	cl_mem v_bufferRW = clCreateBuffer(context, CL_MEM_READ_WRITE, MEMSIZE_BYTES, NULL, &err);
	CL_CHECK_ERR("clCreateBuffer", err);
	cl_mem v_dummyBuffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(CL_HOST_DATATYPE), NULL, &err);
	CL_CHECK_ERR("clCreateBuffer", err);

	float letturaCL_MEM_READ_ONLY = runTest(command_queue, k_mem_read, v_bufferRO, v_dummyBuffer);
	float scritturaCL_MEM_WRITE_ONLY = runTest(command_queue, k_mem_write, v_bufferRO, v_dummyBuffer);
	float letturaCL_MEM_READ_WRITE = runTest(command_queue, k_mem_read, v_bufferRW, v_dummyBuffer);
	float scritturaCL_MEM_READ_WRITE = runTest(command_queue, k_mem_write, v_bufferRW, v_dummyBuffer);

	const float tempoDiUnaScrittura = scritturaCL_MEM_READ_WRITE / num_iterazioni;
	letturaCL_MEM_READ_ONLY -= tempoDiUnaScrittura;
	letturaCL_MEM_READ_WRITE -= tempoDiUnaScrittura;

	printf("Lettura da buffer CL_MEM_READ_ONLY\n");
	printf(" tempo impiegato: %g ms\n", letturaCL_MEM_READ_ONLY * 1e3);
	printf(" bandwidth: %g MiB/s\n\n", megabytes / letturaCL_MEM_READ_ONLY);

	printf("Scrittura su buffer CL_MEM_WRITE_ONLY\n");
	printf(" tempo impiegato: %g ms\n", scritturaCL_MEM_WRITE_ONLY * 1e3);
	printf(" bandwidth: %g MiB/s\n\n", megabytes / scritturaCL_MEM_WRITE_ONLY);

	printf("Lettura da buffer CL_MEM_READ_WRITE\n");
	printf(" tempo impiegato: %g ms\n", letturaCL_MEM_READ_WRITE * 1e3);
	printf(" bandwidth: %g MiB/s\n\n", megabytes / letturaCL_MEM_READ_WRITE);

	printf("Scrittura su buffer CL_MEM_READ_WRITE\n");
	printf(" tempo impiegato: %g ms\n", scritturaCL_MEM_READ_WRITE * 1e3);
	printf(" bandwidth: %g MiB/s\n\n", megabytes / scritturaCL_MEM_READ_WRITE);

	CL_CHECK_ERR("clFinish", clFinish(command_queue));

	CL_CHECK_ERR("clReleaseMemObject", clReleaseMemObject(v_bufferRO));
	CL_CHECK_ERR("clReleaseMemObject", clReleaseMemObject(v_bufferWO));
	CL_CHECK_ERR("clReleaseMemObject", clReleaseMemObject(v_bufferRW));
	CL_CHECK_ERR("clReleaseMemObject", clReleaseMemObject(v_dummyBuffer));

	CL_CHECK_ERR("clReleaseKernel", clReleaseKernel(k_mem_read));
	CL_CHECK_ERR("clReleaseKernel", clReleaseKernel(k_mem_write));
	CL_CHECK_ERR("clReleaseProgram", clReleaseProgram(program));

	CL_CHECK_ERR("clReleaseCommandQueue", clReleaseCommandQueue(command_queue));
	CL_CHECK_ERR("clReleaseContext", clReleaseContext(context));
	return EXIT_SUCCESS;
}
