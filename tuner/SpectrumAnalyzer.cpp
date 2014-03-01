#include "SpectrumAnalyzer.h"

#include <numeric>

SpectrumAnalyzer::SpectrumAnalyzer(LiveAudioInput *audioIn, DftAlgorithm *algorithm,
	int windowHistoryLength, QObject *parent)
: QObject(parent), m_algorithm(algorithm), m_windowHistoryLength(windowHistoryLength)
{
	audioIn->setParent(this);
	algorithm->setParent(this);

	connect(algorithm, SIGNAL(spectrumAnalyzed(QVector<float>)), this, SLOT(slotSpectrumAnalyzed(QVector<float>)));
	connect(audioIn, SIGNAL(newChunkAvailable(QVector<qint16>)), this, SLOT(slotAudioChunkAvailable(QVector<qint16>)));
}

void SpectrumAnalyzer::slotAudioChunkAvailable(const QVector<qint16> &data)
{
	// Window presum
	if (m_sampleHistory.size() == m_windowHistoryLength)
		m_sampleHistory.removeFirst();
	m_sampleHistory.append(data);

	const float gain = 1.0 / 5000;
	QVector<float> sum(data.size());
	foreach (const QVector<qint16> &window, m_sampleHistory)
	{
		for (int i = 0; i < sum.size(); i++)
			sum[i] += window[i] * gain;
	}

	m_algorithm->analyzeSpectrum(sum);
}

void SpectrumAnalyzer::slotSpectrumAnalyzed(const QVector<float> &data)
{
	const float average = std::accumulate(data.begin(), data.end(), .0f) / data.size();
	const float threshold = 50 * average;

	emit spectrumAvailable(data, threshold);
}
