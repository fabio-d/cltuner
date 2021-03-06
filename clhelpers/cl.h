#ifndef CLHELPERS_CL_H
#define CLHELPERS_CL_H

#define CL_USE_DEPRECATED_OPENCL_1_1_APIS
#include <CL/cl.h>

#include <vector>
#include <string>

using namespace std;

// align.cpp
extern size_t clhAlignUp(size_t value, size_t alignment);

// check_err.cpp
extern void CL_CHECK_ERR(const char *prefix, cl_int status);

// command_queue.cpp
extern cl_command_queue clhCreateCommandQueue(cl_context context, cl_device_id device, bool enableProfiling);

// context.cpp
extern cl_context clhCreateContextSingleDevice(cl_platform_id platform, cl_device_id device);

// device.cpp
extern string clhGetDeviceFriendlyName(cl_device_id device);
extern vector<string> clhAvailableDeviceNames(cl_platform_id platform);
extern cl_device_id clhSelectDevice(cl_platform_id platform, int index);

// event.cpp
extern float clhEventWaitAndGetDuration(cl_event evt);
extern float clhEventWaitAndGetDifference(cl_event eventStart, cl_event eventEnd);

// kernel.cpp
extern cl_kernel clhCreateKernel(cl_program program, const char *kernelName);

// nvidia_cache.cpp
extern void clhEmptyNvidiaCache();

// platform.cpp
extern string clhGetPlatformFriendlyName(cl_platform_id platform);
extern vector<string> clhAvailablePlatformNames();
extern cl_platform_id clhSelectPlatform(int index);

// program.cpp
extern cl_program clhBuildProgram(cl_context context, cl_device_id device, const char *srcFile, const char *buildOptions = NULL);

#endif // CLHELPERS_CL_H
