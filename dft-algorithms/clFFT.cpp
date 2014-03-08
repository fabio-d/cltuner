#include "cl_base.h"
#include "cpx.h"
#include "pi_float.h"
#include "sizeconv.h"

#include <clFFT.h>

#include <cstdio>
#include <cstdlib>

template <typename T> const char *clFFT_algoName();

template <> const char *clFFT_algoName<float>()
{ return "OpenCL clFFT (real-to-complex)"; }

template <> const char *clFFT_algoName<cpx>()
{ return "OpenCL clFFT (complex-to-complex)"; }

void CLFFT_CHECK_ERR(const char *prefix, clfftStatus status)
{
	const char *errtext;

	switch (status)
	{
		case CLFFT_BUGCHECK:
			errtext = "clFFT bugcheck";
			break;
		case CLFFT_NOTIMPLEMENTED:
			errtext = "clFFT not implemented";
			break;
		case CLFFT_TRANSPOSED_NOTIMPLEMENTED:
			errtext = "clFFT transposed not implemented";
			break;
		case CLFFT_FILE_NOT_FOUND:
			errtext = "clFFT file not found";
			break;
		case CLFFT_FILE_CREATE_FAILURE:
			errtext = "clFFT file create failure";
			break;
		case CLFFT_VERSION_MISMATCH:
			errtext = "clFFT version mismatch";
			break;
		case CLFFT_INVALID_PLAN:
			errtext = "clFFT invalid plan";
			break;
		case CLFFT_DEVICE_NO_DOUBLE:
			errtext = "clFFT no double precision on this device";
			break;
		case CLFFT_DEVICE_MISMATCH:
			errtext = "clFFT device mismatch";
			break;
		default:
			CL_CHECK_ERR(prefix, (cl_int)status);
			return;
	}

	fprintf(stderr, "%s: %s\n", prefix, errtext);
	exit(EXIT_FAILURE);
}

// clfft input layout
template <typename HostType> static clfftLayout clfft_inputLayout();
template <> clfftLayout clfft_inputLayout<float>() { return CLFFT_REAL; }
template <> clfftLayout clfft_inputLayout<cpx>() { return CLFFT_COMPLEX_INTERLEAVED; }

// clfft output layout
template <typename HostType> static clfftLayout clfft_outputLayout();
template <> clfftLayout clfft_outputLayout<float>() { return CLFFT_HERMITIAN_INTERLEAVED; }
template <> clfftLayout clfft_outputLayout<cpx>() { return CLFFT_COMPLEX_INTERLEAVED; }

template <typename T>
class clFFT : public cl_base
{
	public:
		clFFT(int platform_index, int device_index, int samplesPerRun);
		vector<cpx> run(const vector<T> &input);
		~clFFT();

	private:
		void printStatsAndReleaseEvents(cl_event upload_unmap_evt, cl_event clfft_start_evt, cl_event clfft_end_evt, cl_event download_map_evt);

		size_t samplesMemSize, resultMemSize;

		clfftPlanHandle clfft_plan;

		cl_mem v_samples;
		cl_mem v_result;
};

template <typename T>
clFFT<T>::clFFT(int platform_index, int device_index, int samplesPerRun)
: cl_base(platform_index, device_index, samplesPerRun)
{
	clfftSetupData fftSetup;
	CLFFT_CHECK_ERR("clfftInitSetupData", clfftInitSetupData(&fftSetup));

	if (getenv("CLFFT_DUMP_KERNELS"))
		fftSetup.debugFlags |= CLFFT_DUMP_PROGRAMS; // memorizza i kernel generati in working dir

	CLFFT_CHECK_ERR("clfftSetup", clfftSetup(&fftSetup));

	const size_t samplesPerRunAsSizeT = samplesPerRun;
	CLFFT_CHECK_ERR("clfftCreateDefaultPlan", clfftCreateDefaultPlan(&clfft_plan, context, CLFFT_1D, &samplesPerRunAsSizeT));
	CLFFT_CHECK_ERR("clfftCreateDefaultPlan", clfftSetPlanPrecision(clfft_plan, CLFFT_SINGLE));
	CLFFT_CHECK_ERR("clfftCreateDefaultPlan", clfftSetLayout(clfft_plan, clfft_inputLayout<T>(), clfft_outputLayout<T>()));
	CLFFT_CHECK_ERR("clfftCreateDefaultPlan", clfftSetResultLocation(clfft_plan, CLFFT_OUTOFPLACE));
	CLFFT_CHECK_ERR("clfftBakePlan", clfftBakePlan(clfft_plan, 1, &command_queue, NULL, NULL));

	cl_int err;
	samplesMemSize = cl_deviceDataSize<T>(samplesPerRun);
	v_samples = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR, samplesMemSize, NULL, &err);
	CL_CHECK_ERR("clCreateBuffer", err);

	resultMemSize = samplesPerRun * sizeof(cl_float2);
	v_result = clCreateBuffer(context, CL_MEM_WRITE_ONLY | CL_MEM_ALLOC_HOST_PTR, resultMemSize, NULL, &err);
	CL_CHECK_ERR("clCreateBuffer", err);
}

