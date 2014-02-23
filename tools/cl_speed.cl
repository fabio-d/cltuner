__kernel
void mem_read(__global volatile DATATYPE *buffer, int num_it, __global DATATYPE *dummyBuffer)
{
	const int k = get_global_id(0);
	DATATYPE tmp = 0;

	while (num_it-- != 0)
		tmp += buffer[k];

	*dummyBuffer = tmp;
}

__kernel
void mem_write(__global volatile DATATYPE *buffer, int num_it, __global DATATYPE *dummyBuffer)
{
	const int k = get_global_id(0);

	while (num_it-- != 0)
		buffer[k] = 0;
}
