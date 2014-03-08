#ifndef DFTALGORITHMS_CLBASE_H
#define DFTALGORITHMS_CLBASE_H

#include "clhelpers/cl.h"
#include <iostream>

class cl_base
{
	protected:
		cl_base(int platform_index, int device_index, int samplesPerRun)
		: samplesPerRun(samplesPerRun), ownsContextAndQueue(true)
		{
			platform = clhSelectPlatform(platform_index);
			cerr << "CL platform: " << clhGetPlatformFriendlyName(platform) << endl;

			device = clhSelectDevice(platform, device_index);
			cerr << "CL device: " << clhGetDeviceFriendlyName(device) << endl;

			context = clhCreateContextSingleDevice(platform, device);
			command_queue = clhCreateCommandQueue(context, device, true /* profiling abilitato */);

			clhEmptyNvidiaCache();
		}

		cl_base(cl_platform_id platform, cl_device_id device, cl_context context,
			cl_command_queue command_queue, int samplesPerRun)
		: samplesPerRun(samplesPerRun), platform(platform), device(device),
		  context(context), command_queue(command_queue), ownsContextAndQueue(false)
		{
		}

		virtual ~cl_base()
		{
			if (ownsContextAndQueue)
			{
				CL_CHECK_ERR("clReleaseCommandQueue", clReleaseCommandQueue(command_queue));
				CL_CHECK_ERR("clReleaseContext", clReleaseContext(context));
			}
		}

		int samplesPerRun;

		cl_platform_id platform;
		cl_device_id device;
		cl_context context;
		cl_command_queue command_queue;

	private:
		bool ownsContextAndQueue;
};

#endif // DFTALGORITHMS_CLBASE_H
