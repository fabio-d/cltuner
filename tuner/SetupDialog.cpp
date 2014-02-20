#include "SetupDialog.h"

#include "clhelpers/cl.h"
#include "dft-interface/DftAlgorithmManager.h"

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

	m_computePlatform->addItem("Serial (no OpenCL)", QVariant::fromValue<int>(-1));
	QList<QString> cl_platforms = clhAvailablePlatformNames();
	for (int i = 0; i < cl_platforms.count(); i++)
		m_computePlatform->addItem(QString("#%0 %1").arg(i).arg(cl_platforms[i]), QVariant::fromValue<int>(i));
	if (!cl_platforms.empty())
		m_computePlatform->setCurrentIndex(1); // Selezioniamo la prima OpenCL, se disponibile

	m_layout->addWidget(new QLabel("Compute device"), row, 0);
	m_computeDevice = new QComboBox();
	m_layout->addWidget(m_computeDevice, row++, 1);

	m_layout->addItem(new QSpacerItem(0, 20), row++, 0, 1, -1);

	m_layout->addWidget(new QLabel("DFT algorithm"), row, 0);
	m_dftAlgorithm = new QComboBox();
	m_layout->addWidget(m_dftAlgorithm, row++, 1);

	m_layout->addItem(new QSpacerItem(0, 20), row++, 0, 1, -1);

	m_layout->addWidget(new QLabel("Audio device"), row, 0);
	m_audioDevice = new QComboBox();
	for (int i = 0; i < m_availableAudioInputDevices.count(); i++)
		m_audioDevice->addItem(m_availableAudioInputDevices[i].deviceName());
	m_layout->addWidget(m_audioDevice, row++, 1);

	m_layout->addWidget(new QLabel("Sample rate"), row, 0);
	m_audioSampleRate = new QComboBox();
	m_layout->addWidget(m_audioSampleRate, row++, 1);

	connect(m_computePlatform, SIGNAL(currentIndexChanged(int)), this, SLOT(slotPlatformChanged()));
	connect(m_audioDevice, SIGNAL(currentIndexChanged(int)), this, SLOT(slotAudioDeviceChanged()));
	slotPlatformChanged();
	slotAudioDeviceChanged();
}

SetupDialog::~SetupDialog()
{
}

void SetupDialog::slotPlatformChanged()
{
	m_computeDevice->clear();
	m_dftAlgorithm->clear();

	if (m_computePlatform->currentIndex() == -1)
		return;

	QList< QPair<QString, DftAlgorithm*> > algorithms;

	const int cl_platform_index = m_computePlatform->itemData(m_computePlatform->currentIndex()).toInt();
	if (cl_platform_index == -1)
	{
		// Esecuzione seriale su CPU
		algorithms = DftAlgorithmManager::getSingleInstance()->algorithms();
	}
	else
	{
		// OpenCL
		cl_platform_id plat = clhSelectPlatform(cl_platform_index);
		foreach (const QString &s, clhAvailableDeviceNames(plat))
			m_computeDevice->addItem(s);
	}

	for (int i = 0; i < algorithms.count(); i++)
		m_dftAlgorithm->addItem(algorithms[i].first, QVariant::fromValue<void*>(static_cast<void*>(algorithms[i].second)));
}

void SetupDialog::slotAudioDeviceChanged()
{
	m_audioSampleRate->clear();

	if (m_audioDevice->currentIndex() == -1)
		return;

	QList<int> sampleRates = m_availableAudioInputDevices[m_audioDevice->currentIndex()].supportedSampleRates();
	qSort(sampleRates);

	foreach (const int r, sampleRates)
		m_audioSampleRate->addItem(QString::number(r));

	// Selezionamo la più alta come predefinita
	m_audioSampleRate->setCurrentIndex(sampleRates.count() - 1);
}
