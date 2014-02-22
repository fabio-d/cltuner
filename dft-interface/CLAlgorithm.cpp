#include "DftAlgorithm.h"
#include "DftAlgorithmManager.h"

#include <iostream>

#include "dft-algorithms/cl_naive_dft.cpp"

template <typename T>
struct CLAlgorithm_real2cpx : public T, public DftAlgorithm
{
	static DftAlgorithm *createInstance(int platform_index, int device_index, int samplesPerRun)
	{
		return new CLAlgorithm_real2cpx(platform_index, device_index, samplesPerRun);
	}

	CLAlgorithm_real2cpx(int platform_index, int device_index, int samplesPerRun)
	: T(platform_index, device_index, samplesPerRun)
	{
	}

	void analyzeSpectrum(const QVector<float> &audioSamples)
	{
		const vector<cpx> output_ft = this->run(audioSamples.toStdVector());

		QVector<float> absValues(output_ft.size());
		for (unsigned int i = 0; i < output_ft.size(); i++)
			absValues[i] = abs(output_ft[i]);

		emit spectrumAnalyzed(absValues);
	}
};

template <typename T>
struct CLAlgorithm_cpx2cpx : public T, public DftAlgorithm
{
	static DftAlgorithm *createInstance(int platform_index, int device_index, int samplesPerRun)
	{
		return new CLAlgorithm_cpx2cpx(platform_index, device_index, samplesPerRun);
	}

	CLAlgorithm_cpx2cpx(int platform_index, int device_index, int samplesPerRun)
	: T(platform_index, device_index, samplesPerRun)
	{
	}

	void analyzeSpectrum(const QVector<float> &audioSamples)
	{
		vector<cpx> cpxSamples(audioSamples.size());
		for (unsigned int i = 0; i < audioSamples.size(); i++)
			cpxSamples[i] = cpx(audioSamples[i], 0);

		const vector<cpx> output_ft = this->run(cpxSamples);

		QVector<float> absValues(output_ft.size());
		for (unsigned int i = 0; i < output_ft.size(); i++)
			absValues[i] = abs(output_ft[i]);

		emit spectrumAnalyzed(absValues);
	}
};

static DftAlgorithmManager::CLPlugin< CLAlgorithm_real2cpx< cl_naive_dft<float> > > A(cl_naive_dft_algoName<float>());
static DftAlgorithmManager::CLPlugin< CLAlgorithm_cpx2cpx< cl_naive_dft<cpx> > > B(cl_naive_dft_algoName<cpx>());
