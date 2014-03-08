#include "cl_base.h"
#include "cpx.h"
#include "pi_float.h"
#include "sizeconv.h"

#include <cstdio>
#include <cstdlib>

#define _STRINGIFY(arg)	#arg
#define STRINGIFY(arg)	_STRINGIFY(arg)

template <typename T> const char *cl_dft_symmatrix_algoName();

template <> const char *cl_dft_symmatrix_algoName<float>()
{ return "OpenCL DFT symmetric matrix (real-to-complex)"; }

template <> const char *cl_dft_symmatrix_algoName<cpx>()
{ return "OpenCL DFT symmetric matrix (complex-to-complex)"; }

#define GS 16

template <typename T>
class cl_dft_symmatrix : public cl_base
{
	public:
		cl_dft_symmatrix(int platform_index, int device_index, int samplesPerRun);
		vector<cpx> run(const vector<T> &input);
		~cl_dft_symmatrix();

	private:
		void printStatsAndReleaseEvents(cl_event upload_unmap_evt, cl_event step1_evt, cl_event step2_evt, cl_event download_map_evt);

		size_t coeffsMemSize, samplesMemSize, subtotMemSize, resultMemSize;
		size_t step1_groupSize[2], step1_globalSize[2];
		size_t step2_groupSize, step2_globalSize;

		cl_program program;
		cl_kernel k_mtx_init, k_step1_cpx2cpx, k_step1_real2cpx, k_step2;

		cl_uint subtotRows;

		cl_mem v_coeffs;
		cl_mem v_samples;
		cl_mem v_subtot;
		cl_mem v_result;
};

template <typename T>
cl_dft_symmatrix<T>::cl_dft_symmatrix(int platform_index, int device_index, int samplesPerRun)
: cl_base(platform_index, device_index, samplesPerRun)
{
	program = clhBuildProgram(context, device, "dft-algorithms/cl_dft_symmatrix.cl", "-DGS=" STRINGIFY(GS));
	k_mtx_init = clhCreateKernel(program, "symmtx_init");
	k_step1_cpx2cpx = clhCreateKernel(program, "step1_cpx2cpx");
	k_step1_real2cpx = clhCreateKernel(program, "step1_real2cpx");
	k_step2 = clhCreateKernel(program, "step2");

	// Parametri di lancio dei kernel step1_cpx2cpx e step1_real2cpx.
	step1_groupSize[0] = GS;
	step1_globalSize[0] = clhAlignUp(samplesPerRun, GS);
	step1_groupSize[1] = GS;
	step1_globalSize[1] = clhAlignUp(samplesPerRun, GS);

	// Parametri di lancio dello step2
	step2_groupSize = atoi(getenv("GS_X") ?: "128");
	step2_globalSize = clhAlignUp(samplesPerRun, step2_groupSize);

	cl_int err;
	coeffsMemSize = samplesPerRun * samplesPerRun * sizeof(cl_float2);
	v_coeffs = clCreateBuffer(context, CL_MEM_READ_WRITE, coeffsMemSize, NULL, &err);
	CL_CHECK_ERR("clCreateBuffer", err);

	samplesMemSize = cl_deviceDataSize<T>(samplesPerRun);
	v_samples = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR, samplesMemSize, NULL, &err);
	CL_CHECK_ERR("clCreateBuffer", err);

	subtotRows = clhAlignUp(samplesPerRun, GS) / GS;
	subtotMemSize = subtotRows * samplesPerRun * sizeof(cl_float2);
	v_subtot = clCreateBuffer(context, CL_MEM_READ_WRITE, subtotMemSize, NULL, &err);
	CL_CHECK_ERR("clCreateBuffer", err);

	resultMemSize = samplesPerRun * sizeof(cl_float2);
	v_result = clCreateBuffer(context, CL_MEM_WRITE_ONLY | CL_MEM_ALLOC_HOST_PTR, resultMemSize, NULL, &err);
	CL_CHECK_ERR("clCreateBuffer", err);

	fprintf(stderr, "Memoria occupata dalla DFT matrix [N=%d]: %g MiB\n",
		samplesPerRun, coeffsMemSize / 1.024e3 / 1.024e3);
	fprintf(stderr, "Memoria occupata dalla tabella dei subtotali [N=%d]: %g MiB\n\n",
		samplesPerRun, subtotMemSize / 1.024e3 / 1.024e3);

	// Inizializziamo la matrice dei coefficienti
	cl_event init_evt;
	cl_uint samplesPerRunAsCLUint = samplesPerRun;
	cl_float parteFissa = M_PI_F/samplesPerRun*-2.f;

	const size_t init_groupSize[2] = { 16, 8 };
	const size_t init_globalSize[2] = {
		clhAlignUp(samplesPerRun, init_groupSize[0]),
		clhAlignUp(samplesPerRun, init_groupSize[1])
	};

	CL_CHECK_ERR("clSetKernelArg", clSetKernelArg(k_mtx_init, 0, sizeof(cl_mem), &v_coeffs));
	CL_CHECK_ERR("clSetKernelArg", clSetKernelArg(k_mtx_init, 1, sizeof(cl_uint), &samplesPerRunAsCLUint));
	CL_CHECK_ERR("clSetKernelArg", clSetKernelArg(k_mtx_init, 2, sizeof(cl_float), &parteFissa));
	CL_CHECK_ERR("clEnqueueNDRangeKernel", clEnqueueNDRangeKernel(command_queue,
		k_mtx_init,
		2,
		NULL,
		init_globalSize,
		init_groupSize,
		0,
		NULL,
		&init_evt
	));

	const float init_secs = clhEventWaitAndGetDuration(init_evt);
	const float init_memSizeMiB = coeffsMemSize / SIZECONV_MB;

	fprintf(stderr, "%s [N=%d, GS=%dx%d]:\n",
		"OpenCL DFT matrix initialization", samplesPerRun, init_groupSize[0], init_groupSize[1]);

	fprintf(stderr, " kernel %g ms, %g MiB/s, %g valori/s\n",
		init_secs * 1e3, init_memSizeMiB / init_secs, (samplesPerRun*samplesPerRun) / init_secs);

	CL_CHECK_ERR("clReleaseEvent", clReleaseEvent(init_evt));
}

