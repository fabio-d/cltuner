#include "cpx.cl"

const sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_NONE | CLK_FILTER_NEAREST;

__kernel
void mtx_init(__write_only image2d_t coeffs, int N, float parteFissa)
{
	const int x = get_global_id(0);
	const int y = get_global_id(1);

	if (x >= N || y >= N)
		return;

	float c, s = sincos(parteFissa*x*y, &c);
	write_imagef(coeffs, (int2)(x, y), (float4)(c, s, 0, 0));
}

__kernel
void mtx_cpx2cpx(__read_only image2d_t samples, __global cpx *result, int N, __read_only image2d_t coeffs)
{
	const int k = get_global_id(0);

	if (k >= N)
		return;

	cpx sum = (cpx)(0, 0);

	for (int j = 0; j < N; j++)
		sum += cmult(
			read_imagef(samples, sampler, (int2)(1, j)).xy,
			read_imagef(coeffs, sampler, (int2)(k, j)).xy );

	result[k] = sum;
}

__kernel
void mtx_real2cpx(__read_only image2d_t samples, __global cpx *result, int N, __read_only image2d_t coeffs)
{
	const int k = get_global_id(0);

	if (k >= N)
		return;

	cpx sum = (cpx)(0, 0);

	for (int j = 0; j < N; j++)
		sum += read_imagef(samples, sampler, (int2)(1, j)).x
			* read_imagef(coeffs, sampler, (int2)(k, j)).xy;

	result[k] = sum;
}
