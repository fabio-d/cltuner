#include "cl.h"

#include <cstdio>
#include <cstdlib>

static string get_device_info_string(cl_device_id device, cl_device_info param)
{
	cl_uint numchars;
	CL_CHECK_ERR("clGetDeviceInfo", clGetDeviceInfo(device, param, 0, NULL, &numchars));

	char *buffer = new char[numchars];
	CL_CHECK_ERR("clGetDeviceInfo", clGetDeviceInfo(device, param, numchars, buffer, NULL));

	string result(buffer);

	delete[] buffer;

	return result;
}

static string get_device_types(cl_device_id device)
{
	cl_device_type dt;
	CL_CHECK_ERR("clGetDeviceInfo", clGetDeviceInfo(device, CL_DEVICE_TYPE, sizeof(dt), &dt, NULL));

	string result;
	string sep;

	if (dt & CL_DEVICE_TYPE_CPU)
		result += sep + "CPU", sep = ", ";
	if (dt & CL_DEVICE_TYPE_GPU)
		result += sep + "GPU", sep = ", ";
	if (dt & CL_DEVICE_TYPE_ACCELERATOR)
		result += sep + "ACCELERATOR", sep = ", ";
	if (dt & CL_DEVICE_TYPE_DEFAULT)
		result += sep + "DEFAULT", sep = ", ";

	return result;
}

string clhGetDeviceFriendlyName(cl_device_id device)
{
	string devName = get_device_info_string(device, CL_DEVICE_NAME);

	// Elimina doppi spazi (la mia CPU ne ha troppi!)
	int dblspcpos;
	while ((dblspcpos = devName.find("  ")) != string::npos)
		devName.erase(dblspcpos, 1);

	return devName + " [" + get_device_types(device) + "]";
}

vector<string> clhAvailableDeviceNames(cl_platform_id platform)
{
	cl_uint numdevices;
	CL_CHECK_ERR("clGetDeviceIDs", clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 0, NULL, &numdevices));

	cl_device_id *devices = new cl_device_id[numdevices];
	CL_CHECK_ERR("clGetDeviceIDs", clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, numdevices, devices, NULL));

	vector<string> result;
	for (cl_uint i = 0; i < numdevices; i++)
		result.push_back(clhGetDeviceFriendlyName(devices[i]));

	delete[] devices;

	return result;
}

cl_device_id clhSelectDevice(cl_platform_id platform, int index)
{
	cl_uint numdevices;
	CL_CHECK_ERR("clGetDeviceIDs", clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 0, NULL, &numdevices));

	cl_device_id *devices = new cl_device_id[numdevices];
	CL_CHECK_ERR("clGetDeviceIDs", clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, numdevices, devices, NULL));

	if (index < 0 || index >= numdevices)
	{
		fprintf(stderr, "clhSelectDevice: Indice %d non valido\n", index);
		exit(EXIT_FAILURE);
	}

	cl_device_id result = devices[index];

	delete[] devices;

	return result;
}
