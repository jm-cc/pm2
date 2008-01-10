#include <omp.h>
#include <stdio.h>

int
main(int argc, char **argv) {
#pragma omp task
	{
		fprintf(stderr, "task body\n");
#pragma omp taskwait
		fprintf(stderr, "task body again\n");
	}

	return 0;
}
