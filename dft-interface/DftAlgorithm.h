#ifndef DFTINTERFACE_DFTALGORITHM_H
#define DFTINTERFACE_DFTALGORITHM_H

#include <vector>

class DftAlgorithm
{
	public:
		virtual ~DftAlgorithm();

		virtual std::vector<float> analyzeSpectrum(std::vector<float> &audioSamples) const = 0;
};

#endif // DFTINTERFACE_DFTALGORITHM_H
