#include <omp.h>
#include <stdio.h>

int
main(int argc, char **argv) {
#pragma omp task
	{
		fprintf(stderr, "task body\n");
	}

	return 0;
}
