#!/bin/sh

# Example TIMES=5 tools/time_median.sh test-cl_naive_dft complex 4096 0 0

export PRINT_KERNEL_EXECUTION_TIME=yes

EXPR=$({
	for i in $(seq 1 $TIMES); do
		echo "== Esecuzione $i/$TIMES ==" >&2
		"$@"
		echo >&2
	done
})

SORTED="$(echo "$EXPR" | sort -g)"
MEDIAN="$(echo "$SORTED" | sed -n "$(($TIMES/2+1))p")"

echo "In ordine crescente:" $SORTED >&2
echo "Mediana:" $MEDIAN >&2
echo $MEDIAN
