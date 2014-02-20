#ifndef DFTTESTS_TESTLIB_H
#define DFTTESTS_TESTLIB_H

#include "dft-algorithms/cpx.h"

#include <ostream>
#include <vector>

template <typename T>
vector<T> generateTestData(int N);

template <>
vector<float> generateTestData(int N)
{
	vector<float> res(N);

	for (int i = 0; i < N; ++i)
		res[i] = float( sin(i*2*M_PI/N)+.5*sin(i*8*M_PI/N) );

	return res;
}

template <>
vector<cpx> generateTestData(int N)
{
	vector<float> realData = generateTestData<float>(N);
	vector<cpx> res(N);

	for (int i = 0; i < N; ++i)
		res[i] = cpx(realData[i], 0);

	return res;
}

static float maxAbsDistance(const vector<cpx> &vecA, const vector<cpx> &vecB)
{
	float res = 0;
	for (unsigned int i = 0; i < vecA.size(); i++)
		res = max(res, abs(vecA[i] - vecB[i]));
	return res;
}

static ostream& operator<<(ostream& s, const cpx &c)
{
	float im = imag(c);
	s << real(c) << " ";

	if (im >= 0)
		s << "+";

	return s << im << "i";
}

template <typename T>
ostream& operator<<(ostream& s, const vector<T> &v)
{
	s << '[';

	for (unsigned int i = 0; i < v.size(); i++)
	{
		if (i != 0)
			s << ',';
		s << ' ' << v[i];
	}

	return s << " ]";
}

#endif // DFTTESTS_TESTLIB_H
