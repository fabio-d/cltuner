#ifndef TUNER_MAINWINDOW_H
#define TUNER_MAINWINDOW_H

#include "LiveAudioInput.h"
#include "SpectrumAnalyzer.h"
#include "SpectrumWidget.h"
#include "clhelpers/cl.h"

#include <QMainWindow>

class Ui_MainWindow;

class MainWindow : public QMainWindow
{
	Q_OBJECT

	public:
		MainWindow(LiveAudioInput *audioIn, cl_platform_id platform, cl_device_id device, QWidget *parent = 0);

	private slots:
		void slotSpectrumAvailable(int rangeNum, const QVector<float> &data, float threshold);
		void slotZoomIn();
		void slotZoomOut();
		void updateHighlightedKey();

	private:
		Ui_MainWindow *m_ui;
		SpectrumAnalyzer *m_analyzer;
		QList<SpectrumWidget*> m_spectrumWidgets;
};

#endif // TUNER_MAINWINDOW_H
