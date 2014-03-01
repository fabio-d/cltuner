#include "SpectrumAnalyzer.h"

#include <cmath>
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

static bool isPeak(const float *elem, float threshold)
{
	return threshold < elem[0] && elem[-1] < elem[0] && elem[+1] < elem[0];
}

void SpectrumAnalyzer::slotSpectrumAnalyzed(const QVector<float> &data)
{
	const float average = std::accumulate(data.begin(), data.end(), .0f) / data.size();
	const float threshold = qMax(50 * average, .1f * data.size());

	QSet<int> pressedKeys;

	for (int i = 1; i < data.size() / 2; ++i)
	{
		if (isPeak(&data[i], threshold) && !isPeak(&data[i/2], threshold))
		{
			const float freq = 48000.0 * i / data.size();
			const int nTasto = qRound(12 * log2(freq / 440) + 48);

			if (nTasto >= 0 && nTasto < 88)
				pressedKeys.insert(nTasto);
		}
	}

	emit spectrumAvailable(data, threshold);
	emit pressedKeysAvailable(pressedKeys);
}
