#include "DftAlgorithmManager.h"

#include <cstddef>

DftAlgorithmManager * DftAlgorithmManager::getSingleInstance()
{
	static DftAlgorithmManager *instance = NULL;

	if (instance == NULL)
		instance = new DftAlgorithmManager();

	return instance;
}

const QList< QPair<QString, SerialAlgorithmFactoryFunction> >& DftAlgorithmManager::serialAlgorithms() const
{
	return m_serialAlgorithms;
}

const QList< QPair<QString, CLAlgorithmFactoryFunction> >& DftAlgorithmManager::clAlgorithms() const
{
	return m_clAlgorithms;
}

void DftAlgorithmManager::registerSerialImplementation(const char *algoName, SerialAlgorithmFactoryFunction factory)
{
	m_serialAlgorithms << QPair<QString, SerialAlgorithmFactoryFunction>(algoName, factory);
}

void DftAlgorithmManager::registerCLImplementation(const char *algoName, CLAlgorithmFactoryFunction factory)
{
	m_clAlgorithms << QPair<QString, CLAlgorithmFactoryFunction>(algoName, factory);
}
