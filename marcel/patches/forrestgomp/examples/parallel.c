#include <omp.h>
#include <stdio.h>

int
main(int argc, char **argv) {
#pragma omp parallel
	{
		fprintf(stderr, "parallel body\n");
	}

	return 0;
}
