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

#define OPTI(W) \
__kernel \
__attribute__((reqd_work_group_size(W, OPTISIZE_GS / W, 1))) \
void fftstep_opti ## W (__global cpx *vec_in, __global cpx *vec_out, __read_only image2d_t twiddle_factors, int Nhalf) \
{ \
	const int src_x = get_global_id(0); \
	const int k = get_global_id(1); \
\
	__local float input_real[OPTISIZE_GS * 2 * 2]; \
	__local float input_imag[OPTISIZE_GS * 2 * 2]; \
	const int load_base = get_group_id(1) * OPTISIZE_GS * 2; \
	int offset = get_local_id(1) * get_local_size(0) + get_local_id(0); \
	cpx tmp; \
	tmp = vec_in[load_base + offset]; \
	input_real[pitched_idx(offset, W)] = tmp.x; input_imag[pitched_idx(offset, W)] = tmp.y; \
	offset += OPTISIZE_GS; \
	tmp = vec_in[load_base + offset]; \
	input_real[pitched_idx(offset, W)] = tmp.x; input_imag[pitched_idx(offset, W)] = tmp.y; \
\
	const cpx twiddle_factor = read_imagef(twiddle_factors, sampler, wrap_index(k * W)).xy; \
	const int dest_row_idx = k * W; \
	const int dest_i = dest_row_idx + src_x; \
\
	barrier(CLK_LOCAL_MEM_FENCE); \
\
	const int base = get_local_id(1) * 2 * get_local_size(0) + get_local_id(0); \
	const cpx p1 = (cpx)(input_real[pitched_idx(base, W)], input_imag[pitched_idx(base, W)]); \
	const cpx p2 = cmult((cpx)(input_real[pitched_idx(base + get_local_size(0), W)], input_imag[pitched_idx(base + get_local_size(0), W)]), twiddle_factor); \
\
	vec_out[dest_i] = p1 + p2; \
	vec_out[dest_i + Nhalf] = p1 - p2; \
}

OPTI(1)
OPTI(2)
OPTI(4)
OPTI(8)
