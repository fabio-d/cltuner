#include "SpectrumAnalyzer.h"
#include "dft-algorithms/cl_fft.cpp"

#include <QDebug>

#include <cmath>
#include <numeric>

static const size_t presumGroupSize = 256;

class SpectrumAnalyzerRangeData
{
	public:
		SpectrumAnalyzerRangeData(cl_platform_id platform, cl_device_id device, cl_context context,
				cl_command_queue command_queue, const SpectrumAnalyzerRange &range, cl_kernel circsum);
		~SpectrumAnalyzerRangeData();

		QVector<float> runFFT(cl_mem samplesHistory, cl_int historyOffset, cl_int historyLength);

		const int firstKey, lastKey;

	private:
		cl_fft<float> algorithm;
		cl_mem samples;

		cl_command_queue command_queue;
		cl_kernel circsum;

		size_t windowSize;
		cl_int presumWindows;
};

SpectrumAnalyzerRangeData::SpectrumAnalyzerRangeData(cl_platform_id platform, cl_device_id device,
	cl_context context, cl_command_queue command_queue, const SpectrumAnalyzerRange &range, cl_kernel circsum)
: firstKey(range.firstKey), lastKey(range.lastKey),
  algorithm(platform, device, context, command_queue, range.windowSize),
  command_queue(command_queue), circsum(circsum), windowSize(range.windowSize),
  presumWindows(range.presumWindows)
{
	cl_int err;
	samples = clCreateBuffer(context, CL_MEM_READ_ONLY, windowSize * sizeof(cl_float), NULL, &err);
	CL_CHECK_ERR("clCreateBuffer", err);
}

SpectrumAnalyzerRangeData::~SpectrumAnalyzerRangeData()
{
	CL_CHECK_ERR("clReleaseMemObject", clReleaseMemObject(samples));
}

QVector<float> SpectrumAnalyzerRangeData::runFFT(cl_mem samplesHistory, cl_int historyOffset, cl_int historyLength)
{
	cl_event presum_evt, algo_evt;

	CL_CHECK_ERR("clSetKernelArg", clSetKernelArg(circsum, 0, sizeof(cl_mem), &samples));
	CL_CHECK_ERR("clSetKernelArg", clSetKernelArg(circsum, 1, sizeof(cl_mem), &samplesHistory));
	CL_CHECK_ERR("clSetKernelArg", clSetKernelArg(circsum, 2, sizeof(cl_int), &historyOffset));
	CL_CHECK_ERR("clSetKernelArg", clSetKernelArg(circsum, 3, sizeof(cl_int), &historyLength));
	CL_CHECK_ERR("clSetKernelArg", clSetKernelArg(circsum, 4, sizeof(cl_int), &presumWindows));
	CL_CHECK_ERR("clEnqueueNDRangeKernel", clEnqueueNDRangeKernel(command_queue,
		circsum,
		1,
		NULL,
		&windowSize,
		&presumGroupSize,
		0,
		NULL,
		&presum_evt
	));

	clEnqueueWaitForEvents(command_queue, 1, &presum_evt);

	cl_mem output = algorithm.run(samples, &algo_evt);

	QVector<float> risultato(windowSize);

	cl_int err;
	cl_float2 *output_buffer = (cl_float2*)clEnqueueMapBuffer(command_queue,
		output,
		CL_TRUE,
		CL_MAP_READ,
		0,
		windowSize * sizeof(cl_float2),
		1,
		&algo_evt,
		NULL,
		&err);
	CL_CHECK_ERR("clEnqueueMapBuffer", err);

	for (int i = 0; i < windowSize; i++)
		risultato[i] = abs(cpx(output_buffer[i].x, output_buffer[i].y));

	CL_CHECK_ERR("clEnqueueUnmapMemObject", clEnqueueUnmapMemObject(command_queue, output, output_buffer, 0, NULL, NULL));

	CL_CHECK_ERR("clReleaseEvent", clReleaseEvent(presum_evt));
	CL_CHECK_ERR("clReleaseEvent", clReleaseEvent(algo_evt));

	return risultato;
}

SpectrumAnalyzer::SpectrumAnalyzer(LiveAudioInput *audioIn, cl_platform_id platform, cl_device_id device,
	const SpectrumAnalyzerRange *ranges, int numRanges, QObject *parent)
