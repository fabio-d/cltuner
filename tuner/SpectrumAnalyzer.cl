__kernel
void circsum(__global float *dest, __global float *src, int offset, int length, int presumWindows)
{
	float sum = 0;

	offset += get_global_id(0);

	while (presumWindows-- != 0)
	{
		offset = (offset + length - get_global_size(0)) % length;
		sum += src[offset];
	}

	dest[get_global_id(0)] = sum;
}