template <>
vector<cpx> cl_dft_symmatrix<cpx>::run(const vector<cpx> &input)
{
	cl_event upload_unmap_evt, step1_evt, step2_evt, download_map_evt;
	cl_int err;

	// Upload
	cl_float2 *input_buffer = (cl_float2*)clEnqueueMapBuffer(command_queue,
		v_samples,
		CL_TRUE,
		CL_MAP_WRITE,
		0,
		samplesMemSize,
		0,
		NULL,
		NULL,
		&err);
	CL_CHECK_ERR("clEnqueueMapBuffer", err);

	for (int i = 0; i < samplesPerRun; i++)
	{
		input_buffer[i].x = real(input[i]);
		input_buffer[i].y = imag(input[i]);
	}

	CL_CHECK_ERR("clEnqueueUnmapMemObject", clEnqueueUnmapMemObject(command_queue, v_samples, input_buffer, 0, NULL, &upload_unmap_evt));

	// Lancio del primo step
	cl_uint samplesPerRunAsCLUint = samplesPerRun;
	CL_CHECK_ERR("clSetKernelArg", clSetKernelArg(k_step1_cpx2cpx, 0, sizeof(cl_mem), &v_samples));
	CL_CHECK_ERR("clSetKernelArg", clSetKernelArg(k_step1_cpx2cpx, 1, sizeof(cl_mem), &v_subtot));
	CL_CHECK_ERR("clSetKernelArg", clSetKernelArg(k_step1_cpx2cpx, 2, sizeof(cl_uint), &samplesPerRunAsCLUint));
	CL_CHECK_ERR("clSetKernelArg", clSetKernelArg(k_step1_cpx2cpx, 3, sizeof(cl_mem), &v_coeffs));
	CL_CHECK_ERR("clEnqueueNDRangeKernel", clEnqueueNDRangeKernel(command_queue,
		k_step1_cpx2cpx,
		2,
		NULL,
		step1_globalSize,
		step1_groupSize,
		1,
		&upload_unmap_evt,
		&step1_evt
	));

	// Lancio del secondo step
	CL_CHECK_ERR("clSetKernelArg", clSetKernelArg(k_step2, 0, sizeof(cl_mem), &v_subtot));
	CL_CHECK_ERR("clSetKernelArg", clSetKernelArg(k_step2, 1, sizeof(cl_mem), &v_result));
	CL_CHECK_ERR("clSetKernelArg", clSetKernelArg(k_step2, 2, sizeof(cl_uint), &samplesPerRunAsCLUint));
	CL_CHECK_ERR("clSetKernelArg", clSetKernelArg(k_step2, 3, sizeof(cl_uint), &subtotRows));
	CL_CHECK_ERR("clEnqueueNDRangeKernel", clEnqueueNDRangeKernel(command_queue,
		k_step2,
		1,
		NULL,
		&step2_globalSize,
		&step2_groupSize,
		1,
		&step1_evt,
		&step2_evt
	));

	// Download
	vector<cpx> result(samplesPerRun);
	cl_float2 *output_buffer = (cl_float2*)clEnqueueMapBuffer(command_queue,
		v_result,
		CL_TRUE,
		CL_MAP_READ,
		0,
		resultMemSize,
		1,
		&step2_evt,
		&download_map_evt,
		&err);
	CL_CHECK_ERR("clEnqueueMapBuffer", err);

	for (int i = 0; i < samplesPerRun; i++)
		result[i] = cpx(output_buffer[i].x, output_buffer[i].y);

	CL_CHECK_ERR("clEnqueueUnmapMemObject", clEnqueueUnmapMemObject(command_queue, v_result, output_buffer, 0, NULL, NULL));

	printStatsAndReleaseEvents(upload_unmap_evt, step1_evt, step2_evt, download_map_evt);

	return result;
}

