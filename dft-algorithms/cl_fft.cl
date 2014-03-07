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
void fftstep_optibase(__global float8 *vec_in, __global cpx *vec_out, __read_only image2d_t twiddle_factors, int Nhalf)
{
	const int k = get_global_id(0);

	const cpx twiddle_factor2 = read_imagef(twiddle_factors, sampler, wrap_index(k * 2)).xy;

	const float8 data = vec_in[k];
	const float4 p1 = data.s0123;
	const float4 p2 = (float4)(cmult(data.s45, twiddle_factor2), cmult(data.s67, twiddle_factor2));

	const float4 intermA = p1 + p2;
	const float4 intermB = p1 - p2;

	const cpx twiddle_factor1A = read_imagef(twiddle_factors, sampler, wrap_index(k)).xy;

	const cpx p2A = cmult(intermA.zw, twiddle_factor1A);
	vec_out[k] = intermA.xy + p2A;
	vec_out[Nhalf + k] = intermA.xy - p2A;

	const cpx twiddle_factor1B = read_imagef(twiddle_factors, sampler, wrap_index(Nhalf/2 + k)).xy;

	const cpx p2B = cmult(intermB.zw, twiddle_factor1B);
	vec_out[k + Nhalf/2] = intermB.xy + p2B;
	vec_out[Nhalf + k + Nhalf/2] = intermB.xy - p2B;
}
