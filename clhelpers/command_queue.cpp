#include "cl.h"

cl_command_queue clhCreateCommandQueue(cl_context context, cl_device_id device, bool enableProfiling)
{
	cl_int err;
	cl_command_queue q = clCreateCommandQueue(context, device, enableProfiling ? CL_QUEUE_PROFILING_ENABLE : 0, &err);
	CL_CHECK_ERR("clCreateCommandQueue", err);
	return q;
}
