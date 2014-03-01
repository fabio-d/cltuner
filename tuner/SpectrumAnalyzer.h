#ifndef TUNER_SPECTRUMANALYZER_H
#define TUNER_SPECTRUMANALYZER_H

#include "LiveAudioInput.h"
#include "dft-interface/DftAlgorithm.h"

#include <QLinkedList>
#include <QObject>
#include <QSet>
#include <QVector>

/* Classe che prende dati in input e li forwarda all'algoritmo DFT */
class SpectrumAnalyzer : public QObject
{
	Q_OBJECT

	public:
		// prende l'ownership di audioIn e algorithm
		SpectrumAnalyzer(LiveAudioInput *audioIn, DftAlgorithm *algorithm, int windowHistoryLength = 1, QObject *parent = 0);

	signals:
		void spectrumAvailable(const QVector<float> &data, float threshold);
		void pressedKeysAvailable(const QSet<int> &pressedKeys);

	private slots:
		void slotAudioChunkAvailable(const QVector<qint16> &data);
		void slotSpectrumAnalyzed(const QVector<float> &data);

	private:
		DftAlgorithm *m_algorithm;

		// Window presum -- coda delle window più recenti
		int m_windowHistoryLength;
		QLinkedList< QVector<qint16> > m_sampleHistory;
};

#endif // TUNER_SPECTRUMANALYZER_H
