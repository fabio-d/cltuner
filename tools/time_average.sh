#!/bin/sh

# Example TIMES=5 tools/time_average.sh test-cl_naive_dft complex 4096 0 0

export PRINT_KERNEL_EXECUTION_TIME=yes

EXPR=$({
	echo "("

	for i in $(seq 1 $TIMES); do
		echo "== Esecuzione $i/$TIMES ==" >&2
		if [ "$i" != "1" ];
		then
			echo "+"
		fi
		"$@"
		echo >&2
	done

	echo ") / $TIMES"
})

AVG="$({ echo "scale=3"; echo $EXPR; } | bc)"
echo $EXPR = $AVG >&2
echo $AVG
