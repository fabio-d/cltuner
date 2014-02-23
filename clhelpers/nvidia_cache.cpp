#include "cl.h"

#include <cstdlib>

void clhEmptyNvidiaCache()
{
	// svuota cache del compilatore NVIDIA
	system("rm -rf ~/.nv/ComputeCache/*");
}
