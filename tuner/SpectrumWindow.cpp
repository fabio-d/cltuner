#include "SpectrumWindow.h"

#include <QDebug>
#include <QPaintEvent>
#include <QPainter>

#include <numeric>

SpectrumWindow::SpectrumWindow(LiveAudioInput *audioIn, DftAlgorithm *algorithm, QWidget *parent)
: QMainWindow(parent), audioIn(audioIn), algorithm(algorithm)
{
	audioIn->setParent(this);
	algorithm->setParent(this);
	setAttribute(Qt::WA_DeleteOnClose);

	connect(algorithm, SIGNAL(spectrumAnalyzed(QVector<float>)), this, SLOT(setData(QVector<float>)));
	connect(audioIn, SIGNAL(newChunkAvailable(QVector<qint16>)), this, SLOT(slotAudioChunkAvailable(QVector<qint16>)));

	resize(800, 600);
}

void SpectrumWindow::setData(QVector<float> newData)
{
	data = newData;
	update();
}

void SpectrumWindow::paintEvent(QPaintEvent *pe)
{
        QPainter p(this);
	p.fillRect(pe->rect(), Qt::white);
        p.setRenderHint(QPainter::Antialiasing);

	p.scale(width(), -height());
	p.translate(0, -1);

	if (data.size() < 2)
		return;

	const float average = std::accumulate(data.begin(), data.end(), .0f) / data.size();
	const float max = *std::max_element(data.begin(), data.end());

	const float theshold = 50 * average;

	p.scale(1, 1 / max);
	p.setPen(Qt::red);
	p.drawLine(0, theshold, 1, theshold);

	p.scale(qreal(1) / (data.size() - 1), 1);

	p.setPen(Qt::black);
	QPainterPath path;
	path.moveTo(0, data[0]);
	for (int i = 1; i < data.size(); ++i)
		path.lineTo(i, data[i]);
	p.drawPath(path);

	p.setPen(Qt::green);
	for (int i = 0; i < data.size(); ++i)
	{
		if (data[i] > theshold)
		{
			p.drawLine(i, 0, i, 100);
			qDebug() << i << data[i];
		}
	}
}

void SpectrumWindow::slotAudioChunkAvailable(const QVector<qint16> &data)
{
	// Window presum
	if (sampleWindows.size() == 3)
		sampleWindows.removeFirst();
	sampleWindows.append(data);

	const float gain = 1.0 / 5000;
	QVector<float> sum(data.size());
	foreach (const QVector<qint16> &window, sampleWindows)
	{
		for (int i = 0; i < sum.size(); i++)
			sum[i] += window[i] * gain;
	}

	algorithm->analyzeSpectrum(sum);
}
