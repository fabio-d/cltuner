#!/bin/sh

# Example seq 32 32 512 | tools/test_workgroup_sizes.sh test-cl_naive_dft complex 4096 0 0 2>/dev/null

export PRINT_KERNEL_EXECUTION_TIME=yes

while read GS_X; do
	export GS_X
	EXEC_TIME_A=$("$@")
	EXEC_TIME_B=$("$@")
	EXEC_TIME_C=$("$@")
	EXEC_TIME_D=$("$@")
	EXEC_TIME_E=$("$@")
	EXEC_TIME_F=$("$@")
	echo $GS_X,$(echo '(' $EXEC_TIME_A+$EXEC_TIME_B+$EXEC_TIME_C+$EXEC_TIME_D+$EXEC_TIME_E+$EXEC_TIME_F ')/6' | bc)
done
