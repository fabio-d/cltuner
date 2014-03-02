#include "cl_base.h"
#include "cpx.h"
#include "pi_float.h"
#include "sizeconv.h"

#include <cstdio>
#include <cstdlib>

template <typename T> const char *cl_fft_algoName();

template <> const char *cl_fft_algoName<float>()
{ return "OpenCL non-recursive FFT (real-to-complex)"; }

template <> const char *cl_fft_algoName<cpx>()
{ return "OpenCL non-recursive FFT (complex-to-complex)"; }

template <typename T>
class cl_fft : public cl_base
{
	public:
		cl_fft(int platform_index, int device_index, int samplesPerRun);
		vector<cpx> run(const vector<T> &input);
		void printStatsAndReleaseEvents(cl_event upload_unmap_evt, cl_event *kernel_evts, cl_event download_map_evt);
		~cl_fft();

	private:
		size_t twiddleFactorsMemSize, samplesMemSize, tmpMemSize;

		struct launch_grid
		{
			size_t globalSize[2], groupSize[2];
		};
		vector<launch_grid> launches;

		cl_program program;
		cl_kernel k_fftstep_init, k_fftstep_cpx2cpx, k_fftstep_real2cpx;

		cl_mem v_twiddleFactors;
		cl_mem v_samples;
		cl_mem v_tmp1, v_tmp2;
};

template <typename T>
cl_fft<T>::cl_fft(int platform_index, int device_index, int samplesPerRun)
: cl_base(platform_index, device_index, samplesPerRun)
{
	program = clhBuildProgram(context, device, "dft-algorithms/cl_fft.cl");
	k_fftstep_init = clhCreateKernel(program, "fftstep_init");
	k_fftstep_cpx2cpx = clhCreateKernel(program, "fftstep_cpx2cpx");
	k_fftstep_real2cpx = clhCreateKernel(program, "fftstep_real2cpx");

	cl_image_format fmt;
	fmt.image_channel_order = cl_channelOrder<cpx>();
	fmt.image_channel_data_type = CL_FLOAT;

	// deve essere una una potenza di due
	const size_t maxGroupSize = atoi(getenv("GS_X") ?: "128");

	// Parametri di lancio dei kernel mtx_cpx2cpx e mtx_real2cpx
	// Griglia con una riga per ogni coppia di righe nella matrice
	launch_grid tmp;
	tmp.globalSize[0] = samplesPerRun / 2;
	tmp.globalSize[1] = 1;
	tmp.groupSize[0] = maxGroupSize;
	tmp.groupSize[1] = 1;

	while (tmp.globalSize[0] != 0)
	{
		if (tmp.groupSize[0] > tmp.globalSize[0])
			tmp.groupSize[0] = tmp.globalSize[0];
		launches.push_back(tmp);
		tmp.globalSize[0] /= 2;
		tmp.globalSize[1] *= 2;
	}

	cl_int err;
	size_t twiddleFactorsCount = samplesPerRun / 2;
	twiddleFactorsMemSize = twiddleFactorsCount * sizeof(cl_float2);
	size_t rows, cols;
	if (twiddleFactorsCount <= 4096)
	{
		cols = twiddleFactorsCount;
		rows = 1;
	}
	else
	{
		cols = 4096;
		rows = (twiddleFactorsCount + 4096 - 1) / 4096;
	}
	v_twiddleFactors = clCreateImage2D(context, CL_MEM_READ_WRITE, &fmt,
		cols, rows, 0, NULL, &err);
	CL_CHECK_ERR("clCreateBuffer", err);

	fprintf(stderr, "Memoria occupata dai twiddle factors [N=%d]: %g KiB\n\n",
		samplesPerRun, twiddleFactorsMemSize / 1.024e3);

	samplesMemSize = cl_deviceDataSize<T>(samplesPerRun);
	v_samples = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR, samplesMemSize, NULL, &err);
	CL_CHECK_ERR("clCreateBuffer", err);

	tmpMemSize = samplesPerRun * sizeof(cl_float2);
	v_tmp1 = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR, tmpMemSize, NULL, &err);
	CL_CHECK_ERR("clCreateBuffer", err);
	v_tmp2 = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR, tmpMemSize, NULL, &err);
	CL_CHECK_ERR("clCreateBuffer", err);

	cl_event init_evt;
	size_t initGroupSize = min(maxGroupSize, twiddleFactorsCount);
	CL_CHECK_ERR("clSetKernelArg", clSetKernelArg(k_fftstep_init, 0, sizeof(cl_mem), &v_twiddleFactors));
	CL_CHECK_ERR("clEnqueueNDRangeKernel", clEnqueueNDRangeKernel(command_queue,
		k_fftstep_init,
		1,
		NULL,
		&twiddleFactorsCount,
		&initGroupSize,
		0,
		NULL,
		&init_evt
	));

	const float init_secs = clhEventWaitAndGetDuration(init_evt);
	const float init_memSizeMiB = twiddleFactorsMemSize / SIZECONV_MB;

	fprintf(stderr, "%s [N=%d, GS=%d]:\n",
		"OpenCL FFT twiddle factors initialization", samplesPerRun, maxGroupSize);

	fprintf(stderr, " kernel %g ms, %g MiB/s, %g valori/s\n",
		init_secs * 1e3, init_memSizeMiB / init_secs, samplesPerRun / 2 / init_secs);

	CL_CHECK_ERR("clReleaseEvent", clReleaseEvent(init_evt));
}

