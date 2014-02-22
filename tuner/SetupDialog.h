#ifndef TUNER_SETUPDIALOG_H
#define TUNER_SETUPDIALOG_H

#include <QAudioDeviceInfo>
#include <QDialog>
#include <QComboBox>
#include <QGridLayout>
#include <QList>

class SetupDialog : public QDialog
{
	Q_OBJECT

	public:
		explicit SetupDialog(QWidget *parent = 0);
		~SetupDialog();

	private slots:
		void slotPlatformChanged();
		void slotAudioDeviceChanged();
		void slotAccepted();

	private:
		QGridLayout *m_layout;

		QComboBox *m_computePlatform;
		QComboBox *m_computeDevice;

		QComboBox *m_dftAlgorithm;

		QComboBox *m_audioDevice;
		QComboBox *m_audioSampleRate;
		QComboBox *m_window;

		QList<QAudioDeviceInfo> m_availableAudioInputDevices;
};

#endif // TUNER_SETUPDIALOG_H
