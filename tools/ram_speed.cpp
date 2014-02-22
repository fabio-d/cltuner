#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

#include <unistd.h>

#define MAX_MEGABYTES 2048
#define MAX_ITERAZIONI 128

int main(int argc, char **argv)
{
	int megabytes;
	int num_iterazioni;

	if (argc != 3 || (megabytes = atoi(argv[1])) <= 0 || (num_iterazioni = atoi(argv[2])) <= 0)
	{
		fprintf(stderr, "Uso: %s <MiB> <num-iterazioni>\n", argv[0]);
		fprintf(stderr, "Specificare la quantita' di RAM da utilizzare per i test\n");
		fprintf(stderr, "e il numero di iterazioni da eseguire.\n");
		return EXIT_FAILURE;
	}

	if (megabytes > MAX_MEGABYTES)
	{
		fprintf(stderr, "Troppi MiB richiesti!\n");
		fprintf(stderr, "Per evitare di rendere instabile il sistema, questo programma\n");
		fprintf(stderr, "impedisce di allocare piu' di %d MiB\n", MAX_MEGABYTES);
		return EXIT_FAILURE;
	}

	if (num_iterazioni > MAX_ITERAZIONI)
	{
		fprintf(stderr, "Troppe iterazioni richieste (max %d)!\n", MAX_ITERAZIONI);
		return EXIT_FAILURE;
	}

	const int bytes_half = megabytes * 1024 * 1024 / 2;
	char *buffer1 = (char*)malloc(bytes_half);
	if (buffer1 == NULL)
	{
		perror("malloc");
		return EXIT_FAILURE;
	}

	char *buffer2 = (char*)malloc(bytes_half);
	if (buffer2 == NULL)
	{
		perror("malloc");
		return EXIT_FAILURE;
	}

	// Tocca ogni pagina almeno una volta per essere sicuri che sia stata
	// effettvamente allocata dal kernel
	const unsigned int pagesize = getpagesize();
	for (unsigned int i = 0; i < bytes_half; i += pagesize)
		buffer1[i] = 121, buffer2[i] = 212;

	clock_t start = clock();
	for (int i = 0; i < num_iterazioni; i++)
		memcpy(buffer2, buffer1, bytes_half);
	clock_t end = clock();

	const float secs = (end - start) / float(CLOCKS_PER_SEC);

	printf("Bandwidth: %g MiB/s\n", num_iterazioni * megabytes / secs);

	free(buffer1);
	free(buffer2);
	return EXIT_SUCCESS;
}
