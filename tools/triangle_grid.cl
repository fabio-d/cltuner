#include "dft-algorithms/isqrt.cl"

__kernel
void schema_naive(__global int *dummyBuffer)
{
	const int group_x = get_group_id(0);
	const int group_y = get_group_id(1);

	if (group_x > group_y)
		return;

	*dummyBuffer = group_x + group_y;
}

__kernel
void schema_triangolo_tutticalcolano(__global int *dummyBuffer)
{
	const int group_y = (isqrt(8*get_group_id(0) + 1) - 1) / 2;
	const int group_x = get_group_id(0) - group_y * (group_y + 1) / 2;

	*dummyBuffer = group_x + group_y;
}

__kernel
void schema_triangolo_solounocalcola(__global int *dummyBuffer)
{
	const int id_x = get_local_id(0);
	const int id_y = get_local_id(1);

	// Calcola le "vere" coordinate del blocco, e propaga informazione a tutti
	__local int __group_x, __group_y;
	if (id_x == 0 && id_y == 0)
	{
		__group_y = (isqrt(8*get_group_id(0) + 1) - 1) / 2;
		__group_x = get_group_id(0) - __group_y * (__group_y + 1) / 2;
	}

	barrier(CLK_LOCAL_MEM_FENCE);

	const int group_x = __group_x;
	const int group_y = __group_y;

	*dummyBuffer = group_x + group_y;
}
