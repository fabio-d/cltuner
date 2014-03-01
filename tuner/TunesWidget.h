#ifndef TUNER_TUNESWIDGET_H
#define TUNER_TUNESWIDGET_H

#include <QAbstractGraphicsShapeItem>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QSet>
#include <QVector>

class TunesWidget : public QGraphicsView
{
	Q_OBJECT

	public:
		explicit TunesWidget(QWidget *parent = 0);

	public slots:
		void setPressedKeys(const QSet<int> &keyNumbers);
		void setHighlightedKey(int keyNumber);

	protected:
		void resizeEvent(QResizeEvent *re);

	private:
		void applyColors();

		QGraphicsScene *m_scene;
		QVector<QAbstractGraphicsShapeItem*> m_keys;

		QSet<int> m_pressedKeys;
		int m_highlightedKey;
};

#endif // TUNER_TUNESWIDGET_H
