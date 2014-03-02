#include "cpx.cl"

__kernel
void fftstep_cpx2cpx(__global cpx *vec_in, __global cpx *vec_out)
{
	const int W = get_global_size(0);
	const int H = 2*get_global_size(1);
	const int N = W*H;

	const int src_x = get_global_id(0);
	const int k = get_global_id(1);

	const int dest_i = k*W + src_x;
	const int src_y1 = 2*k;
	const int src_y2 = 2*k+1;

	float c, s = sincos(k * -2.0 * M_PI_F / H, &c);
	const cpx twiddle_factor = (cpx)(c, s);

	const cpx p1 = vec_in[src_y1*W + src_x];
	const cpx p2 = cmult(vec_in[src_y2*W + src_x], twiddle_factor);

	vec_out[dest_i] = p1 + p2;
	vec_out[dest_i + N/2] = p1 - p2;
}

__kernel
void fftstep_real2cpx(__global float *vec_in, __global cpx *vec_out)
{
	const int W = get_global_size(0);
	const int H = 2*get_global_size(1);
	const int N = W*H;

	const int src_x = get_global_id(0);
	const int k = get_global_id(1);

	const int dest_i = k*W + src_x;
	const int src_y1 = 2*k;
	const int src_y2 = 2*k+1;

	float c, s = sincos(k * -2.0 * M_PI_F / H, &c);
	const cpx twiddle_factor = (cpx)(c, s);

	const cpx p1 = (cpx)(vec_in[src_y1*W + src_x], 0);
	const cpx p2 = vec_in[src_y2*W + src_x] * twiddle_factor;

	vec_out[dest_i] = p1 + p2;
	vec_out[dest_i + N/2] = p1 - p2;
}
