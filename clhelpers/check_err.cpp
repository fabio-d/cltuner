#include "cl.h"

#include <cstdio>
#include <cstdlib>

void CL_CHECK_ERR(const char *prefix, cl_int status)
{
	const char *errtext;

	switch (status)
	{
		case CL_SUCCESS:
			return;
		case CL_DEVICE_NOT_FOUND:
			errtext = "Device not found.";
			break;
		case CL_DEVICE_NOT_AVAILABLE:
			errtext = "Device not available";
			break;
		case CL_COMPILER_NOT_AVAILABLE:
			errtext = "Compiler not available";
			break;
		case CL_MEM_OBJECT_ALLOCATION_FAILURE:
			errtext = "Memory object allocation failure";
			break;
		case CL_OUT_OF_RESOURCES:
			errtext = "Out of resources";
			break;
		case CL_OUT_OF_HOST_MEMORY:
			errtext = "Out of host memory";
			break;
		case CL_PROFILING_INFO_NOT_AVAILABLE:
			errtext = "Profiling information not available";
			break;
		case CL_MEM_COPY_OVERLAP:
			errtext = "Memory copy overlap";
			break;
		case CL_IMAGE_FORMAT_MISMATCH:
			errtext = "Image format mismatch";
			break;
		case CL_IMAGE_FORMAT_NOT_SUPPORTED:
			errtext = "Image format not supported";
			break;
		case CL_BUILD_PROGRAM_FAILURE:
			errtext = "Program build failure";
			break;
		case CL_MAP_FAILURE:
			errtext = "Map failure";
			break;
		case CL_INVALID_VALUE:
			errtext = "Invalid value";
			break;
		case CL_INVALID_DEVICE_TYPE:
			errtext = "Invalid device type";
			break;
		case CL_INVALID_PLATFORM:
			errtext = "Invalid platform";
			break;
		case CL_INVALID_DEVICE:
			errtext = "Invalid device";
			break;
		case CL_INVALID_CONTEXT:
			errtext = "Invalid context";
			break;
		case CL_INVALID_QUEUE_PROPERTIES:
			errtext = "Invalid queue properties";
			break;
		case CL_INVALID_COMMAND_QUEUE:
			errtext = "Invalid command queue";
			break;
		case CL_INVALID_HOST_PTR:
			errtext = "Invalid host pointer";
			break;
		case CL_INVALID_MEM_OBJECT:
			errtext = "Invalid memory object";
			break;
		case CL_INVALID_IMAGE_FORMAT_DESCRIPTOR:
			errtext = "Invalid image format descriptor";
			break;
		case CL_INVALID_IMAGE_SIZE:
			errtext = "Invalid image size";
			break;
		case CL_INVALID_SAMPLER:
			errtext = "Invalid sampler";
			break;
		case CL_INVALID_BINARY:
			errtext = "Invalid binary";
			break;
		case CL_INVALID_BUILD_OPTIONS:
			errtext = "Invalid build options";
			break;
		case CL_INVALID_PROGRAM:
			errtext = "Invalid program";
			break;
		case CL_INVALID_PROGRAM_EXECUTABLE:
			errtext = "Invalid program executable";
			break;
		case CL_INVALID_KERNEL_NAME:
			errtext = "Invalid kernel name";
			break;
		case CL_INVALID_KERNEL_DEFINITION:
			errtext = "Invalid kernel definition";
			break;
		case CL_INVALID_KERNEL:
			errtext = "Invalid kernel";
			break;
		case CL_INVALID_ARG_INDEX:
			errtext = "Invalid argument index";
			break;
		case CL_INVALID_ARG_VALUE:
			errtext = "Invalid argument value";
			break;
		case CL_INVALID_ARG_SIZE:
			errtext = "Invalid argument size";
			break;
		case CL_INVALID_KERNEL_ARGS:
			errtext = "Invalid kernel arguments";
			break;
		case CL_INVALID_WORK_DIMENSION:
			errtext = "Invalid work dimension";
			break;
		case CL_INVALID_WORK_GROUP_SIZE:
			errtext = "Invalid work group size";
			break;
		case CL_INVALID_WORK_ITEM_SIZE:
			errtext = "Invalid work item size";
			break;
		case CL_INVALID_GLOBAL_OFFSET:
			errtext = "Invalid global offset";
			break;
		case CL_INVALID_EVENT_WAIT_LIST:
			errtext = "Invalid event wait list";
			break;
		case CL_INVALID_EVENT:
			errtext = "Invalid event";
			break;
		case CL_INVALID_OPERATION:
			errtext = "Invalid operation";
			break;
		case CL_INVALID_GL_OBJECT:
			errtext = "Invalid OpenGL object";
			break;
		case CL_INVALID_BUFFER_SIZE:
			errtext = "Invalid buffer size";
			break;
		case CL_INVALID_MIP_LEVEL:
			errtext = "Invalid mip-map level";
			break;
		default:
			errtext = "Unknown error";
			break;
	}

	fprintf(stderr, "%s: %s\n", prefix, errtext);
	exit(EXIT_FAILURE);
}