template <>
vector<cpx> cl_dft_symmatrix<float>::run(const vector<float> &input)
{
	cl_event upload_unmap_evt, step1_evt, step2_evt, download_map_evt;
	cl_int err;

	// Upload
	cl_float *input_buffer = (cl_float*)clEnqueueMapBuffer(command_queue,
		v_samples,
		CL_TRUE,
		CL_MAP_WRITE,
		0,
		samplesMemSize,
		0,
		NULL,
		NULL,
		&err);
	CL_CHECK_ERR("clEnqueueMapBuffer", err);

	for (int i = 0; i < samplesPerRun; i++)
		input_buffer[i] = input[i];

	CL_CHECK_ERR("clEnqueueUnmapMemObject", clEnqueueUnmapMemObject(command_queue, v_samples, input_buffer, 0, NULL, &upload_unmap_evt));

	// Lancio del primo step
	cl_uint samplesPerRunAsCLUint = samplesPerRun;
	CL_CHECK_ERR("clSetKernelArg", clSetKernelArg(k_step1_real2cpx, 0, sizeof(cl_mem), &v_samples));
	CL_CHECK_ERR("clSetKernelArg", clSetKernelArg(k_step1_real2cpx, 1, sizeof(cl_mem), &v_subtot));
	CL_CHECK_ERR("clSetKernelArg", clSetKernelArg(k_step1_real2cpx, 2, sizeof(cl_uint), &samplesPerRunAsCLUint));
	CL_CHECK_ERR("clSetKernelArg", clSetKernelArg(k_step1_real2cpx, 3, sizeof(cl_mem), &v_coeffs));
	CL_CHECK_ERR("clEnqueueNDRangeKernel", clEnqueueNDRangeKernel(command_queue,
		k_step1_real2cpx,
		2,
		NULL,
		step1_globalSize,
		step1_groupSize,
		1,
		&upload_unmap_evt,
		&step1_evt
	));

	// Lancio del secondo step
	CL_CHECK_ERR("clSetKernelArg", clSetKernelArg(k_step2, 0, sizeof(cl_mem), &v_subtot));
	CL_CHECK_ERR("clSetKernelArg", clSetKernelArg(k_step2, 1, sizeof(cl_mem), &v_result));
	CL_CHECK_ERR("clSetKernelArg", clSetKernelArg(k_step2, 2, sizeof(cl_uint), &samplesPerRunAsCLUint));
	CL_CHECK_ERR("clSetKernelArg", clSetKernelArg(k_step2, 3, sizeof(cl_uint), &subtotRows));
	CL_CHECK_ERR("clEnqueueNDRangeKernel", clEnqueueNDRangeKernel(command_queue,
		k_step2,
		1,
		NULL,
		&step2_globalSize,
		&step2_groupSize,
		1,
		&step1_evt,
		&step2_evt
	));

	// Download
	vector<cpx> result(samplesPerRun);
	cl_float2 *output_buffer = (cl_float2*)clEnqueueMapBuffer(command_queue,
		v_result,
		CL_TRUE,
		CL_MAP_READ,
		0,
		resultMemSize,
		1,
		&step2_evt,
		&download_map_evt,
		&err);
	CL_CHECK_ERR("clEnqueueMapBuffer", err);

	for (int i = 0; i < samplesPerRun; i++)
		result[i] = cpx(output_buffer[i].x, output_buffer[i].y);

	CL_CHECK_ERR("clEnqueueUnmapMemObject", clEnqueueUnmapMemObject(command_queue, v_result, output_buffer, 0, NULL, NULL));

	printStatsAndReleaseEvents(upload_unmap_evt, step1_evt, step2_evt, download_map_evt);

	return result;
}

