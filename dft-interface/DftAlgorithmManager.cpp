#include "DftAlgorithmManager.h"

#include <cstddef>

DftAlgorithmManager * DftAlgorithmManager::getSingleInstance()
{
	static DftAlgorithmManager *instance = NULL;

	if (instance == NULL)
		instance = new DftAlgorithmManager();

	return instance;
}

const QList< QPair<QString, DftAlgorithm*> >& DftAlgorithmManager::algorithms() const
{
	return m_algorithms;
}

void DftAlgorithmManager::registerImplementation(const char *algoName)
{
	m_algorithms << QPair<QString, DftAlgorithm*>(algoName, NULL);
}
