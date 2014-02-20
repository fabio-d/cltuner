#ifndef DFTINTERFACE_DFTALGORITHMMANAGER_H
#define DFTINTERFACE_DFTALGORITHMMANAGER_H

#include "DftAlgorithm.h"

#include <QList>
#include <QPair>

class DftAlgorithmManager
{
	template <class T>
	friend struct Impl;

	public:
		template <class T>
		struct Impl
		{
			Impl(const char *algoName);
		};

		static DftAlgorithmManager * getSingleInstance();

		const QList< QPair<QString, DftAlgorithm*> >& algorithms() const;

	private:
		void registerImplementation(const char *algoName);

		QList< QPair<QString, DftAlgorithm*> > m_algorithms;
};

template <class T>
DftAlgorithmManager::Impl<T>::Impl(const char *algoName)
{
	DftAlgorithmManager *man = DftAlgorithmManager::getSingleInstance();
	man->registerImplementation(algoName);
}

#endif // DFTINTERFACE_DFTALGORITHMMANAGER_H