template <typename T>
void cl_dft_symmatrix<T>::printStatsAndReleaseEvents(cl_event upload_unmap_evt, cl_event step1_evt, cl_event step2_evt, cl_event download_map_evt)
{
	const float upload_secs = clhEventWaitAndGetDuration(upload_unmap_evt);
	const float upload_memSizeMiB = samplesMemSize / SIZECONV_MB;

	const float step1_secs = clhEventWaitAndGetDuration(step1_evt);
	const float step1_memSizeMiB = (samplesPerRun*samplesMemSize/GS + coeffsMemSize/2 + subtotMemSize) / SIZECONV_MB;
	const float step2_secs = clhEventWaitAndGetDuration(step2_evt);
	const float step2_memSizeMiB = (subtotMemSize + resultMemSize) / SIZECONV_MB;
	const float stepBOTH_secs = step1_secs + step2_secs;

	const float download_secs = clhEventWaitAndGetDuration(download_map_evt);
	const float download_memSizeMiB = resultMemSize / SIZECONV_MB;

	fprintf(stderr, "%s [N=%d, step1_GS=%dx%d, step2_GS=%d]:\n",
		cl_dft_symmatrix_algoName<T>(), samplesPerRun,
		step1_groupSize[0], step1_groupSize[1], step2_groupSize);

	fprintf(stderr, " upload %g ms, %g MiB/s\n",
		upload_secs * 1e3, upload_memSizeMiB / upload_secs);
	fprintf(stderr, " step1 %g ms, %g MiB/s\n",
		step1_secs * 1e3, step1_memSizeMiB / step1_secs);
	fprintf(stderr, " step2 %g ms, %g MiB/s\n",
		step2_secs * 1e3, step2_memSizeMiB / step2_secs);
	fprintf(stderr, " step1+step2 %g ms, %g samples/s\n",
		stepBOTH_secs * 1e3, samplesPerRun / stepBOTH_secs);
	fprintf(stderr, " download %g ms, %g MiB/s\n",
		download_secs * 1e3, download_memSizeMiB / download_secs);

	if (getenv("PRINT_KERNEL_EXECUTION_TIME"))
		printf("%g\n", stepBOTH_secs * 1e3);

	CL_CHECK_ERR("clReleaseEvent", clReleaseEvent(upload_unmap_evt));
	CL_CHECK_ERR("clReleaseEvent", clReleaseEvent(step1_evt));
	CL_CHECK_ERR("clReleaseEvent", clReleaseEvent(step2_evt));
	CL_CHECK_ERR("clReleaseEvent", clReleaseEvent(download_map_evt));
}
template <typename T>
cl_dft_symmatrix<T>::~cl_dft_symmatrix()
{
	CL_CHECK_ERR("clReleaseMemObject", clReleaseMemObject(v_coeffs));
	CL_CHECK_ERR("clReleaseMemObject", clReleaseMemObject(v_samples));
	CL_CHECK_ERR("clReleaseMemObject", clReleaseMemObject(v_subtot));
	CL_CHECK_ERR("clReleaseMemObject", clReleaseMemObject(v_result));

	CL_CHECK_ERR("clReleaseKernel", clReleaseKernel(k_mtx_init));
	CL_CHECK_ERR("clReleaseKernel", clReleaseKernel(k_step1_cpx2cpx));
	CL_CHECK_ERR("clReleaseKernel", clReleaseKernel(k_step1_real2cpx));
	CL_CHECK_ERR("clReleaseKernel", clReleaseKernel(k_step2));
	CL_CHECK_ERR("clReleaseProgram", clReleaseProgram(program));
}

#undef _STRINGIFY
#undef STRINGIFY
#undef GS
