#include "SpectrumWidget.h"

#include <QDebug>
#include <QPaintEvent>
#include <QPainter>
#include <QScrollBar>

#include <numeric>

#define ZOOM_INCREMENT	qreal(1.5)

SpectrumWidget::SpectrumWidget(QWidget *parent)
: QAbstractScrollArea(parent), m_zoomLevel(1.0)
{
	setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	horizontalScrollBar()->setSingleStep(20);
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	verticalScrollBar()->setSingleStep(20);
}

void SpectrumWidget::setData(QVector<float> newData, float threshold)
{
	m_data = newData;
	m_maximum = *std::max_element(m_data.begin(), m_data.end());
	m_threshold = threshold;

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
        QPainter p(viewport());
	p.fillRect(pe->rect(), Qt::white);
        p.setRenderHint(QPainter::Antialiasing);

	p.translate(-horizontalScrollBar()->value(), 0);
	p.scale(viewport()->width() * m_zoomLevel, -viewport()->height());
	p.translate(0, -1);

	if (m_data.size() < 2)
		return;

	p.scale(1, 1 / m_maximum);
	p.setPen(Qt::red);
	p.drawLine(0, m_threshold, 1, m_threshold);

	p.scale(qreal(1) / (m_data.size() - 1), 1);

	p.setPen(Qt::black);
	QPainterPath path;
	path.moveTo(0, m_data[0]);
	for (int i = 1; i < m_data.size(); ++i)
		path.lineTo(i, m_data[i]);
	p.drawPath(path);

	p.setPen(Qt::green);
	for (int i = 0; i < m_data.size(); ++i)
	{
		if (m_data[i] > m_threshold)
			p.drawLine(i, 0, i, m_maximum);
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
