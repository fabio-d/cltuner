#include "cl.h"

#include <QRegExp>
#include <QStringList>

static QString get_device_info_string(cl_device_id device, cl_device_info param)
{
	cl_uint numchars;
	CL_CHECK_ERR("clGetDeviceInfo", clGetDeviceInfo(device, param, 0, NULL, &numchars));

	char *buffer = new char[numchars];
	CL_CHECK_ERR("clGetDeviceInfo", clGetDeviceInfo(device, param, numchars, buffer, NULL));

	const QString result(buffer);

	delete[] buffer;

	return result;
}

static QString get_device_types(cl_device_id device)
{
	cl_device_type dt;
	CL_CHECK_ERR("clGetDeviceInfo", clGetDeviceInfo(device, CL_DEVICE_TYPE, sizeof(dt), &dt, NULL));

	QStringList result;

	if (dt & CL_DEVICE_TYPE_CPU)
		result << "CPU";
	if (dt & CL_DEVICE_TYPE_GPU)
		result << "GPU";
	if (dt & CL_DEVICE_TYPE_ACCELERATOR)
		result << "ACCELERATOR";
	if (dt & CL_DEVICE_TYPE_DEFAULT)
		result << "DEFAULT";

	return result.join(", ");
}

QList<QString> clhAvailableDeviceNames(cl_platform_id platform)
{
	cl_uint numdevices;
	CL_CHECK_ERR("clGetDeviceIDs", clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 0, NULL, &numdevices));

	cl_device_id *devices = new cl_device_id[numdevices];
	CL_CHECK_ERR("clGetDeviceIDs", clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, numdevices, devices, NULL));

	QList<QString> result;
	for (cl_uint i = 0; i < numdevices; i++)
	{
		QString devName = get_device_info_string(devices[i], CL_DEVICE_NAME);
		devName.replace(QRegExp("  +"), " "); // Elimina doppi spazi (la mia CPU ne ha troppi!)

		const QString devTypes = get_device_types(devices[i]);
		result << QString("%0 [%1]").arg(devName, devTypes);
	}

	delete[] devices;

	return result;
}
