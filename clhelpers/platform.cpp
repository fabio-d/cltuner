#include "cl.h"

#include <cstdio>
#include <cstdlib>

static QString get_plaform_info_string(cl_platform_id platform, cl_platform_info param)
{
	cl_uint numchars;
	CL_CHECK_ERR("clGetPlatformInfo", clGetPlatformInfo(platform, param, 0, NULL, &numchars));

	char *buffer = new char[numchars];
	CL_CHECK_ERR("clGetPlatformInfo", clGetPlatformInfo(platform, param, numchars, buffer, NULL));

	const QString result(buffer);

	delete[] buffer;

	return result;
}

QList<QString> clhAvailablePlatformNames()
{
	cl_uint numplatforms;
	CL_CHECK_ERR("clGetPlatformIDs", clGetPlatformIDs(0, NULL, &numplatforms));

	cl_platform_id *platforms = new cl_platform_id[numplatforms];
	CL_CHECK_ERR("clGetPlatformIDs", clGetPlatformIDs(numplatforms, platforms, NULL));

	QList<QString> result;
	for (cl_uint i = 0; i < numplatforms; i++)
	{
		const QString platName = get_plaform_info_string(platforms[i], CL_PLATFORM_NAME);
		const QString platVersion = get_plaform_info_string(platforms[i], CL_PLATFORM_VERSION);
		result << QString("%0 - %1").arg(platName, platVersion);
	}

	delete[] platforms;

	return result;
}

cl_platform_id clhSelectPlatform(int index)
{
	cl_uint numplatforms;
	CL_CHECK_ERR("clGetPlatformIDs", clGetPlatformIDs(0, NULL, &numplatforms));

	cl_platform_id *platforms = new cl_platform_id[numplatforms];
	CL_CHECK_ERR("clGetPlatformIDs", clGetPlatformIDs(numplatforms, platforms, NULL));

	if (index < 0 || index > numplatforms)
	{
		fprintf(stderr, "clhSelectPlatform: Indice non valido\n");
		exit(EXIT_FAILURE);
	}

	cl_platform_id result = platforms[index];

	delete[] platforms;

	return result;
}
