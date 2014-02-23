// Calcolo della DFT in base alla definizione
// https://en.wikipedia.org/wiki/Discrete_Fourier_transform

#include "cpx.h"
#include "pi_float.h"
#include "sizeconv.h"

#include <cstdio>
#include <ctime>

template <typename T> const char *serial_naive_dft_algoName();

template <> const char *serial_naive_dft_algoName<float>()
{ return "Serial naive DFT (real-to-complex)"; }

template <> const char *serial_naive_dft_algoName<cpx>()
{ return "Serial naive DFT (complex-to-complex)"; }

template <typename T>
vector<cpx> serial_naive_dft(const vector<T> &data)
{
	const int N = data.size();
	vector<cpx> result(N);

	const clock_t start = clock();

	for (int k = 0; k < N; k++)
	{
		cpx sum(0, 0);

		for (int j = 0; j < N; j++)
		{
			float s, c;
			sincosf(M_PI_F/N*-2.f*j*k, &s, &c);
			sum += data[j] * cpx(c, s);
		}

		result[k] = sum;
	}

	const clock_t end = clock();
	const float secs = (end - start) / float(CLOCKS_PER_SEC);
	const float memSizeKiB = (N*N*sizeof(T) + N*sizeof(cpx)) / SIZECONV_MB;
	fprintf(stderr, "%s [N=%d]: %g ms, %g MiB/s, %g samples/s\n",
		serial_naive_dft_algoName<T>(),
		N, secs * 1e3, memSizeKiB / secs, N / secs);

	return result;
}
