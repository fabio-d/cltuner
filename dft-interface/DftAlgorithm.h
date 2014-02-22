#ifndef DFTINTERFACE_DFTALGORITHM_H
#define DFTINTERFACE_DFTALGORITHM_H

#include <QObject>
#include <QVector>

class DftAlgorithm : public QObject
{
	Q_OBJECT

	public:
		virtual ~DftAlgorithm();

		virtual void analyzeSpectrum(const QVector<float> &audioSamples) = 0;

	protected:
		DftAlgorithm();

	signals:
		void spectrumAnalyzed(QVector<float> result);
};

#endif // DFTINTERFACE_DFTALGORITHM_H
