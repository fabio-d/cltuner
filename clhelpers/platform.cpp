#include "cl.h"

#include <cstdio>
#include <cstdlib>

static string get_plaform_info_string(cl_platform_id platform, cl_platform_info param)
{
	cl_uint numchars;
	CL_CHECK_ERR("clGetPlatformInfo", clGetPlatformInfo(platform, param, 0, NULL, &numchars));

	char *buffer = new char[numchars];
	CL_CHECK_ERR("clGetPlatformInfo", clGetPlatformInfo(platform, param, numchars, buffer, NULL));

	const string result(buffer);

	delete[] buffer;

	return result;
}

string clhGetPlatformFriendlyName(cl_platform_id platform)
{
	const string platName = get_plaform_info_string(platform, CL_PLATFORM_NAME);
	const string platVersion = get_plaform_info_string(platform, CL_PLATFORM_VERSION);

	return platName + " - " + platVersion;
}

vector<string> clhAvailablePlatformNames()
{
	cl_uint numplatforms;
	CL_CHECK_ERR("clGetPlatformIDs", clGetPlatformIDs(0, NULL, &numplatforms));

	cl_platform_id *platforms = new cl_platform_id[numplatforms];
	CL_CHECK_ERR("clGetPlatformIDs", clGetPlatformIDs(numplatforms, platforms, NULL));

	vector<string> result;
	for (cl_uint i = 0; i < numplatforms; i++)
		result.push_back(clhGetPlatformFriendlyName(platforms[i]));

	delete[] platforms;

	return result;
}

cl_platform_id clhSelectPlatform(int index)
{
	cl_uint numplatforms;
	CL_CHECK_ERR("clGetPlatformIDs", clGetPlatformIDs(0, NULL, &numplatforms));

	cl_platform_id *platforms = new cl_platform_id[numplatforms];
	CL_CHECK_ERR("clGetPlatformIDs", clGetPlatformIDs(numplatforms, platforms, NULL));

	if (index < 0 || index >= numplatforms)
	{
		fprintf(stderr, "clhSelectPlatform: Indice %d non valido\n", index);
		exit(EXIT_FAILURE);
	}

	cl_platform_id result = platforms[index];

	delete[] platforms;

	return result;
}
