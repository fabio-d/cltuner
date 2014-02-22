#include "testlib.h"

#include "dft-algorithms/serial_naive_dft.cpp"
#include "dft-algorithms/serial_nonrecursive_fft.cpp"

#include "dft-algorithms/cl_naive_dft.cpp"

#include <cstdlib>
#include <cstring>
#include <cctype>
#include <iostream>

template <typename T, typename I>
static int runTest(I *cl_algorithm_instance, int N, bool print, bool check)
{
	const vector<T> input = generateTestData<T>(N);

	const vector<cpx> output = cl_algorithm_instance->run(input);

	if (print)
		cerr << output << endl;

	if (check)
	{
#ifdef ALLOW_NPOT
		fprintf(stderr, "Calcolo serial naive DFT per riferimento...\n");
		const vector<cpx> ref = serial_naive_dft(input);
#else
		fprintf(stderr, "Calcolo serial non-recursive FFT per riferimento...\n");
		const vector<cpx> ref = serial_nonrecursive_fft(input);
#endif
		fprintf(stderr, "Distanza massima: %g\n", maxAbsDistance(output, ref));

	}

	return EXIT_SUCCESS;
}

static bool isdigit_string(const char *str)
{
	if (*str == '\0')
		return false; // Non accetta stringhe vuote

	while (*str && isdigit(*str)) str++;

	return *str == '\0'; // Accetta se siamo arrivati alla fine, ovvero tutti i caratteri erano numeri
}

int main(int argc, const char **argv)
{
	int input_size;
	bool use_complex_inputs;
	bool print = false;
	bool check = false;

	int cl_platform_index = 0;
	int cl_device_index = 0;

	// Parsing degli eventuali ultimi due argomenti numerici, da destra verso
	// sinistra
	if (argc >= 4 && isdigit_string(argv[argc - 1]))
		cl_platform_index = atoi(argv[--argc]);
	if (argc >= 4 && isdigit_string(argv[argc - 1]))
	{
		cl_device_index = cl_platform_index;
		cl_platform_index = atoi(argv[--argc]);
	}

	if (
		!(argc == 3
			|| (argc == 4 && (print = !strcmp(argv[3], "print")))
			|| (argc == 4 && (check = !strcmp(argv[3], "check")))
		)
		|| (use_complex_inputs = !!strcmp(argv[1], "real")) == !!strcmp(argv[1], "complex")
		|| (input_size = atoi(argv[2])) <= 0)
	{
#ifdef ALLOW_NPOT
		const char *pot_text = "";
#else
		const char *pot_text = " (potenza di due)";
#endif

		cerr << "Uso: " << argv[0] << " <real | complex> input-size [print | check] [cl-platform-num [cl-device-num]]" << endl << endl;
		cerr << " real             Usa input di tipo reale" << endl;
		cerr << " complex          Usa input di tipo complesso" << endl;
		cerr << " input-size       Numero di samples da passare in input" << pot_text << endl;
		cerr << " print            Mostra output" << endl;
#ifdef ALLOW_NPOT
		cerr << " check            Confronta output con serial naive DFT" << endl;
#else
		cerr << " check            Confronta output con serial non-recursive FFT" << endl;
#endif
		cerr << " cl-platform-num  Indice della piattaforma OpenCL da utilizzare (default: 0)" << endl;
		cerr << " cl-device-num    Indice del dispositivo OpenCL da utilizzare (default: 0)" << endl;
		cerr << endl;

		cerr << "Piattaforme e dispositivi OpenCL disponibili:" << endl;
		vector<string> platforms = clhAvailablePlatformNames();
		if (platforms.size() == 0)
		{
			cerr << "(nessuna piattaforma)" << endl;
		}
		else
		{
			for (unsigned i = 0; i < platforms.size(); i++)
			{
				cerr << "#" << i << " " << platforms[i] << endl;

				vector<string> devices = clhAvailableDeviceNames(clhSelectPlatform(i));
				if (devices.size() == 0)
				{
					cerr << "   (nessun dispositivo)" << endl;
				}
				else
				{
					for (unsigned j = 0; j < platforms.size(); j++)
						cerr << "  -> " << i << " " << j << " - " << devices[j] << endl;
				}
			}
		}

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
	{
		cl_naive_dft<cpx> instance(cl_platform_index, cl_device_index, input_size);
		runTest<cpx>(&instance, input_size, print, check);
	}
	else
	{
		cl_naive_dft<float> instance(cl_platform_index, cl_device_index, input_size);
		runTest<float>(&instance, input_size, print, check);
	}
}
