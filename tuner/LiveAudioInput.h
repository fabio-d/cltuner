#ifndef TUNER_LIVEAUDIOINPUT_H
#define TUNER_LIVEAUDIOINPUT_H

#include <QAudioInput>
#include <QByteArray>
#include <QDataStream>
#include <QIODevice>
#include <QVector>

class LiveAudioInput : public QObject
{
	Q_OBJECT

	public:
		LiveAudioInput(const QAudioDeviceInfo &audioDevice, int sampleRate, int samplesPerChunk, QObject *parent = 0);
		~LiveAudioInput();

		int sampleRate() const;

	signals:
		void newChunkAvailable(QVector<qint16> data);

	private slots:
		void slotStateChanged(QAudio::State state);

	private:
		class ReceiverIODevice : public QIODevice
		{
			public:
				ReceiverIODevice(LiveAudioInput *parent);

			protected:
				qint64 readData(char *data, qint64 maxlen);
				qint64 writeData(const char *data, qint64 len);

			private:
				LiveAudioInput *parent;
				QByteArray buffer;
		};

		QAudioInput *m_input;
		ReceiverIODevice m_receiver;
		int m_samplesPerChunk, m_sampleRate;
};

#endif // TUNER_LIVEAUDIOINPUT_H
