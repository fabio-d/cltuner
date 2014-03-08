#ifndef TUNER_SPECTRUMANALYZER_H
#define TUNER_SPECTRUMANALYZER_H

#include "LiveAudioInput.h"
#include "clhelpers/cl.h"

#include <QObject>
#include <QSet>
#include <QVector>

struct SpectrumAnalyzerRange
{
	int firstKey, lastKey;
	int windowSize, presumWindows; // presumWindows = # di finestre da sommare (minimo 1)
};

struct SpectrumAnalyzerRangeData;

/* Classe che prende dati in input e li forwarda all'algoritmo DFT */
class SpectrumAnalyzer : public QObject
{
	Q_OBJECT

	public:
		// prende l'ownership di audioIn e algorithm
		SpectrumAnalyzer(LiveAudioInput *audioIn, cl_platform_id platform, cl_device_id device,
			const SpectrumAnalyzerRange *ranges, int numRanges, QObject *parent = 0);
		~SpectrumAnalyzer();

		int sampleRate() const;

	signals:
		void spectrumAvailable(int rangeNum, const QVector<float> &data, float threshold);
		void pressedKeysAvailable(const QSet<int> &pressedKeys);

	private slots:
		void slotAudioChunkAvailable(const QVector<qint16> &data);

	private:
		QSet<int> analyzeFT(int range, const QVector<float> &data);

		QList<SpectrumAnalyzerRangeData*> m_ranges;
		int m_sampleRate;

		cl_context m_context;
		cl_command_queue m_command_queue;

		cl_program m_program;
		cl_kernel m_circsum;

		cl_int m_sampleHistoryLength, m_sampleHistoryOffset;
		cl_mem m_sampleHistoryCircQueue;
};

#endif // TUNER_SPECTRUMANALYZER_H
