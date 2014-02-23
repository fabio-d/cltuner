__kernel
void mem_copy(__global int *dest, __global int *src)
{
	const int k = get_global_id(0);
	dest[k] = src[k];
}
