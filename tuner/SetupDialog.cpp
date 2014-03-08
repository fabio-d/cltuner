#include "SetupDialog.h"
#include "MainWindow.h"

#include "clhelpers/cl.h"

#include <QDialogButtonBox>
#include <QLabel>
#include <QSpacerItem>

SetupDialog::SetupDialog(QWidget *parent)
: QDialog(parent), m_availableAudioInputDevices(QAudioDeviceInfo::availableDevices(QAudio::AudioInput))
{
	setMinimumSize(400, 100);

	m_layout = new QGridLayout();
	setLayout(m_layout);

	int row = 0;

	m_layout->addWidget(new QLabel("Compute platform"), row, 0);
	m_computePlatform = new QComboBox();
	m_layout->addWidget(m_computePlatform, row++, 1);

	vector<string> cl_platforms = clhAvailablePlatformNames();
	for (unsigned int i = 0; i < cl_platforms.size(); i++)
		m_computePlatform->addItem(QString("#%0 %1").arg(i).arg(cl_platforms[i].c_str()));

	m_layout->addWidget(new QLabel("Compute device"), row, 0);
	m_computeDevice = new QComboBox();
	m_layout->addWidget(m_computeDevice, row++, 1);

	m_layout->addItem(new QSpacerItem(0, 20), row++, 0, 1, -1);

	m_layout->addWidget(new QLabel("Audio device"), row, 0);
	m_audioDevice = new QComboBox();
	for (int i = 0; i < m_availableAudioInputDevices.count(); i++)
		m_audioDevice->addItem(m_availableAudioInputDevices[i].deviceName());
	m_layout->addWidget(m_audioDevice, row++, 1);

	m_layout->addWidget(new QLabel("Sample rate"), row, 0);
	m_audioSampleRate = new QComboBox();
	m_layout->addWidget(m_audioSampleRate, row++, 1);

	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	m_layout->addWidget(buttonBox, row, 0, 1, -1);

	connect(m_computePlatform, SIGNAL(currentIndexChanged(int)), this, SLOT(slotPlatformChanged()));
	connect(m_audioDevice, SIGNAL(currentIndexChanged(int)), this, SLOT(slotAudioDeviceChanged()));
	slotPlatformChanged();
	slotAudioDeviceChanged();

	connect(buttonBox, SIGNAL(accepted()), this, SLOT(slotAccepted()));
	connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}

SetupDialog::~SetupDialog()
{
}

void SetupDialog::slotPlatformChanged()
{
	m_computeDevice->clear();

	if (m_computePlatform->currentIndex() == -1)
		return;

	QList<QString> algoNames;

	cl_platform_id plat = clhSelectPlatform(m_computePlatform->currentIndex());
	foreach (const string &s, clhAvailableDeviceNames(plat))
		m_computeDevice->addItem(s.c_str());
}

void SetupDialog::slotAudioDeviceChanged()
{
	m_audioSampleRate->clear();

	if (m_audioDevice->currentIndex() == -1)
		return;

	QList<int> sampleRates = m_availableAudioInputDevices[m_audioDevice->currentIndex()].supportedSampleRates();
	qSort(sampleRates);

	foreach (const int r, sampleRates)
		m_audioSampleRate->addItem(QString::number(r), QVariant::fromValue<int>(r));

	// Selezionamo la più alta come predefinita
	m_audioSampleRate->setCurrentIndex(sampleRates.count() - 1);
}

void SetupDialog::slotAccepted()
{
	if (m_computePlatform->currentIndex() == -1
		|| m_computeDevice->currentIndex() == -1
		|| m_audioDevice->currentIndex() == -1
		|| m_audioSampleRate->currentIndex() == -1)
	{
		return; // Non chiamiamo accept()
	}

	const int sample_rate = m_audioSampleRate->itemData(m_audioSampleRate->currentIndex()).toInt();
	const int cl_device_index = m_computeDevice->currentIndex();

	LiveAudioInput *audioIn = new LiveAudioInput(
		m_availableAudioInputDevices[m_audioDevice->currentIndex()],
		sample_rate,
		2048);

	cl_platform_id plat = clhSelectPlatform(m_computePlatform->currentIndex());
	cl_device_id dev = clhSelectDevice(plat, m_computeDevice->currentIndex());
	MainWindow *w = new MainWindow(audioIn, plat, dev);
	accept();
	w->show();
}
