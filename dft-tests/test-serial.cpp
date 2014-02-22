#include "testlib.h"

#include "dft-algorithms/serial_naive_dft.cpp"
#include "dft-algorithms/serial_recursive_fft.cpp"
#include "dft-algorithms/serial_nonrecursive_fft.cpp"

#include <cstdlib>
#include <cstring>
#include <iostream>

template <typename T>
static int runTest(vector<cpx> (*func)(const vector<T>&), int N, bool print, bool check)
{
	const vector<T> input = generateTestData<T>(N);

	const vector<cpx> output = func(input);

	if (print)
		cerr << output << endl;

	if (check)
	{
		fprintf(stderr, "Calcolo serial naive DFT per riferimento...\n");
		fprintf(stderr, "Distanza massima: %g\n", maxAbsDistance(output, serial_naive_dft(input)));

	}

	return EXIT_SUCCESS;
}

int main(int argc, const char **argv)
{
	int input_size;
	bool use_complex_inputs;
	bool print = false;
	bool check = false;

	if (
		!(argc == 3
			|| (argc == 4 && (print = !strcmp(argv[3], "print")))
#ifdef ENABLE_CHECK
			|| (argc == 4 && (check = !strcmp(argv[3], "check")))
#endif
		)
		|| (use_complex_inputs = !!strcmp(argv[1], "real")) == !!strcmp(argv[1], "complex")
		|| (input_size = atoi(argv[2])) <= 0)
	{
#ifdef ENABLE_CHECK
		const char *check_text = " | check";
#else
		const char *check_text = "";
#endif

#ifdef ALLOW_NPOT
		const char *pot_text = "";
#else
		const char *pot_text = " (potenza di due)";
#endif

		cerr << "Uso: " << argv[0] << " <real | complex> input-size [print" << check_text << "]" << endl << endl;
		cerr << " real             Usa input di tipo reale" << endl;
		cerr << " complex          Usa input di tipo complesso" << endl;
		cerr << " input-size       Numero di samples da passare in input" << pot_text << endl;
		cerr << " print            Mostra output" << endl;
#ifdef ENABLE_CHECK
		cerr << " check            Confronta output con serial naive DFT" << endl;
#endif

		cerr << endl;
		return EXIT_FAILURE;
	}

#ifndef ALLOW_NPOT
	if (input_size & (input_size-1))
	{
		cerr << "input-size deve essere una potenza di 2" << endl;
		return EXIT_FAILURE;
	}
#endif

	if (use_complex_inputs)
		runTest(ALGOFUNC<cpx>, input_size, print, check);
	else
		runTest(ALGOFUNC<float>, input_size, print, check);
}
