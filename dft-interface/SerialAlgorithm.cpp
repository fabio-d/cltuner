#include "DftAlgorithm.h"
#include "DftAlgorithmManager.h"

#include "dft-algorithms/serial_naive_dft.cpp"
#include "dft-algorithms/serial_recursive_fft.cpp"
#include "dft-algorithms/serial_nonrecursive_fft.cpp"

template <vector<cpx> (*func)(const vector<float>&)>
struct SerialAlgorithm_real2cpx : public DftAlgorithm
{
	static DftAlgorithm *createInstance()
	{
		return new SerialAlgorithm_real2cpx();
	}

	void analyzeSpectrum(const QVector<float> &audioSamples)
	{
		const vector<cpx> output_ft = func(audioSamples.toStdVector());

		QVector<float> absValues(output_ft.size());
		for (unsigned int i = 0; i < output_ft.size(); i++)
			absValues[i] = abs(output_ft[i]);

		emit spectrumAnalyzed(absValues);
	}
};

template <vector<cpx> (*func)(const vector<cpx>&)>
struct SerialAlgorithm_cpx2cpx : public DftAlgorithm
{
	static DftAlgorithm *createInstance()
	{
		return new SerialAlgorithm_cpx2cpx();
	}

	void analyzeSpectrum(const QVector<float> &audioSamples)
	{
		vector<cpx> cpxSamples(audioSamples.size());
		for (unsigned int i = 0; i < audioSamples.size(); i++)
			cpxSamples[i] = cpx(audioSamples[i], 0);

		const vector<cpx> output_ft = func(cpxSamples);

		QVector<float> absValues(output_ft.size());
		for (unsigned int i = 0; i < output_ft.size(); i++)
			absValues[i] = abs(output_ft[i]);

		emit spectrumAnalyzed(absValues);
	}
};

static DftAlgorithmManager::SerialPlugin< SerialAlgorithm_real2cpx<serial_naive_dft> > A(serial_naive_dft_algoName<float>());
static DftAlgorithmManager::SerialPlugin< SerialAlgorithm_cpx2cpx<serial_naive_dft> > B(serial_naive_dft_algoName<cpx>());

static DftAlgorithmManager::SerialPlugin< SerialAlgorithm_real2cpx<serial_recursive_fft> > C(serial_recursive_fft_algoName<float>());
static DftAlgorithmManager::SerialPlugin< SerialAlgorithm_cpx2cpx<serial_recursive_fft> > D(serial_recursive_fft_algoName<cpx>());

static DftAlgorithmManager::SerialPlugin< SerialAlgorithm_real2cpx<serial_nonrecursive_fft> > E(serial_nonrecursive_fft_algoName<float>());
static DftAlgorithmManager::SerialPlugin< SerialAlgorithm_cpx2cpx<serial_nonrecursive_fft> > F(serial_nonrecursive_fft_algoName<cpx>());
