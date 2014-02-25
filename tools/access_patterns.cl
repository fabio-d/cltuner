__kernel
void same_location(__global DATATYPE *buffer, __global DATATYPE *dummyBuffer)
{
	*dummyBuffer = buffer[0];
}

__kernel
void aligned_sequential(__global DATATYPE *buffer, __global DATATYPE *dummyBuffer)
{
	*dummyBuffer = buffer[get_local_id(0)];
}

__kernel
void aligned_nonsequential(__global DATATYPE *buffer, __global DATATYPE *dummyBuffer)
{
	int idx = get_local_id(0);

	if (idx == 3)		idx = 4;
	else if (idx == 4)	idx = 3;
	else if (idx == 25)	idx = 26;
	else if (idx == 26)	idx = 25;

	*dummyBuffer = buffer[idx];
}

__kernel
void misaligned_sequential(__global DATATYPE *buffer, __global DATATYPE *dummyBuffer)
{
	*dummyBuffer = buffer[get_local_id(0) + 1];
}
