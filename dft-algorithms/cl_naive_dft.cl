#include "cpx.cl"

__kernel
void dft_cpx2cpx(__global cpx *samples, __global cpx *result, int N)
{
	const int k = get_global_id(0);

	if (k >= N)
		return;

	cpx sum = (cpx)(0, 0);

	for (int j = 0; j < N; j++)
		sum += cmult(samples[j], cexp((cpx)(0, -2.0*j*k*M_PI_F/N)));

	result[k] = sum;
}

__kernel
void dft_real2cpx(__global float *samples, __global cpx *result, int N)
{
	const int k = get_global_id(0);

	if (k >= N)
		return;

	cpx sum = (cpx)(0, 0);

	for (int j = 0; j < N; j++)
		sum += samples[j] * cexp((cpx)(0, -2.0*j*k*M_PI_F/N));

	result[k] = sum;
}
