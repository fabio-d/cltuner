#include "MainWindow.h"
#include "SpectrumAnalyzer.h"

#include "ui_MainWindow.h"

MainWindow::MainWindow(LiveAudioInput *audioIn, DftAlgorithm *algorithm, QWidget *parent)
: QMainWindow(parent), ui(new Ui_MainWindow())
{
	ui->setupUi(this);

	setAttribute(Qt::WA_DeleteOnClose);

	SpectrumAnalyzer *analyzer = new SpectrumAnalyzer(audioIn, algorithm, 3 /* TODO */, this);

	connect(ui->actionZoomIn, SIGNAL(activated()), ui->spectrumWidget, SLOT(zoomIn()));
	connect(ui->actionZoomOut, SIGNAL(activated()), ui->spectrumWidget, SLOT(zoomOut()));

	connect(analyzer, SIGNAL(spectrumAvailable(QVector<float>, float)),
		ui->spectrumWidget, SLOT(setData(QVector<float>, float)) );
	connect(analyzer, SIGNAL(pressedKeysAvailable(QSet<int>)),
		ui->tunesWidget, SLOT(setPressedKeys(QSet<int>)) );
	connect(ui->spectrumWidget, SIGNAL(highlightedKeyAvailable(int)),
		ui->tunesWidget, SLOT(setHighlightedKey(int)) );

	resize(800, 600);
}
