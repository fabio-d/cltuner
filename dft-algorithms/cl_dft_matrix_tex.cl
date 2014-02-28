#include "cpx.cl"

const sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_NONE | CLK_FILTER_NEAREST;

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
void mtx_cpx2cpx(__read_only image2d_t samples, __global cpx *result, int N, __global cpx *coeffs)
{
	const int k = get_global_id(0);

	if (k >= N)
		return;

	cpx sum = (cpx)(0, 0);

	for (int j = 0; j < N; j++)
		sum += cmult(read_imagef(samples, sampler, (int2)(j, 1)).xy, coeffs[j*N + k]);

	result[k] = sum;
}

__kernel
void mtx_real2cpx(__read_only image2d_t samples, __global cpx *result, int N, __global cpx *coeffs)
{
	const int k = get_global_id(0);

	if (k >= N)
		return;

	cpx sum = (cpx)(0, 0);

	for (int j = 0; j < N; j++)
		sum += read_imagef(samples, sampler, (int2)(j, 1)).x * coeffs[j*N + k];

	result[k] = sum;
}
