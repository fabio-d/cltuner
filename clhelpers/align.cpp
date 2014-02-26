#include "cl.h"

size_t clhAlignUp(size_t value, size_t alignment)
{
	value = (value + alignment - 1) / alignment;
	return value * alignment;
}
