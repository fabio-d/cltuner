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
void mtx_cpx2cpx(__global cpx *samples, __global cpx *result, int N, __global cpx *coeffs, __local float *temp_r, __local float *temp_i)
{
	const int k = get_global_id(0);
	const int id = get_local_id(0);
	const int GS = get_local_size(0);

	cpx sum = (cpx)(0, 0);

	for (int j_start = 0; j_start < N; j_start += GS)
	{
		const int j_end = min(N, j_start + GS);

		if (j_start != 0) // Attende che il ciclo precedente sia stato completato
			barrier(CLK_LOCAL_MEM_FENCE);

		if (j_start + id < j_end)
		{
			cpx v = samples[j_start + id];
			temp_r[id] = v.x; temp_i[id] = v.y;
		}
		barrier(CLK_LOCAL_MEM_FENCE);

		if (k < N)
		{
			for (int j = j_start; j < j_end; j++)
				sum += cmult((cpx)(temp_r[j - j_start], temp_i[j - j_start]), coeffs[j*N + k]);
		}
	}

	if (k < N)
		result[k] = sum;
}

__kernel
void mtx_real2cpx(__global float *samples, __global cpx *result, int N, __global cpx *coeffs, __local float *temp)
{
	const int k = get_global_id(0);
	const int id = get_local_id(0);
	const int GS = get_local_size(0);

	cpx sum = (cpx)(0, 0);

	for (int j_start = 0; j_start < N; j_start += 2*GS)
	{
		const int j_end = min(N, j_start + 2*GS);

		if (j_start != 0) // Attende che il ciclo precedente sia stato completato
			barrier(CLK_LOCAL_MEM_FENCE);

		// Carica in transazioni da 128b, facendo attenzione all'ultimo elemento
		if (j_start + 2*id < j_end)
			((__local float2*)temp)[id] = ((__global float2*)(samples + j_start))[id];
		else if (j_start + 2*id == j_end)
			temp[2*id - 1] = samples[j_start + 2*id - 1];
		barrier(CLK_LOCAL_MEM_FENCE);

		if (k < N)
		{
			for (int j = j_start; j < j_end; j++)
				sum += temp[j - j_start] * coeffs[j*N + k];
		}
	}

	if (k < N)
		result[k] = sum;
}
