#include "cl.h"

cl_context clhCreateContextSingleDevice(cl_platform_id platform, cl_device_id device)
{
	const cl_context_properties ctx_prop[] = { // è una lista di coppie
		CL_CONTEXT_PLATFORM, (cl_context_properties)platform,
		0 // terminatore lista
	};

	cl_int err;

	cl_context result = clCreateContext(ctx_prop, 1, &device, NULL, NULL, &err);
	CL_CHECK_ERR("clCreateContext", err);

	return result;
}

