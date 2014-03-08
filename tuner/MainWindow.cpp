#include "MainWindow.h"
#include "SpectrumAnalyzer.h"

#include "ui_MainWindow.h"

#define NUM_RANGES (sizeof(ranges) / sizeof(ranges[0]))
static SpectrumAnalyzerRange ranges[] =
{
	{ 0, 44, 4096, 2 },
	{ 45, 87, 4096, 2 }
};

MainWindow::MainWindow(LiveAudioInput *audioIn, cl_platform_id platform, cl_device_id device, QWidget *parent)
: QMainWindow(parent), m_ui(new Ui_MainWindow())
{
	m_ui->setupUi(this);

	setAttribute(Qt::WA_DeleteOnClose);

	for (int i = 0; i < NUM_RANGES; i++)
	{
		SpectrumWidget *sw = new SpectrumWidget(this);
		m_ui->spectrumTabs->addTab(sw, QString("Range %0-%1 (L%2 P%3)")
			.arg(ranges[i].firstKey).arg(ranges[i].lastKey)
			.arg(ranges[i].windowSize).arg(ranges[i].presumWindows));
		m_spectrumWidgets.append(sw);

		connect(sw, SIGNAL(highlightedKeyChanged()),
			this, SLOT(updateHighlightedKey()) );
	}

	m_analyzer = new SpectrumAnalyzer(audioIn, platform, device, ranges, NUM_RANGES, this);

	connect(m_analyzer, SIGNAL(pressedKeysAvailable(QSet<int>)),
		m_ui->tunesWidget, SLOT(setPressedKeys(QSet<int>)) );
	connect(m_ui->spectrumTabs, SIGNAL(currentChanged(int)),
		this, SLOT(updateHighlightedKey()) );
	connect(m_ui->actionZoomIn, SIGNAL(activated()), this, SLOT(slotZoomIn()));
	connect(m_ui->actionZoomOut, SIGNAL(activated()), this, SLOT(slotZoomOut()));
	connect(m_analyzer, SIGNAL(spectrumAvailable(int, QVector<float>, float)),
		this, SLOT(slotSpectrumAvailable(int, QVector<float>, float)) );

	resize(800, 600);
}

void MainWindow::slotSpectrumAvailable(int rangeNum, const QVector<float> &data, float threshold)
{
	m_spectrumWidgets[rangeNum]->setData(data, threshold, m_analyzer->sampleRate());
}

void MainWindow::slotZoomIn()
{
	m_spectrumWidgets[m_ui->spectrumTabs->currentIndex()]->zoomIn();
}

void MainWindow::slotZoomOut()
{
	m_spectrumWidgets[m_ui->spectrumTabs->currentIndex()]->zoomOut();
}

void MainWindow::updateHighlightedKey()
{
	int newKey = m_spectrumWidgets[m_ui->spectrumTabs->currentIndex()]->highlightedKey();
	m_ui->tunesWidget->setHighlightedKey(newKey);
}
