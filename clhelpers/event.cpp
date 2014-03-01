#include "cl.h"

float clhEventWaitAndGetDuration(cl_event event)
{
	cl_ulong start, end;

	CL_CHECK_ERR("clWaitForEvents", clWaitForEvents(1, &event));
	CL_CHECK_ERR("clGetEventProfilingInfo", clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &start, NULL));
	CL_CHECK_ERR("clGetEventProfilingInfo", clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &end, NULL));

	return (end - start) * 1e-9; // conversione ns -> secondi
}

float clhEventWaitAndGetDifference(cl_event eventStart, cl_event eventEnd)
{
	cl_ulong start, end;

	const cl_event both_events[2] = { eventStart, eventEnd };

	CL_CHECK_ERR("clWaitForEvents", clWaitForEvents(2, both_events));

	CL_CHECK_ERR("clGetEventProfilingInfo", clGetEventProfilingInfo(eventStart, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &start, NULL));
	CL_CHECK_ERR("clGetEventProfilingInfo", clGetEventProfilingInfo(eventEnd, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &end, NULL));

	return (end - start) * 1e-9; // conversione ns -> secondi
}
