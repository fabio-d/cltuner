#include "cl_base.h"
#include "cpx.h"

#include <cstdio>

template <typename T> const char *cl_naive_dft_algoName();

template <> const char *cl_naive_dft_algoName<float>()
{ return "OpenCL naive DFT (real-to-complex)"; }

template <> const char *cl_naive_dft_algoName<cpx>()
{ return "OpenCL naive DFT (complex-to-complex)"; }

template <typename T>
class cl_naive_dft : public cl_base
{
	public:
		cl_naive_dft(int platform_index, int device_index, int samplesPerRun);
		vector<cpx> run(const vector<T> &input);
		void printStatsAndReleaseEvents(cl_event upload_unmap_evt, cl_event kernel_evt, cl_event download_map_evt);
		~cl_naive_dft();

	private:
		size_t samplesMemSize, resultMemSize;

		cl_program program;
		cl_kernel k_dft_cpx2cpx, k_dft_real2cpx;

		cl_mem v_samples;
		cl_mem v_result;
};

template <typename T>
cl_naive_dft<T>::cl_naive_dft(int platform_index, int device_index, int samplesPerRun)
: cl_base(platform_index, device_index, samplesPerRun)
{
	program = clhBuildProgram(context, device, "dft-algorithms/cl_naive_dft.cl");
	k_dft_cpx2cpx = clhCreateKernel(program, "dft_cpx2cpx");
	k_dft_real2cpx = clhCreateKernel(program, "dft_real2cpx");

	cl_int err;
	samplesMemSize = cl_deviceDataSize<T>(samplesPerRun);
	v_samples = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR, samplesMemSize, NULL, &err);
	CL_CHECK_ERR("clCreateBuffer", err);

	resultMemSize = samplesPerRun * sizeof(cl_float2);
	v_result = clCreateBuffer(context, CL_MEM_WRITE_ONLY | CL_MEM_ALLOC_HOST_PTR, resultMemSize, NULL, &err);
	CL_CHECK_ERR("clCreateBuffer", err);
}

