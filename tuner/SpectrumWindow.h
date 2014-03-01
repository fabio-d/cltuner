#ifndef TUNER_SPECTRUMWINDOW_H
#define TUNER_SPECTRUMWINDOW_H

#include "LiveAudioInput.h"
#include "dft-interface/DftAlgorithm.h"

#include <QLinkedList>
#include <QMainWindow>
#include <QVector>

class SpectrumWindow : public QMainWindow
{
	Q_OBJECT

	public:
		explicit SpectrumWindow(LiveAudioInput *audioIn, DftAlgorithm *algorithm, QWidget *parent = 0);

	public slots:
		void setData(QVector<float> newData);

	protected:
		void paintEvent(QPaintEvent *pe);

	private slots:
		void slotAudioChunkAvailable(const QVector<qint16> &data);

	private:
		LiveAudioInput *audioIn;
		DftAlgorithm *algorithm;

		QList< QVector<qint16> > sampleWindows;
		QVector<float> data;
};

#endif // TUNER_SPECTRUMWINDOW_H
