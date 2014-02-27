#include "cpx.cl"
#include "isqrt.cl"

__kernel
void symmtx_init(__global cpx *coeffs, int N, float parteFissa)
{
	const int x = get_global_id(0);
	const int y = get_global_id(1);

	if (x >= N || y >= N)
		return;

	float c, s = sincos(parteFissa*x*y, &c);
	coeffs[y*N + x] = (cpx)(c, s);
}

__kernel
void step1_cpx2cpx(__global cpx *samples, __global cpx *subtot, int N, __global cpx *coeffs)
{
	const int id_x = get_local_id(0);
	const int id_y = get_local_id(1);
	const int group_x = get_group_id(0);
	const int group_y = get_group_id(1);

	if (group_x > group_y)
		return;

	__local float samplesA_real[GS];
	__local float samplesA_imag[GS];

	__local float samplesB_real[GS];
	__local float samplesB_imag[GS];

	/* Matrice utilizzata inizialmente per caricare i coefficienti, poi come
	 * supporto alla riduzione */
	__local float scratch_real[GS][GS+1];
	__local float scratch_imag[GS][GS+1];

	cpx tmp;

	// Caricamento coefficienti
	const int coeff_x = group_x * GS + id_x;
	const int coeff_y = group_y * GS + id_y;
	tmp = coeffs[coeff_y * N + coeff_x];
	scratch_real[id_y][id_x] = tmp.x;
	scratch_imag[id_y][id_x] = tmp.y;

	// Caricamento dei samples
	if (id_y == 0)
	{
		tmp = samples[group_y * GS + id_x];
		samplesA_real[id_x] = tmp.x;
		samplesA_imag[id_x] = tmp.y;
	}
	else if (id_y == 1)
	{
		tmp = samples[group_x * GS + id_x];
		samplesB_real[id_x] = tmp.x;
		samplesB_imag[id_x] = tmp.y;
	}

	barrier(CLK_LOCAL_MEM_FENCE);

	// Calcolo di due prodotti e della loro somma
	if (id_y < GS/2)
	{
		// Consultiamo la tabella dei sample regolarmente
		tmp = cmult(
			(cpx)(samplesA_real[id_y], samplesA_imag[id_y]),
			(cpx)(scratch_real[id_y][id_x], scratch_imag[id_y][id_x]) )
		    + cmult(
			(cpx)(samplesA_real[id_y + GS/2], samplesA_imag[id_y + GS/2]),
			(cpx)(scratch_real[id_y + GS/2][id_x], scratch_imag[id_y + GS/2][id_x]) );
	}
	else
	{
		// Consultiamo la tabella dei sample invertendo x e y
		tmp = cmult(
			(cpx)(samplesB_real[id_y - GS/2], samplesB_imag[id_y - GS/2]),
			(cpx)(scratch_real[id_x][id_y - GS/2], scratch_imag[id_x][id_y - GS/2]) )
		    + cmult(
			(cpx)(samplesB_real[id_y], samplesB_imag[id_y]),
			(cpx)(scratch_real[id_x][id_y], scratch_imag[id_x][id_y]) );
	}
	barrier(CLK_LOCAL_MEM_FENCE);

	// Scriviamo il risultato
	scratch_real[id_y][id_x] = tmp.x;
	scratch_imag[id_y][id_x] = tmp.y;

	// Somma (riduzione)
	// La prima metà dei thread calcola le somme relative ai sample A, la seconda
	// metà quelle relative ai sample B
	int active_threads = GS/2;
	while (active_threads != 1)
	{
		barrier(CLK_LOCAL_MEM_FENCE);

		active_threads /= 2;

		if (id_y < GS/2 && id_y < active_threads)
		{
			scratch_real[id_y][id_x] += scratch_real[id_y + active_threads][id_x];
			scratch_imag[id_y][id_x] += scratch_imag[id_y + active_threads][id_x];
		}
		else if (id_y >= GS/2 && id_y - GS/2 < active_threads)
		{
			scratch_real[id_y][id_x] += scratch_real[id_y + active_threads][id_x];
			scratch_imag[id_y][id_x] += scratch_imag[id_y + active_threads][id_x];
		}
	}

	if (id_y == 0)
		subtot[group_y * N + (group_x * GS + id_x)] = (cpx)(scratch_real[id_y][id_x], scratch_imag[id_y][id_x]);
	else if (id_y == GS/2 && group_x != group_y /* evita di riscrivere se il blocco si trova sulla diagonale */)
		subtot[group_x * N + (group_y * GS + id_x)] = (cpx)(scratch_real[id_y][id_x], scratch_imag[id_y][id_x]);
}

__kernel
void step1_real2cpx(__global float *samples, __global cpx *result, int N, __global cpx *coeffs, __local float *temp)
{
#if 0
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
#endif
}

__kernel
void step2(__global cpx *subtot, __global cpx *result, int N, int rows)
{
	const int k = get_global_id(0);

	if (k >= N)
		return;

	cpx sum = (cpx)(0, 0);

	for (int j = 0; j < rows; j++)
		sum += subtot[j*N + k];

	result[k] = sum;
}
