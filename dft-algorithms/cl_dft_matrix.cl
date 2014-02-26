#include "cpx.cl"

__kernel
void mtx_init(__global cpx *coeffs, int N, float parteFissa)
{
	const int x = get_global_id(0);
	const int y = get_global_id(1);

	if (x >= N || y >= N)
		return;

	float c, s = sincos(parteFissa*x*y, &c);
	coeffs[y*N + x] = (cpx)(c, s);
}

__kernel
void mtx_cpx2cpx(__global cpx *samples, __global cpx *result, int N, __global cpx *coeffs)
{
	const int k = get_global_id(0);

	if (k >= N)
		return;

	cpx sum = (cpx)(0, 0);

	for (int j = 0; j < N; j++)
		sum += cmult(samples[j], coeffs[j*N + k]);

	result[k] = sum;
}

__kernel
void mtx_real2cpx(__global float *samples, __global cpx *result, int N, __global cpx *coeffs)
{
	const int k = get_global_id(0);

	if (k >= N)
		return;

	cpx sum = (cpx)(0, 0);

	for (int j = 0; j < N; j++)
		sum += samples[j] * coeffs[j*N + k];

	result[k] = sum;
}
