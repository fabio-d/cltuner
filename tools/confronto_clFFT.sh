#!/bin/sh

# Example confronto_clFFT.sh path/to/build/dir

THISDIR="$(dirname "$0")"
BUILDDIR="$1"
export TIMES=15

CSVDATA="$(
	echo DimInput,Mio,clFFT;
	for N in 2 4 8 16 32 64 128 256 512 1024 2048 4096 8192 16384 32768 65536;
	do
		MIO=$("$THISDIR/time_median.sh" "$BUILDDIR/test-cl_fft" complex $N check)
		clFFT=$("$THISDIR/time_median.sh" "$BUILDDIR/test-clFFT" complex $N check)
		echo "$N,$MIO,$clFFT"
	done
)"
echo "$CSVDATA"
