#include "cl.h"

float clhEventWaitAndGetDuration(cl_event event)
{
	cl_ulong start, end;

	CL_CHECK_ERR("clWaitForEvents", clWaitForEvents(1, &event));
	CL_CHECK_ERR("clGetEventProfilingInfo", clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &start, NULL));
	CL_CHECK_ERR("clGetEventProfilingInfo", clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &end, NULL));

	return (end - start) * 1e-9; // conversione ns -> secondi
}
