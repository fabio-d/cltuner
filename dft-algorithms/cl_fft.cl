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
void fftstep_cpx2cpx(__global cpx *vec_in, __global cpx *vec_out, __read_only image2d_t twiddle_factors)
{
	const int W = get_global_size(0);
	const int H = 2*get_global_size(1);
	const int N = W*H;

	const int src_x = get_global_id(0);
	const int k = get_global_id(1);

	const int dest_i = k*W + src_x;
	const int src_y1 = 2*k;
	const int src_y2 = 2*k+1;

	const cpx twiddle_factor = read_imagef(twiddle_factors, sampler, wrap_index(k*W)).xy;

	const cpx p1 = vec_in[src_y1*W + src_x];
	const cpx p2 = cmult(vec_in[src_y2*W + src_x], twiddle_factor);

	vec_out[dest_i] = p1 + p2;
	vec_out[dest_i + N/2] = p1 - p2;
}

__kernel
void fftstep_real2cpx(__global float *vec_in, __global cpx *vec_out, __read_only image2d_t twiddle_factors)
{
	const int W = get_global_size(0);
	const int H = 2*get_global_size(1);
	const int N = W*H;

	const int src_x = get_global_id(0);
	const int k = get_global_id(1);

	const int dest_i = k*W + src_x;
	const int src_y1 = 2*k;
	const int src_y2 = 2*k+1;

	const cpx twiddle_factor = read_imagef(twiddle_factors, sampler, wrap_index(k*W)).xy;

	const cpx p1 = (cpx)(vec_in[src_y1*W + src_x], 0);
	const cpx p2 = vec_in[src_y2*W + src_x] * twiddle_factor;

	vec_out[dest_i] = p1 + p2;
	vec_out[dest_i + N/2] = p1 - p2;
}
