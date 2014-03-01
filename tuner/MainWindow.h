#ifndef TUNER_MAINWINDOW_H
#define TUNER_MAINWINDOW_H

#include "LiveAudioInput.h"
#include "SpectrumWidget.h"
#include "dft-interface/DftAlgorithm.h"

#include <QMainWindow>

class Ui_MainWindow;

class MainWindow : public QMainWindow
{
	Q_OBJECT

	public:
		MainWindow(LiveAudioInput *audioIn, DftAlgorithm *algorithm, QWidget *parent = 0);

	private:
		Ui_MainWindow *ui;
};

#endif // TUNER_MAINWINDOW_H