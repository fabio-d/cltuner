#include "SpectrumWidget.h"

#include <QDebug>
#include <QPaintEvent>
#include <QPainter>
#include <QScrollBar>

#include <cmath>
#include <numeric>

#define ZOOM_INCREMENT	qreal(1.5)

SpectrumWidget::SpectrumWidget(QWidget *parent)
: QAbstractScrollArea(parent), m_zoomLevel(1.0), m_highlightedKey(-1)
{
	setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	horizontalScrollBar()->setSingleStep(20);
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	verticalScrollBar()->setSingleStep(20);
}

int SpectrumWidget::highlightedKey() const
{
	return m_highlightedKey;
}

void SpectrumWidget::setData(QVector<float> newData, float threshold, int sampleRate)
{
	m_data = newData;
	m_maximum = *std::max_element(m_data.begin(), m_data.end());
	m_threshold = threshold;
	m_sampleRate = sampleRate;

	zoomMultiply(1); // imposta scrollbar e ridisegna tutto
}

void SpectrumWidget::zoomIn()
{
	zoomMultiply(ZOOM_INCREMENT);
}

void SpectrumWidget::zoomOut()
{
	zoomMultiply(1 / ZOOM_INCREMENT);
}

void SpectrumWidget::paintEvent(QPaintEvent *pe)
{
	if (m_data.size() < 2)
		return;

        QPainter p(viewport());
	p.fillRect(pe->rect(), Qt::white);
        p.setRenderHint(QPainter::Antialiasing);

	p.translate(-horizontalScrollBar()->value(), 0);
	p.scale(viewport()->width() * m_zoomLevel, -viewport()->height());
	p.translate(0, -1);

	float vScale = qMax(m_maximum, float(m_data.size()));
	p.scale(1, 1 / vScale);
	p.setPen(Qt::red);
	p.drawLine(0, m_threshold, 1, m_threshold);

	p.scale(qreal(1) / (m_data.size() - 1), 1);

	p.setPen(Qt::black);
	QPainterPath path;
	path.moveTo(0, m_data[0]);
	for (int i = 1; i < m_data.size(); ++i)
		path.lineTo(i, m_data[i]);
	p.drawPath(path);

	int hlKey = -1;

	p.setPen(Qt::red);
	QPoint mousePos = mapFromGlobal(QCursor::pos());
	if (mousePos.x() >= 0 && mousePos.x() < viewport()->width()
		&& mousePos.y() >= 0 && mousePos.y() < viewport()->height())
	{
		const int freq_idx = p.transform().inverted().map(mousePos).x();
		p.drawLine(freq_idx, 0, freq_idx, vScale);

		if (freq_idx != 0 && freq_idx < m_data.size() / 2)
		{
			const float freq = float(m_sampleRate) * freq_idx / m_data.size();
			hlKey = qRound(12 * log2(freq / 440) + 49);

			QString freqText = QString("%1 Hz").arg(freq);
			p.resetTransform();
			p.drawText(mousePos, freqText);
		}
	}

	hlKey = (hlKey >= 0 && hlKey < 88) ? hlKey : -1;
	if (hlKey != m_highlightedKey)
	{
		m_highlightedKey = hlKey;
		emit highlightedKeyChanged();
	}
}

void SpectrumWidget::resizeEvent(QResizeEvent *re)
{
	zoomMultiply(1); // imposta scrollbar e ridisegna tutto

	QAbstractScrollArea::resizeEvent(re);
}

void SpectrumWidget::zoomMultiply(qreal factor)
{
	// Salva posizione corrente
	const double hPos = (double)(horizontalScrollBar()->value() + viewport()->width() / 2) /
	                            (horizontalScrollBar()->maximum() + viewport()->width());

	m_zoomLevel = qMax(m_zoomLevel * factor, 1.0);

	const int totalWidth = viewport()->width() * m_zoomLevel;

	horizontalScrollBar()->setRange(0, qMax(0, totalWidth - viewport()->width()));
	horizontalScrollBar()->setPageStep(viewport()->width());

	// Ripristina posizione
	horizontalScrollBar()->setValue( hPos * (horizontalScrollBar()->maximum() + viewport()->width()) -
	                                 viewport()->width() / 2);

	viewport()->update();
}
