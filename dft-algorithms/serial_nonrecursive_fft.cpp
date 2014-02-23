// Algoritmo FFT equivalente al Cooley-Tukey (radix-2 DIT)

#include "cpx.h"
#include "pi_float.h"
#include "sizeconv.h"

#include <cstdio>
#include <ctime>

template <typename T> const char *serial_nonrecursive_fft_algoName();

template <> const char *serial_nonrecursive_fft_algoName<float>()
{ return "Serial non-recursive FFT (real-to-complex)"; }

template <> const char *serial_nonrecursive_fft_algoName<cpx>()
{ return "Serial non-recursive FFT (complex-to-complex)"; }

template <typename T>
void serial_nonrecursive_fft_step(const vector<T> &vec_in, vector<cpx> &vec_out, int W)
{
	const int N = vec_in.size();
	const int H = N/W;
	int dest_i = 0;

	for (int k = 0; k < H/2; k++)
	{
		const int src_y1 = 2*k;
		const int src_y2 = 2*k+1;
		const cpx twiddle_factor = exp(cpx(0, M_PI_F/H*-2.f*k));

		for (int src_x = 0; src_x < W; src_x++)
		{
			cpx p1 = vec_in[src_y1*W + src_x];
			cpx p2 = vec_in[src_y2*W + src_x] * twiddle_factor;

			vec_out[dest_i] = p1 + p2;
			vec_out[dest_i + N/2] = p1 - p2;
			dest_i++;
		}
	}
}

template <typename T>
vector<cpx> serial_nonrecursive_fft(const vector<T> &data)
{
	const int N = data.size();

	vector<cpx> v1(N), v2(N);
	vector<cpx> *result = &v1, *tmp = &v2;

	const clock_t start = clock();
	size_t memSize = 0;

	int W = N/2;
	if (W != 0)
	{
		serial_nonrecursive_fft_step(data, *result, W);
		memSize += 2*N*sizeof(T) + 2*N*sizeof(cpx);
		while ((W /= 2) != 0)
		{
			serial_nonrecursive_fft_step(*result, *tmp, W);
			memSize += 2*N*sizeof(cpx) + 2*N*sizeof(cpx);
			swap(result, tmp);
		}
	}

	const clock_t end = clock();
	const float secs = (end - start) / float(CLOCKS_PER_SEC);
	const float memSizeKiB = memSize / SIZECONV_MB;
	fprintf(stderr, "%s [N=%d]: %g ms, %g MiB/s, %g samples/s\n",
		serial_nonrecursive_fft_algoName<T>(),
		N, secs * 1e3, memSizeKiB / secs, N / secs);

	return *result;
}
