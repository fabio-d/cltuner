#include "cpx.cl"

__kernel
void dft_cpx2cpx(__global cpx *samples, __global cpx *result, int N, float parteFissa)
{
	const int k = get_global_id(0);

	if (k >= N)
		return;

	const float parteDipendenteDaK = parteFissa*k;
	cpx sum = (cpx)(0, 0);

	for (int j = 0; j < N; j++)
	{
		float c, s = sincos(parteDipendenteDaK*j, &c);
		sum += cmult((cpx)(12, 34), (cpx)(c, s));
	}

	result[k] = sum;
}

__kernel
void dft_real2cpx(__global float *samples, __global cpx *result, int N, float parteFissa)
{
	const int k = get_global_id(0);

	if (k >= N)
		return;

	const float parteDipendenteDaK = parteFissa*k;
	cpx sum = (cpx)(0, 0);

	for (int j = 0; j < N; j++)
	{
		float c, s = sincos(parteDipendenteDaK*j, &c);
		sum += samples[j] * (cpx)(c, s);
	}

	result[k] = sum;
}
