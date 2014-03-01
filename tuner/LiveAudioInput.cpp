#include "LiveAudioInput.h"

#include <cstdlib>
#include <cstring>

#include <QAudioFormat>
#include <QDebug>

LiveAudioInput::LiveAudioInput(const QAudioDeviceInfo &audioDevice, int sampleRate, int samplesPerChunk, QObject *parent)
: QObject(parent), receiver(this), samplesPerChunk(samplesPerChunk)
{
	QAudioFormat fmt;
	fmt.setFrequency(sampleRate);
	fmt.setChannels(1);
	fmt.setSampleSize(16);
	fmt.setSampleType(QAudioFormat::SignedInt);
	fmt.setByteOrder(QAudioFormat::LittleEndian);
	fmt.setCodec("audio/pcm");

	QAudioDeviceInfo info = QAudioDeviceInfo::defaultInputDevice();
	if (!info.isFormatSupported(fmt))
	{
		qWarning() << "LiveAudioInput -- fmt not supported";
		fmt = info.nearestFormat(fmt);
		qDebug() << "closest match: frequency=" << fmt.frequency() << ", channels=" << fmt.channels()
			 << ", sampleSize=" << fmt.sampleSize();
		exit(EXIT_FAILURE);
	}

	input = new QAudioInput(audioDevice, fmt);
	input->setBufferSize(500); // fino a 500 ms

	connect(input, SIGNAL(stateChanged(QAudio::State)), this, SLOT(slotStateChanged(QAudio::State)));

	receiver.open(QIODevice::WriteOnly);
	input->start(&receiver);
}

LiveAudioInput::~LiveAudioInput()
{
	input->stop();
	delete input;

	receiver.close();
}

void LiveAudioInput::slotStateChanged(QAudio::State state)
{
	const char *stateName = 0;

	switch (state)
	{
		case QAudio::ActiveState:
			stateName = "ActiveState";
			break;
		case QAudio::SuspendedState:
			stateName = "SuspendedState";
			break;
		case QAudio::StoppedState:
			stateName = "StoppedState";
			break;
		case QAudio::IdleState:
			stateName = "IdleState";
			break;
	}

	qDebug() << "slotStateChanged:" << stateName;
}

LiveAudioInput::ReceiverIODevice::ReceiverIODevice(LiveAudioInput *parent)
: parent(parent)
{
}

qint64 LiveAudioInput::ReceiverIODevice::readData(char *data, qint64 maxlen)
{
	return 0; // siamo write-only!
}

qint64 LiveAudioInput::ReceiverIODevice::writeData(const char *data, qint64 len)
{
	buffer.append(data, len);

	// Consuma chunks a blocchi di 2*samplesPerChunk bytes (2 byte per sample)
	while (buffer.size() > 2*parent->samplesPerChunk)
	{
		const qint16 *rawDataChunk = reinterpret_cast<const qint16*>(buffer.constData());
		QVector<qint16> vecDataChunk(parent->samplesPerChunk);
		memcpy(vecDataChunk.data(), rawDataChunk, 2*parent->samplesPerChunk);
		buffer = buffer.mid(2*parent->samplesPerChunk);
		emit parent->newChunkAvailable(vecDataChunk);
	}
	return len;
}