: QObject(parent)
{
	audioIn->setParent(this);

	m_context = clhCreateContextSingleDevice(platform, device);
	m_command_queue = clhCreateCommandQueue(m_context, device, true /* profiling abilitato */);
	clhEmptyNvidiaCache();

	m_program = clhBuildProgram(m_context, device, "tuner/SpectrumAnalyzer.cl");
	m_circsum = clhCreateKernel(m_program, "circsum");

	m_sampleHistoryLength = m_sampleHistoryOffset = 0;
	for (int i = 0; i < numRanges; i++)
	{
		m_ranges.append(new SpectrumAnalyzerRangeData(platform, device, m_context, m_command_queue, ranges[i], m_circsum));
		m_sampleHistoryLength = qMax(m_sampleHistoryLength, ranges[i].windowSize * ranges[i].presumWindows);
	}

	cl_int err;
	m_sampleHistoryCircQueue = clCreateBuffer(m_context, CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR, m_sampleHistoryLength * sizeof(cl_float), NULL, &err);
	CL_CHECK_ERR("clCreateBuffer", err);

	m_sampleRate = audioIn->sampleRate();

	connect(audioIn, SIGNAL(newChunkAvailable(QVector<qint16>)), this, SLOT(slotAudioChunkAvailable(QVector<qint16>)));
}

SpectrumAnalyzer::~SpectrumAnalyzer()
{
	qDeleteAll(m_ranges);

	CL_CHECK_ERR("clReleaseKernel", clReleaseKernel(m_circsum));
	CL_CHECK_ERR("clReleaseProgram", clReleaseProgram(m_program));
	CL_CHECK_ERR("clReleaseMemObject", clReleaseMemObject(m_sampleHistoryCircQueue));
	CL_CHECK_ERR("clReleaseCommandQueue", clReleaseCommandQueue(m_command_queue));
	CL_CHECK_ERR("clReleaseContext", clReleaseContext(m_context));
}

int SpectrumAnalyzer::sampleRate() const
{
	return m_sampleRate;
}

void SpectrumAnalyzer::slotAudioChunkAvailable(const QVector<qint16> &data)
{
	// Assume che il numero di dati da aggiungere sia un sottomultiplo di m_sampleHistoryLength

	cl_int err;
	cl_float *buffer = (cl_float*)clEnqueueMapBuffer(m_command_queue,
		m_sampleHistoryCircQueue,
		CL_TRUE,
		CL_MAP_WRITE,
		m_sampleHistoryOffset * sizeof(cl_float),
		data.size() * sizeof(cl_float),
		0,
		NULL,
		NULL,
		&err);
	CL_CHECK_ERR("clEnqueueMapBuffer", err);

	const float gain = 1.0 / 5000;
	for (int i = 0; i < data.size(); i++)
		buffer[i] = data[i] * gain;

	CL_CHECK_ERR("clEnqueueUnmapMemObject", clEnqueueUnmapMemObject(m_command_queue, m_sampleHistoryCircQueue, buffer, 0, NULL, NULL));

	if ((m_sampleHistoryOffset += data.size()) == m_sampleHistoryLength)
		m_sampleHistoryOffset = 0;

	QSet<int> pressedKeys;

	for (int i = 0; i < m_ranges.size(); i++)
	{
		const QVector<float> ft = m_ranges[i]->runFFT(m_sampleHistoryCircQueue, m_sampleHistoryOffset, m_sampleHistoryLength);
		pressedKeys += analyzeFT(i, ft);
	}

	emit pressedKeysAvailable(pressedKeys);
}

static bool isPeak(const float *elem, float threshold)
{
	return threshold < elem[0] && elem[-1] < elem[0] && elem[+1] < elem[0];
}

QSet<int> SpectrumAnalyzer::analyzeFT(int range, const QVector<float> &data)
{
	const float average = std::accumulate(data.begin(), data.end(), .0f) / data.size();
	const float threshold = qMax(50 * average, .1f * data.size());

	QSet<int> pressedKeys;

	for (int i = 1; i < data.size() / 2; ++i)
	{
		if (isPeak(&data[i], threshold) && !isPeak(&data[i/2], threshold))
		{
			const float freq = float(m_sampleRate) * i / data.size();
			const int nTasto = qRound(12 * log2(freq / 440) + 48);

			if (nTasto >= m_ranges[range]->firstKey && nTasto <= m_ranges[range]->lastKey)
				pressedKeys.insert(nTasto);
		}
	}

	emit spectrumAvailable(range, data, threshold);
	return pressedKeys;
}