template <>
vector<cpx> cl_fft<cpx>::run(const vector<cpx> &input)
{
	cl_event upload_unmap_evt, download_map_evt, *kernel_evts = new cl_event[launches.size()];
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

	// Lanci del kernel
	cl_event prev_evt = upload_unmap_evt;
	for (unsigned int i = 0; i < launches.size(); i++)
	{
		CL_CHECK_ERR("clSetKernelArg", clSetKernelArg(k_fftstep_cpx2cpx, 0, sizeof(cl_mem), (i == 0) ? &v_samples : &v_tmp1));
		CL_CHECK_ERR("clSetKernelArg", clSetKernelArg(k_fftstep_cpx2cpx, 1, sizeof(cl_mem), &v_tmp2));
		CL_CHECK_ERR("clSetKernelArg", clSetKernelArg(k_fftstep_cpx2cpx, 2, sizeof(cl_mem), &v_twiddleFactors));
		CL_CHECK_ERR("clEnqueueNDRangeKernel", clEnqueueNDRangeKernel(command_queue,
			k_fftstep_cpx2cpx,
			2,
			NULL,
			launches[i].globalSize,
			launches[i].groupSize,
			1,
			&prev_evt,
			&kernel_evts[i]
		));

		prev_evt = kernel_evts[i];
		swap(v_tmp1, v_tmp2);
	}

	// Download
	vector<cpx> result(samplesPerRun);
	cl_float2 *output_buffer = (cl_float2*)clEnqueueMapBuffer(command_queue,
		v_tmp1,
		CL_TRUE,
		CL_MAP_READ,
		0,
		tmpMemSize,
		1,
		&prev_evt,
		&download_map_evt,
		&err);
	CL_CHECK_ERR("clEnqueueMapBuffer", err);

	for (int i = 0; i < samplesPerRun; i++)
		result[i] = cpx(output_buffer[i].x, output_buffer[i].y);

	CL_CHECK_ERR("clEnqueueUnmapMemObject", clEnqueueUnmapMemObject(command_queue, v_tmp1, output_buffer, 0, NULL, NULL));

	printStatsAndReleaseEvents(upload_unmap_evt, kernel_evts, download_map_evt);

	delete kernel_evts;

	return result;
}

template <>
vector<cpx> cl_fft<float>::run(const vector<float> &input)
{
	cl_event upload_unmap_evt, download_map_evt, *kernel_evts = new cl_event[launches.size()];
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

	// Lanci del kernel
	cl_event prev_evt = upload_unmap_evt;
	for (unsigned int i = 0; i < launches.size(); i++)
	{
		// Solo il primo step ha input reali
		cl_kernel kernel = (i == 0) ? k_fftstep_real2cpx : k_fftstep_cpx2cpx;

		CL_CHECK_ERR("clSetKernelArg", clSetKernelArg(kernel, 0, sizeof(cl_mem), (i == 0) ? &v_samples : &v_tmp1));
		CL_CHECK_ERR("clSetKernelArg", clSetKernelArg(kernel, 1, sizeof(cl_mem), &v_tmp2));
		CL_CHECK_ERR("clSetKernelArg", clSetKernelArg(kernel, 2, sizeof(cl_mem), &v_twiddleFactors));
		CL_CHECK_ERR("clEnqueueNDRangeKernel", clEnqueueNDRangeKernel(command_queue,
			kernel,
			2,
			NULL,
			launches[i].globalSize,
			launches[i].groupSize,
			1,
			&prev_evt,
			&kernel_evts[i]
		));

		prev_evt = kernel_evts[i];
		swap(v_tmp1, v_tmp2);
	}

	// Download
	vector<cpx> result(samplesPerRun);
	cl_float2 *output_buffer = (cl_float2*)clEnqueueMapBuffer(command_queue,
		v_tmp1,
		CL_TRUE,
		CL_MAP_READ,
		0,
		tmpMemSize,
		1,
		&prev_evt,
		&download_map_evt,
		&err);
	CL_CHECK_ERR("clEnqueueMapBuffer", err);

	for (int i = 0; i < samplesPerRun; i++)
		result[i] = cpx(output_buffer[i].x, output_buffer[i].y);

	CL_CHECK_ERR("clEnqueueUnmapMemObject", clEnqueueUnmapMemObject(command_queue, v_tmp1, output_buffer, 0, NULL, NULL));

	printStatsAndReleaseEvents(upload_unmap_evt, kernel_evts, download_map_evt);

	delete kernel_evts;

	return result;
}