template <>
vector<cpx> clFFT<cpx>::run(const vector<cpx> &input)
{
	cl_event upload_unmap_evt, clfft_start_evt, clfft_end_evt, download_map_evt;
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

	CL_CHECK_ERR("clEnqueueMarker", clEnqueueMarker(command_queue, &clfft_start_evt));

	// Esecuzione dei kernel (gestiti da clFFT)
	CLFFT_CHECK_ERR("clfftEnqueueTransform", clfftEnqueueTransform(clfft_plan,
		CLFFT_FORWARD,
		1,
		&command_queue,
		1,
		&clfft_start_evt,
		&clfft_end_evt,
		&v_samples,
		&v_result,
		NULL));

	// Download
	vector<cpx> result(samplesPerRun);
	cl_float2 *output_buffer = (cl_float2*)clEnqueueMapBuffer(command_queue,
		v_result,
		CL_TRUE,
		CL_MAP_READ,
		0,
		resultMemSize,
		1,
		&clfft_end_evt,
		&download_map_evt,
		&err);
	CL_CHECK_ERR("clEnqueueMapBuffer", err);

	for (int i = 0; i < samplesPerRun; i++)
		result[i] = cpx(output_buffer[i].x, output_buffer[i].y);

	CL_CHECK_ERR("clEnqueueUnmapMemObject", clEnqueueUnmapMemObject(command_queue, v_result, output_buffer, 0, NULL, NULL));

	printStatsAndReleaseEvents(upload_unmap_evt, clfft_start_evt, clfft_end_evt, download_map_evt);

	return result;
}

template <>
vector<cpx> clFFT<float>::run(const vector<float> &input)
{
	cl_event upload_unmap_evt, clfft_start_evt, clfft_end_evt, download_map_evt;
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

	CL_CHECK_ERR("clEnqueueMarker", clEnqueueMarker(command_queue, &clfft_start_evt));

	// Esecuzione dei kernel (gestiti da clFFT)
	CLFFT_CHECK_ERR("clfftEnqueueTransform", clfftEnqueueTransform(clfft_plan,
		CLFFT_FORWARD,
		1,
		&command_queue,
		1,
		&clfft_start_evt,
		&clfft_end_evt,
		&v_samples,
		&v_result,
		NULL));

	// Download
	vector<cpx> result(samplesPerRun);
	cl_float2 *output_buffer = (cl_float2*)clEnqueueMapBuffer(command_queue,
		v_result,
		CL_TRUE,
		CL_MAP_READ,
		0,
		resultMemSize,
		1,
		&clfft_end_evt,
		&download_map_evt,
		&err);
	CL_CHECK_ERR("clEnqueueMapBuffer", err);

	for (int i = 0; i < samplesPerRun/2 + 1; i++)
	{
		result[i] = cpx(output_buffer[i].x, output_buffer[i].y);
		if (i != 0 && i != samplesPerRun/2)
			result[samplesPerRun - i] = cpx(output_buffer[i].x, -output_buffer[i].y);
	}

	CL_CHECK_ERR("clEnqueueUnmapMemObject", clEnqueueUnmapMemObject(command_queue, v_result, output_buffer, 0, NULL, NULL));

	printStatsAndReleaseEvents(upload_unmap_evt, clfft_start_evt, clfft_end_evt, download_map_evt);

	return result;
}


template <typename T>
void clFFT<T>::printStatsAndReleaseEvents(cl_event upload_unmap_evt, cl_event clfft_start_evt, cl_event clfft_end_evt, cl_event download_map_evt)
{
	const float upload_secs = clhEventWaitAndGetDuration(upload_unmap_evt);
	const float upload_memSizeMiB = samplesMemSize / SIZECONV_MB;

	const float clfft_secs = clhEventWaitAndGetDifference(clfft_start_evt, clfft_end_evt);

	const float download_secs = clhEventWaitAndGetDuration(download_map_evt);
	const float download_memSizeMiB = resultMemSize / SIZECONV_MB;

	fprintf(stderr, "%s [N=%d]:\n",
		clFFT_algoName<T>(), samplesPerRun);

	fprintf(stderr, " upload %g ms, %g MiB/s\n",
		upload_secs * 1e3, upload_memSizeMiB / upload_secs);
	fprintf(stderr, " clFFT %g ms, %g Ksamples/s\n",
		clfft_secs * 1e3, 1e-3 * samplesPerRun / clfft_secs);
	fprintf(stderr, " download %g ms, %g MiB/s\n",
		download_secs * 1e3, download_memSizeMiB / download_secs);

	if (getenv("PRINT_KERNEL_EXECUTION_TIME"))
		printf("%g\n", clfft_secs * 1e3);

	CL_CHECK_ERR("clReleaseEvent", clReleaseEvent(upload_unmap_evt));
	CL_CHECK_ERR("clReleaseEvent", clReleaseEvent(clfft_start_evt));
	CL_CHECK_ERR("clReleaseEvent", clReleaseEvent(clfft_end_evt));
	CL_CHECK_ERR("clReleaseEvent", clReleaseEvent(download_map_evt));
}

template <typename T>
clFFT<T>::~clFFT()
{
	CL_CHECK_ERR("clReleaseMemObject", clReleaseMemObject(v_samples));
	CL_CHECK_ERR("clReleaseMemObject", clReleaseMemObject(v_result));

	CLFFT_CHECK_ERR("clfftDestroyPlan", clfftDestroyPlan(&clfft_plan));
	CLFFT_CHECK_ERR("clfftTeardown", clfftTeardown());
}
