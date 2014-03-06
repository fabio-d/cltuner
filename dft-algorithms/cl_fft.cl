#include "cpx.cl"

const sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_NONE | CLK_FILTER_NEAREST;

int2 wrap_index(int idx)
{
	// 4096 è la min. texture size in OpenCL 1.0, ed è anche una
	// costante, quindi il compilatore può ottimizzare questa operazione
	return (int2)(idx % 4096, idx / 4096);
}

__kernel
void fftstep_init(__write_only image2d_t twiddle_factors)
{
	const float arg = -2.0 * M_PI_F * get_global_id(0) / (2 * get_global_size(0));

	float c, s = sincos(arg, &c);
	write_imagef(twiddle_factors, wrap_index(get_global_id(0)), (float4)(c, s, 0, 0));
}

__kernel
void fftstep_cpx2cpx(__global cpx *vec_in, __global cpx *vec_out, __read_only image2d_t twiddle_factors, int Wshift, int Nhalf)
{
	const int src_x = get_global_id(0);
	const int k = get_global_id(1);

	const cpx twiddle_factor = read_imagef(twiddle_factors, sampler, wrap_index(k << Wshift)).xy;
	const int dest_row_idx = (k << Wshift);

	const int dest_i = dest_row_idx + src_x;
	const int src1_idx = dest_row_idx + dest_i; // == 2*dest_row_idx + src_x
	const int src2_idx = src1_idx + (1 << Wshift);

	const cpx p1 = vec_in[src1_idx];
	const cpx p2 = cmult(vec_in[src2_idx], twiddle_factor);

	vec_out[dest_i] = p1 + p2;
	vec_out[dest_i + Nhalf] = p1 - p2;
}

__kernel
void fftstep_real2cpx(__global float *vec_in, __global cpx *vec_out, __read_only image2d_t twiddle_factors, int Wshift, int Nhalf)
{
	const int src_x = get_global_id(0);
	const int k = get_global_id(1);

	const cpx twiddle_factor = read_imagef(twiddle_factors, sampler, wrap_index(k << Wshift)).xy;
	const int dest_row_idx = (k << Wshift);

	const int dest_i = dest_row_idx + src_x;
	const int src1_idx = dest_row_idx + dest_i; // == 2*dest_row_idx + src_x
	const int src2_idx = src1_idx + (1 << Wshift);

	const cpx p1 = (cpx)(vec_in[src1_idx], 0);
	const cpx p2 = vec_in[src2_idx] * twiddle_factor;

	vec_out[dest_i] = p1 + p2;
	vec_out[dest_i + Nhalf] = p1 - p2;
}

inline int pitched_idx(int idx, int W)
{
	return idx + idx/16 * (W % 16);
}

__kernel
void fftstep_optibase(__global cpx *vec_in, __global cpx *vec_out, __read_only image2d_t twiddle_factors, int Wshift)
{
	const int y_inizio = get_group_id(0) * OPTIBASE_GS;
	cpx tmp;

	__local float scratch_real[OPTIBASE_GS * OPTIBASE_GS * 2];
	__local float scratch_imag[OPTIBASE_GS * OPTIBASE_GS * 2];
	tmp = vec_in[y_inizio * OPTIBASE_GS + get_local_id(0)];

	while (true)
	{
		const int stride = 1 << Wshift;

		// Se siamo a una riga dispari dobbiamo applicare i twiddle_factors
		const int n_riga = get_local_id(0) / stride;
		const int k = (y_inizio + n_riga % OPTIBASE_GS) / 2 + (n_riga / OPTIBASE_GS) * (OPTIBASE_GS/2) * get_num_groups(0);
		cpx twiddle_factor = read_imagef(twiddle_factors, sampler, wrap_index(k * stride)).xy;
		tmp = cmult(tmp, select(twiddle_factor, (cpx)(1, 0), (int2)(n_riga % 2 - 1)));

		const int destidx = pitched_idx(get_local_id(0), stride);
		scratch_real[destidx] = tmp.x;
		scratch_imag[destidx] = tmp.y;

		// Attende che i dati in input siano pronti
		barrier(CLK_LOCAL_MEM_FENCE);

		const int src_x = get_local_id(0) % stride;
		const int src_y1 = (2 * n_riga) % (OPTIBASE_GS * OPTIBASE_GS / stride);
		const int src_idx1 = src_y1 * stride + src_x;
		const int src_pidx1 = pitched_idx(src_idx1, stride);
		const int src_pidx2 = pitched_idx(src_idx1 + stride, stride);
		const cpx p1 = (cpx)(scratch_real[src_pidx1], scratch_imag[src_pidx1]);
		const cpx p2 = (cpx)(scratch_real[src_pidx2], scratch_imag[src_pidx2]);

		if (get_local_id(0) < OPTIBASE_GS * OPTIBASE_GS / 2)
			tmp = p1 + p2;
		else
			tmp = p1 - p2;

		if (Wshift-- == 0)
			break;

		// Attende che tutti abbiano finito di consumare l'input prima di scrivere il risultato
		barrier(CLK_LOCAL_MEM_FENCE);
	}

	const int n_colonna = get_local_id(0) % (OPTIBASE_GS / 2);
	const int n_riga = get_local_id(0) / (OPTIBASE_GS / 2);
	const int destidx = (y_inizio + n_riga * get_num_groups(0) * OPTIBASE_GS) / 2 + n_colonna;
	vec_out[destidx] = tmp;
}