template <typename T>
void cl_fft<T>::printStatsAndReleaseEvents(cl_event upload_unmap_evt, cl_event *kernel_evts, cl_event download_map_evt)
{
	const float upload_secs = clhEventWaitAndGetDuration(upload_unmap_evt);
	const float upload_memSizeMiB = samplesMemSize / SIZECONV_MB;

	const float step_memSizeMiB = (samplesMemSize + tmpMemSize) / SIZECONV_MB;

	const float download_secs = clhEventWaitAndGetDuration(download_map_evt);
	const float download_memSizeMiB = tmpMemSize / SIZECONV_MB;

	fprintf(stderr, "%s [N=%d]:\n",
		cl_fft_algoName<T>(), samplesPerRun);

	fprintf(stderr, " upload %g ms, %g MiB/s\n",
		upload_secs * 1e3, upload_memSizeMiB / upload_secs);

	float kernel_secs = 0;
	for (unsigned int i = 0; i < launches.size(); i++)
	{
		const float step_secs = clhEventWaitAndGetDuration(kernel_evts[i]);
		fprintf(stderr, " step%d [GRID=%dx%d GS=%dx%d] %g ms, %g MiB/s\n", i,
			(int)(launches[i].globalSize[0] / launches[i].groupSize[0]),
			(int)(launches[i].globalSize[1] / launches[i].groupSize[1]),
			(int)launches[i].groupSize[0], (int)launches[i].groupSize[1],
			step_secs * 1e3, step_memSizeMiB / step_secs, samplesPerRun / kernel_secs);
		kernel_secs += step_secs;
	}
	fprintf(stderr, " TOTAL %g ms, %g MiB/s, %g Ksamples/s\n",
		kernel_secs * 1e3, launches.size() * step_memSizeMiB / kernel_secs, 1e-3 * samplesPerRun / kernel_secs);
	fprintf(stderr, " download %g ms, %g MiB/s\n",
		download_secs * 1e3, download_memSizeMiB / download_secs);

	if (getenv("PRINT_KERNEL_EXECUTION_TIME"))
		printf("%g\n", kernel_secs * 1e3);

	CL_CHECK_ERR("clReleaseEvent", clReleaseEvent(upload_unmap_evt));
	CL_CHECK_ERR("clReleaseEvent", clReleaseEvent(download_map_evt));

	for (unsigned int i = 0; i < launches.size(); i++)
		CL_CHECK_ERR("clReleaseEvent", clReleaseEvent(kernel_evts[i]));
}

template <typename T>
cl_fft<T>::~cl_fft()
{
	CL_CHECK_ERR("clReleaseMemObject", clReleaseMemObject(v_samples));
	CL_CHECK_ERR("clReleaseMemObject", clReleaseMemObject(v_tmp1));
	CL_CHECK_ERR("clReleaseMemObject", clReleaseMemObject(v_tmp2));

	CL_CHECK_ERR("clReleaseKernel", clReleaseKernel(k_fftstep_init));
	CL_CHECK_ERR("clReleaseKernel", clReleaseKernel(k_fftstep_cpx2cpx));
	CL_CHECK_ERR("clReleaseKernel", clReleaseKernel(k_fftstep_real2cpx));
	CL_CHECK_ERR("clReleaseProgram", clReleaseProgram(program));
}
