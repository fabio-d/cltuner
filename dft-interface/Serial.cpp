#include "DftAlgorithm.h"
#include "DftAlgorithmManager.h"

#include "dft-algorithms/serial_naive_dft.cpp"
#include "dft-algorithms/serial_recursive_fft.cpp"
#include "dft-algorithms/serial_nonrecursive_fft.cpp"

template <vector<cpx> (*func)(const vector<float>&)>
class Serial_real2cpx : public DftAlgorithm
{
	vector<float> analyzeSpectrum(vector<float> &audioSamples) const
	{
		const vector<cpx> ft = func(audioSamples);

		vector<float> absValues(ft.size());
		for (unsigned int i = 0; i < ft.size(); i++)
			absValues[i] = abs(ft[i]);
	}
};

template <vector<cpx> (*func)(const vector<cpx>&)>
class Serial_cpx2cpx : public DftAlgorithm
{
	vector<float> analyzeSpectrum(vector<float> &audioSamples) const
	{
		vector<cpx> cpxSamples(audioSamples.size());
		for (unsigned int i = 0; i < audioSamples.size(); i++)
			cpxSamples[i] = cpx(audioSamples[i], 0);

		const vector<cpx> ft = func(cpxSamples);

		vector<float> absValues(ft.size());
		for (unsigned int i = 0; i < ft.size(); i++)
			absValues[i] = abs(ft[i]);
	}
};

static DftAlgorithmManager::Impl< Serial_real2cpx<serial_naive_dft> > A(serial_naive_dft_algoName<float>());
static DftAlgorithmManager::Impl< Serial_cpx2cpx<serial_naive_dft> > B(serial_naive_dft_algoName<cpx>());

static DftAlgorithmManager::Impl< Serial_real2cpx<serial_recursive_fft> > C(serial_recursive_fft_algoName<float>());
static DftAlgorithmManager::Impl< Serial_cpx2cpx<serial_recursive_fft> > D(serial_recursive_fft_algoName<cpx>());

static DftAlgorithmManager::Impl< Serial_real2cpx<serial_nonrecursive_fft> > E(serial_nonrecursive_fft_algoName<float>());
static DftAlgorithmManager::Impl< Serial_cpx2cpx<serial_nonrecursive_fft> > F(serial_nonrecursive_fft_algoName<cpx>());
