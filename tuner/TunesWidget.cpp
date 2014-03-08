#include "TunesWidget.h"

#include <QDebug>

#define BH 90	// Altezza tasti neri
#define WH 170	// Altezza tasti bianchi
#define TW 1820	// Larghezza totale

TunesWidget::TunesWidget(QWidget *parent)
: QGraphicsView(parent), m_highlightedKey(-1)
{
	m_scene = new QGraphicsScene(this);
	setScene(m_scene);

	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

	for (int x = 0, i = 0; i < 88; i++)
	{
		QAbstractGraphicsShapeItem *item;

		if (i == 0) // LA iniziale, senza spazio per il LAb
			item = m_scene->addPolygon(QPolygonF() << QPointF(0, 0) << QPointF(0, WH) << QPointF(35, WH) << QPointF(35, BH) << QPointF(30, BH) << QPointF(30, 0)), x += 30;
		else if (i == 87) // DO finale, senza spazio per il DO#
			item = m_scene->addRect(x, 0, 35, WH), x += 35;
		else switch (i % 12)
		{
			case 0: // LA
				item = m_scene->addPolygon(QPolygonF() << QPointF(x, 0) << QPointF(x, BH) << QPointF(x-10, BH) << QPointF(x-10, WH) << QPointF(x+25, WH) << QPointF(x+25, BH) << QPointF(x+20, BH) << QPointF(x+20, 0)), x += 20;
				break;
			case 1: // LA#
			case 9: // FA#
			case 11: // SOL#
				item = m_scene->addRect(x, 0, 20, BH), x += 20;
				break;
			case 2: // SI
				item = m_scene->addPolygon(QPolygonF() << QPointF(x, 0) << QPointF(x, BH) << QPointF(x-15, BH) << QPointF(x-15, WH) << QPointF(x+20, WH) << QPointF(x+20, 0)), x += 20;
				break;
			case 3: // DO
				item = m_scene->addPolygon(QPolygonF() << QPointF(x, 0) << QPointF(x, WH) << QPointF(x+35, WH) << QPointF(x+35, BH) << QPointF(x+21, BH) << QPointF(x+21, 0)), x += 21;
				break;
			case 4: // DO#
			case 6: // RE#
				item = m_scene->addRect(x, 0, 21, BH), x += 21;
				break;
			case 5: // RE
				item = m_scene->addPolygon(QPolygonF() << QPointF(x, 0) << QPointF(x, BH) << QPointF(x-7, BH) << QPointF(x-7, WH) << QPointF(x+28, WH) << QPointF(x+28, BH) << QPointF(x+21, BH) << QPointF(x+21, 0)), x += 21;
				break;
			case 7: // MI
				item = m_scene->addPolygon(QPolygonF() << QPointF(x, 0) << QPointF(x, BH) << QPointF(x-14, BH) << QPointF(x-14, WH) << QPointF(x+21, WH) << QPointF(x+21, 0)), x += 21;
				break;
			case 8: // FA
				item = m_scene->addPolygon(QPolygonF() << QPointF(x, 0) << QPointF(x, WH) << QPointF(x+35, WH) << QPointF(x+35, BH) << QPointF(x+20, BH) << QPointF(x+20, 0)), x += 20;
				break;
			case 10: // SOL
				item = m_scene->addPolygon(QPolygonF() << QPointF(x, 0) << QPointF(x, BH) << QPointF(x-5, BH) << QPointF(x-5, WH) << QPointF(x+30, WH) << QPointF(x+30, BH) << QPointF(x+20, BH) << QPointF(x+20, 0)), x += 20;
				break;
		}

		m_keys.append(item);
	}

	applyColors();
}

void TunesWidget::setPressedKeys(const QSet<int> &keyNumbers)
{
	if (m_pressedKeys == keyNumbers)
		return;

	m_pressedKeys = keyNumbers;
	applyColors();
}

void TunesWidget::setHighlightedKey(int keyNumber)
{
	if (m_highlightedKey == keyNumber)
		return;

	m_highlightedKey = keyNumber;
	applyColors();
}

void TunesWidget::applyColors()
{
	for (int i = 0; i < 88; i++)
	{
		if (m_pressedKeys.contains(i))
			m_keys[i]->setBrush(Qt::green);
		else if (i == m_highlightedKey)
			m_keys[i]->setBrush(Qt::red);
		else switch (i % 12)
		{
			case 1: // LA#
			case 4: // DO#
			case 6: // RE#
			case 9: // FA#
			case 11: // SOL#
				m_keys[i]->setBrush(Qt::black);
				break;
			default:
				m_keys[i]->setBrush(Qt::white);
		}
	}
}

void TunesWidget::resizeEvent(QResizeEvent *re)
{
	const int margin = 40;
	fitInView(-margin, -margin, TW+2*margin, WH+2*margin, Qt::KeepAspectRatio);
}
