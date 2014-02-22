#include "cl.h"

cl_kernel clhCreateKernel(cl_program program, const char *kernelName)
{
	cl_int err;
	cl_kernel kernel = clCreateKernel(program, kernelName, &err);
	CL_CHECK_ERR("clCreateKernel", err);
	return kernel;
}
