#include "SetupDialog.h"

#include "clhelpers/cl.h"
#include "dft-interface/DftAlgorithmManager.h"

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

	m_computePlatform->addItem("Serial (no OpenCL)", QVariant::fromValue<int>(-1));
	vector<string> cl_platforms = clhAvailablePlatformNames();
	for (unsigned int i = 0; i < cl_platforms.size(); i++)
		m_computePlatform->addItem(QString("#%0 %1").arg(i).arg(cl_platforms[i].c_str()), QVariant::fromValue<int>(i));
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

	m_layout->addWidget(new QLabel("Window size"), row, 0);
	m_window = new QComboBox();
	for (int i = 512; i <= 32768; i *= 2)
		m_window->addItem(QString("%0 samples").arg(i), QVariant::fromValue<int>(i));
	m_window->setCurrentIndex(2); // Default value: 2048 samples
	m_layout->addWidget(m_window, row++, 1);

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
	m_dftAlgorithm->clear();

	if (m_computePlatform->currentIndex() == -1)
		return;

	QList<QString> algoNames;

	const int cl_platform_index = m_computePlatform->itemData(m_computePlatform->currentIndex()).toInt();
	if (cl_platform_index == -1)
	{
		// Esecuzione seriale su CPU
		QList< QPair<QString, SerialAlgorithmFactoryFunction> > algorithms = DftAlgorithmManager::getSingleInstance()->serialAlgorithms();
		for (int i = 0; i < algorithms.count(); i++)
			algoNames << algorithms[i].first;
	}
	else
	{
		// OpenCL
		cl_platform_id plat = clhSelectPlatform(cl_platform_index);
		foreach (const string &s, clhAvailableDeviceNames(plat))
			m_computeDevice->addItem(s.c_str());

		QList< QPair<QString, CLAlgorithmFactoryFunction> > algorithms = DftAlgorithmManager::getSingleInstance()->clAlgorithms();
		for (int i = 0; i < algorithms.count(); i++)
			algoNames << algorithms[i].first;
	}

	foreach (const QString &name, algoNames)
		m_dftAlgorithm->addItem(name);
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

void SetupDialog::slotAccepted()
{
	if (m_computePlatform->currentIndex() == -1
		|| m_dftAlgorithm->currentIndex() == -1
		|| m_audioDevice->currentIndex() == -1
		|| m_audioSampleRate->currentIndex() == -1)
	{
		return; // Non chiamiamo accept()
	}

	const int window_size = m_window->itemData(m_computePlatform->currentIndex()).toInt();

	DftAlgorithm *algorithmInstance;

	const int cl_platform_index = m_computePlatform->itemData(m_computePlatform->currentIndex()).toInt();
	if (cl_platform_index == -1)
	{
		// Esecuzione seriale su CPU
		QPair<QString, SerialAlgorithmFactoryFunction> algorithm =
			DftAlgorithmManager::getSingleInstance()->serialAlgorithms().at(m_dftAlgorithm->currentIndex());

		algorithmInstance = algorithm.second();
	}
	else
	{
		// OpenCL
		const int cl_device_index = m_computeDevice->currentIndex();
		if (cl_device_index == -1)
			return; // Non chiamiamo accept()

		QPair<QString, CLAlgorithmFactoryFunction> algorithm =
			DftAlgorithmManager::getSingleInstance()->clAlgorithms().at(m_dftAlgorithm->currentIndex());

		algorithmInstance = algorithm.second(cl_platform_index, cl_device_index, window_size);
	}

	accept();
}
