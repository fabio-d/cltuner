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

__kernel
void fftstep_opti1(__global float4 *vec_in, __global cpx *vec_out, __read_only image2d_t twiddle_factors, int Nhalf)
{
	// assumiamo get_global_id(0) == 0
	const int k = get_global_id(1);

	const cpx twiddle_factor = read_imagef(twiddle_factors, sampler, wrap_index(k)).xy;

	const float4 data = vec_in[k];
	const cpx p1 = data.xy;
	const cpx p2 = cmult(data.zw, twiddle_factor);

	vec_out[k] = p1 + p2;
	vec_out[k + Nhalf] = p1 - p2;
}

__kernel
void fftstep_opti2(__global float8 *vec_in, __global float4 *vec_out, __read_only image2d_t twiddle_factors, int Nhalf)
{
	const int k = get_global_id(1);

	const cpx twiddle_factor = read_imagef(twiddle_factors, sampler, wrap_index(k * 2)).xy;

	const float8 data = vec_in[k];
	const float4 p1 = data.s0123;
	const float4 p2 = (float4)(cmult(data.s45, twiddle_factor), cmult(data.s67, twiddle_factor));

	vec_out[k] = p1 + p2;
	vec_out[k + Nhalf] = p1 - p2;
}

__kernel
void fftstep_opti4(__global float16 *vec_in, __global float8 *vec_out, __read_only image2d_t twiddle_factors, int Nhalf)
{
	const int k = get_global_id(1);

	const cpx twiddle_factor = read_imagef(twiddle_factors, sampler, wrap_index(k * 4)).xy;

	const float16 data = vec_in[k];
	const float8 p1 = data.s01234567;
	const float8 p2 = (float8)(
		cmult(data.s89, twiddle_factor),
		cmult(data.sAB, twiddle_factor),
		cmult(data.sCD, twiddle_factor),
		cmult(data.sEF, twiddle_factor));

	vec_out[k] = p1 + p2;
	vec_out[k + Nhalf] = p1 - p2;
}
