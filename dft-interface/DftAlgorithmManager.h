#ifndef DFTINTERFACE_DFTALGORITHMMANAGER_H
#define DFTINTERFACE_DFTALGORITHMMANAGER_H

#include "DftAlgorithm.h"
#include "clhelpers/cl.h"

#include <QList>
#include <QPair>

typedef DftAlgorithm *(*SerialAlgorithmFactoryFunction)();
typedef DftAlgorithm *(*CLAlgorithmFactoryFunction)(int platform_index, int device_index, int samplesPerRun);

class DftAlgorithmManager
{
	template <class T>
	friend struct SerialPlugin;

	template <class T>
	friend struct CLPlugin;

	public:
		template <class T>
		struct SerialPlugin
		{
			SerialPlugin(const char *algoName);
		};

		template <class T>
		struct CLPlugin
		{
			CLPlugin(const char *algoName);
		};

		static DftAlgorithmManager * getSingleInstance();

		const QList< QPair<QString, SerialAlgorithmFactoryFunction> >& serialAlgorithms() const;
		const QList< QPair<QString, CLAlgorithmFactoryFunction> >& clAlgorithms() const;

	private:
		void registerSerialImplementation(const char *algoName, SerialAlgorithmFactoryFunction factory);
		void registerCLImplementation(const char *algoName, CLAlgorithmFactoryFunction factory);

		QList< QPair<QString, SerialAlgorithmFactoryFunction> > m_serialAlgorithms;
		QList< QPair<QString, CLAlgorithmFactoryFunction> > m_clAlgorithms;
};

template <class T>
DftAlgorithmManager::SerialPlugin<T>::SerialPlugin(const char *algoName)
{
	DftAlgorithmManager *man = DftAlgorithmManager::getSingleInstance();
	man->registerSerialImplementation(algoName, T::createInstance);
}

template <class T>
DftAlgorithmManager::CLPlugin<T>::CLPlugin(const char *algoName)
{
	DftAlgorithmManager *man = DftAlgorithmManager::getSingleInstance();
	man->registerCLImplementation(algoName, T::createInstance);
}

#endif // DFTINTERFACE_DFTALGORITHMMANAGER_H
