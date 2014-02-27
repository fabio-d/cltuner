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
	if (coeff_y * N + coeff_x < N*N)
	{
		tmp = coeffs[coeff_y * N + coeff_x];
		scratch_real[id_y][id_x] = tmp.x;
		scratch_imag[id_y][id_x] = tmp.y;
	}

	// Caricamento dei samples
	if (id_y == 0)
	{
		const int load_idx = group_y * GS + id_x;
		if (load_idx < N)
		{
			tmp = samples[load_idx];
			samplesA_real[id_x] = tmp.x;
			samplesA_imag[id_x] = tmp.y;
		}
		else
		{
			samplesA_real[id_x] = 0;
			samplesA_imag[id_x] = 0;
		}
	}
	else if (id_y == 1)
	{
		const int load_idx = group_x * GS + id_x;
		if (load_idx < N)
		{
			tmp = samples[load_idx];
			samplesB_real[id_x] = tmp.x;
			samplesB_imag[id_x] = tmp.y;
		}
		else
		{
			samplesB_real[id_x] = 0;
			samplesB_imag[id_x] = 0;
		}
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

	int store_y, store_x = 0x7FFFFFFF /* INT_MAX */;

	if (id_y == 0)
	{
		store_x = group_x * GS + id_x;
		store_y = group_y;
	}
	else if (id_y == GS/2 && group_x != group_y /* evita di riscrivere se il blocco si trova sulla diagonale */)
	{
		store_x = group_y * GS + id_x;
		store_y = group_x;
	}

	if (store_x < N)
		subtot[store_y * N + store_x] = (cpx)(scratch_real[id_y][id_x], scratch_imag[id_y][id_x]);
}

__kernel
void step1_real2cpx(__global float *samples, __global cpx *subtot, int N, __global cpx *coeffs)
{
	const int id_x = get_local_id(0);
	const int id_y = get_local_id(1);
	const int group_x = get_group_id(0);
	const int group_y = get_group_id(1);

	if (group_x > group_y)
		return;

	__local float samplesA_real[GS];
	__local float samplesB_real[GS];

	/* Matrice utilizzata inizialmente per caricare i coefficienti, poi come
	 * supporto alla riduzione */
	__local float scratch_real[GS][GS+1];
	__local float scratch_imag[GS][GS+1];

	cpx tmp;

	// Caricamento coefficienti
	const int coeff_x = group_x * GS + id_x;
	const int coeff_y = group_y * GS + id_y;
	if (coeff_y * N + coeff_x < N*N)
	{
		tmp = coeffs[coeff_y * N + coeff_x];
		scratch_real[id_y][id_x] = tmp.x;
		scratch_imag[id_y][id_x] = tmp.y;
	}

	// Caricamento dei samples
	if (id_y == 0)
	{
		const int load_idx = group_y * GS + id_x;
		if (load_idx < N)
			samplesA_real[id_x] = samples[load_idx];
		else
			samplesA_real[id_x] = 0;
	}
	else if (id_y == 1)
	{
		const int load_idx = group_x * GS + id_x;
		if (load_idx < N)
			samplesB_real[id_x] = samples[load_idx];
		else
			samplesB_real[id_x] = 0;
	}

	barrier(CLK_LOCAL_MEM_FENCE);

	// Calcolo di due prodotti e della loro somma
	if (id_y < GS/2)
	{
		// Consultiamo la tabella dei sample regolarmente
		tmp = ( samplesA_real[id_y] * (cpx)(scratch_real[id_y][id_x], scratch_imag[id_y][id_x]) )
		    + ( samplesA_real[id_y + GS/2] * (cpx)(scratch_real[id_y + GS/2][id_x], scratch_imag[id_y + GS/2][id_x]) );
	}
	else
	{
		// Consultiamo la tabella dei sample invertendo x e y
		tmp = ( samplesB_real[id_y - GS/2] * (cpx)(scratch_real[id_x][id_y - GS/2], scratch_imag[id_x][id_y - GS/2]) )
		    + ( samplesB_real[id_y] * (cpx)(scratch_real[id_x][id_y], scratch_imag[id_x][id_y]) );
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

	int store_y, store_x = 0x7FFFFFFF /* INT_MAX */;

	if (id_y == 0)
	{
		store_x = group_x * GS + id_x;
		store_y = group_y;
	}
	else if (id_y == GS/2 && group_x != group_y /* evita di riscrivere se il blocco si trova sulla diagonale */)
	{
		store_x = group_y * GS + id_x;
		store_y = group_x;
	}

	if (store_x < N)
		subtot[store_y * N + store_x] = (cpx)(scratch_real[id_y][id_x], scratch_imag[id_y][id_x]);
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
