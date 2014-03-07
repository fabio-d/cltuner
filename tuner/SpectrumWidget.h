#ifndef TUNER_SPECTRUMWIDGET_H
#define TUNER_SPECTRUMWIDGET_H

#include <QAbstractScrollArea>
#include <QVector>

class SpectrumWidget : public QAbstractScrollArea
{
	Q_OBJECT

	public:
		explicit SpectrumWidget(QWidget *parent = 0);

	public slots:
		void setData(QVector<float> newData, float threshold, int sampleRate);
		void zoomIn();
		void zoomOut();

	signals:
		void highlightedKeyAvailable(int keyNumber);

	protected:
		void paintEvent(QPaintEvent *pe);
		void resizeEvent(QResizeEvent *re);

	private:
		void zoomMultiply(qreal factor);

		float m_zoomLevel;

		float m_maximum, m_threshold;
		int m_sampleRate;
		QVector<float> m_data;
};

#endif // TUNER_SPECTRUMWIDGET_H
