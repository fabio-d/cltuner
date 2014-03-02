#include "DftAlgorithm.h"
#include "DftAlgorithmManager.h"

#include <iostream>

#include "dft-algorithms/cl_naive_dft.cpp"
#include "dft-algorithms/cl_dft_matrix.cpp"
#include "dft-algorithms/cl_dft_symmatrix.cpp"
#include "dft-algorithms/cl_dft_matrix_tex.cpp"
#include "dft-algorithms/cl_fft.cpp"

#ifdef HAVE_clFFT
#include "dft-algorithms/clFFT.cpp"
#endif

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

static DftAlgorithmManager::CLPlugin< CLAlgorithm_real2cpx< cl_dft_matrix<float> > > C(cl_dft_matrix_algoName<float>());
static DftAlgorithmManager::CLPlugin< CLAlgorithm_cpx2cpx< cl_dft_matrix<cpx> > > D(cl_dft_matrix_algoName<cpx>());

static DftAlgorithmManager::CLPlugin< CLAlgorithm_real2cpx< cl_dft_symmatrix<float> > > E(cl_dft_symmatrix_algoName<float>());
static DftAlgorithmManager::CLPlugin< CLAlgorithm_cpx2cpx< cl_dft_symmatrix<cpx> > > F(cl_dft_symmatrix_algoName<cpx>());

static DftAlgorithmManager::CLPlugin< CLAlgorithm_real2cpx< cl_dft_matrix_tex<float> > > G(cl_dft_matrix_tex_algoName<float>());
static DftAlgorithmManager::CLPlugin< CLAlgorithm_cpx2cpx< cl_dft_matrix_tex<cpx> > > H(cl_dft_matrix_tex_algoName<cpx>());

static DftAlgorithmManager::CLPlugin< CLAlgorithm_real2cpx< cl_fft<float> > > I(cl_fft_algoName<float>());
static DftAlgorithmManager::CLPlugin< CLAlgorithm_cpx2cpx< cl_fft<cpx> > > J(cl_fft_algoName<cpx>());

#ifdef HAVE_clFFT
static DftAlgorithmManager::CLPlugin< CLAlgorithm_real2cpx< clFFT<float> > > Y(clFFT_algoName<float>());
static DftAlgorithmManager::CLPlugin< CLAlgorithm_cpx2cpx< clFFT<cpx> > > Z(clFFT_algoName<cpx>());
#endif
