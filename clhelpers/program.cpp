#include "cl.h"

#include <iostream>

cl_program clhBuildProgram(cl_context context, cl_device_id device, const char *srcFile, const char *buildOptions)
{
	cl_int err;
	const char *src[] = { "#include \"", srcFile, "\"\n" };
	cl_program program = clCreateProgramWithSource(context,
		3,
		src,
		NULL,
		&err);
	CL_CHECK_ERR("clCreateProgramWithSource", err);

	string options = "-I. "; // non tutti i compilatori OpenCL hanno la directory corrente tra i path predefiniti
	if (buildOptions != NULL)
		options += buildOptions;

	err = clBuildProgram(
		program,
		1,
		&device,
		options.c_str(),
		NULL,
		NULL);

	size_t logSize;
	clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &logSize);
	char *log = new char[logSize];
	clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, logSize, log, NULL);
	cerr << log;
	delete[] log;

	CL_CHECK_ERR("clBuildProgram", err);

	return program;
}