template <>
vector<cpx> cl_naive_dft<cpx>::run(const vector<cpx> &input)
{
	cl_event upload_unmap_evt, kernel_evt, download_map_evt;
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

	// Lancio del kernel
	cl_uint samplesPerRunAsCLUint = samplesPerRun;
	size_t step_groupSize = 256;
	size_t step_globalSize = (samplesPerRun + step_groupSize - 1) & ~(step_groupSize - 1);
	CL_CHECK_ERR("clSetKernelArg", clSetKernelArg(k_dft_cpx2cpx, 0, sizeof(cl_mem), &v_samples));
	CL_CHECK_ERR("clSetKernelArg", clSetKernelArg(k_dft_cpx2cpx, 1, sizeof(cl_mem), &v_result));
	CL_CHECK_ERR("clSetKernelArg", clSetKernelArg(k_dft_cpx2cpx, 2, sizeof(cl_uint), &samplesPerRunAsCLUint));
	CL_CHECK_ERR("clEnqueueNDRangeKernel", clEnqueueNDRangeKernel(command_queue,
		k_dft_cpx2cpx,
		1,
		NULL,
		&step_globalSize,
		&step_groupSize,
		1,
		&upload_unmap_evt,
		&kernel_evt
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
		&kernel_evt,
		&download_map_evt,
		&err);
	CL_CHECK_ERR("clEnqueueMapBuffer", err);

	for (int i = 0; i < samplesPerRun; i++)
		result[i] = cpx(output_buffer[i].x, output_buffer[i].y);

	CL_CHECK_ERR("clEnqueueUnmapMemObject", clEnqueueUnmapMemObject(command_queue, v_result, output_buffer, 0, NULL, NULL));

	printStatsAndReleaseEvents(upload_unmap_evt, kernel_evt, download_map_evt);

	return result;
}

template <>
vector<cpx> cl_naive_dft<float>::run(const vector<float> &input)
{
	cl_event upload_unmap_evt, kernel_evt, download_map_evt;
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

	// Lancio del kernel
	cl_uint samplesPerRunAsCLUint = samplesPerRun;
	size_t step_groupSize = 256;
	size_t step_globalSize = (samplesPerRun + step_groupSize - 1) & ~(step_groupSize - 1);
	CL_CHECK_ERR("clSetKernelArg", clSetKernelArg(k_dft_real2cpx, 0, sizeof(cl_mem), &v_samples));
	CL_CHECK_ERR("clSetKernelArg", clSetKernelArg(k_dft_real2cpx, 1, sizeof(cl_mem), &v_result));
	CL_CHECK_ERR("clSetKernelArg", clSetKernelArg(k_dft_real2cpx, 2, sizeof(cl_uint), &samplesPerRunAsCLUint));
	CL_CHECK_ERR("clEnqueueNDRangeKernel", clEnqueueNDRangeKernel(command_queue,
		k_dft_real2cpx,
		1,
		NULL,
		&step_globalSize,
		&step_groupSize,
		1,
		&upload_unmap_evt,
		&kernel_evt
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
		&kernel_evt,
		&download_map_evt,
		&err);
	CL_CHECK_ERR("clEnqueueMapBuffer", err);

	for (int i = 0; i < samplesPerRun; i++)
		result[i] = cpx(output_buffer[i].x, output_buffer[i].y);

	CL_CHECK_ERR("clEnqueueUnmapMemObject", clEnqueueUnmapMemObject(command_queue, v_result, output_buffer, 0, NULL, NULL));

	printStatsAndReleaseEvents(upload_unmap_evt, kernel_evt, download_map_evt);

	return result;
}

template <typename T>
void cl_naive_dft<T>::printStatsAndReleaseEvents(cl_event upload_unmap_evt, cl_event kernel_evt, cl_event download_map_evt)
{
	const float upload_secs = clhEventWaitAndGetDuration(upload_unmap_evt);
	const float upload_memSizeKiB = samplesMemSize / 1.024e3;

	const float kernel_secs = clhEventWaitAndGetDuration(kernel_evt);
	const float kernel_memSizeKiB = (samplesPerRun*samplesMemSize + resultMemSize) / 1.024e3;

	const float download_secs = clhEventWaitAndGetDuration(download_map_evt);
	const float download_memSizeKiB = resultMemSize / 1.024e3;

	fprintf(stderr, "%s [N=%d]:\n",
		cl_naive_dft_algoName<T>(), samplesPerRun);

	fprintf(stderr, " upload %g ms, %g KiB/s\n",
		upload_secs * 1e3, upload_memSizeKiB / upload_secs);
	fprintf(stderr, " kernel %g ms, %g KiB/s, %g samples/s\n",
		kernel_secs * 1e3, kernel_memSizeKiB / kernel_secs, samplesPerRun / kernel_secs);
	fprintf(stderr, " download %g ms, %g KiB/s\n",
		download_secs * 1e3, download_memSizeKiB / download_secs);

	CL_CHECK_ERR("clReleaseEvent", clReleaseEvent(upload_unmap_evt));
	CL_CHECK_ERR("clReleaseEvent", clReleaseEvent(kernel_evt));
	CL_CHECK_ERR("clReleaseEvent", clReleaseEvent(download_map_evt));
}

template <typename T>
cl_naive_dft<T>::~cl_naive_dft()
{
	CL_CHECK_ERR("clReleaseMemObject", clReleaseMemObject(v_samples));
	CL_CHECK_ERR("clReleaseMemObject", clReleaseMemObject(v_result));

	CL_CHECK_ERR("clReleaseKernel", clReleaseKernel(k_dft_cpx2cpx));
	CL_CHECK_ERR("clReleaseKernel", clReleaseKernel(k_dft_real2cpx));
	CL_CHECK_ERR("clReleaseProgram", clReleaseProgram(program));
}
