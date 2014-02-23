// Calcolo della DFT con algoritmo FFT Cooley-Tukey (radix-2 DIT)
// https://en.wikipedia.org/wiki/Cooley-Tukey_FFT_algorithm

#include "cpx.h"
#include "sizeconv.h"

#include <cstdio>
#include <ctime>

template <typename T> const char *serial_recursive_fft_algoName();

template <> const char *serial_recursive_fft_algoName<float>()
{ return "Serial recursive FFT (real-to-complex)"; }

template <> const char *serial_recursive_fft_algoName<cpx>()
{ return "Serial recursive FFT (complex-to-complex)"; }

template <typename T>
static vector<cpx> serial_recursive_fft_internal(const vector<T> &data)
{
	const int N = data.size();

	if (N == 1)
	{
		return vector<cpx>(1, cpx(data[0]));
	}
	else
	{
		vector<T> even_inputs(N/2);
		vector<T> odd_inputs(N/2);

		for (int k = 0; k < N/2; ++k)
		{
			even_inputs[k] = data[2*k];
			odd_inputs[k] = data[2*k+1];
		}

		const vector<cpx> even_ft = serial_recursive_fft_internal(even_inputs);
		const vector<cpx> odd_ft = serial_recursive_fft_internal(odd_inputs);

		vector<cpx> res(N);

		for (int k = 0; k < N/2; ++k)
		{
			const cpx p1 = even_ft[k];
			const cpx p2 = odd_ft[k] * exp(cpx(0, M_PI/N*(double)-2*k));

			res[k] = p1 + p2;
			res[k + N/2] = p1 - p2;
		}

		return res;
	}
}

template <typename T>
vector<cpx> serial_recursive_fft(const vector<T> &data)
{
	const int N = data.size();

	const clock_t start = clock();
	const vector<cpx> result = serial_recursive_fft_internal(data);

	const clock_t end = clock();
	const float secs = (end - start) / float(CLOCKS_PER_SEC);
	const float memSizeKiB = (N*sizeof(T) + 3*N*sizeof(cpx)) * log2(N) / SIZECONV_MB;
	fprintf(stderr, "%s [N=%d]: %g ms, %g MiB/s, %g samples/s\n",
		serial_recursive_fft_algoName<T>(),
		N, secs * 1e3, memSizeKiB / secs, N / secs);

	return result;
}
