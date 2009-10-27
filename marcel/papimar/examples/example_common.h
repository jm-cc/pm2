#ifndef EXAMPLE_COMMON_H
#define EXAMPLE_COMMON_H
static
void
zero(double *m, int n) {
	int j;

	for (j = 0; j < n; j++) {
		int i;

		for (i = 0; i < n; i++) {
			m[i + n*j] = 0;
		}
	}
}

static
void
disp (double *m, int n) __attribute__((unused));

static
void
disp(double *m, int n) {
	int j;

	for (j = 0; j < n; j++) {
		int i;

		for (i = 0; i < n; i++) {
			printf(" %lf", m[i + j*n]);
		}
		printf("\n");
	}
}

static
void
fill(double *m, int n) {
	int j;

	for (j = 0; j < n; j++) {
		int i;

		for (i = 0; i < n; i++) {
			m[i + n*j] = rand() / (double)RAND_MAX;
		}
	}
}

static
void
basic_mmult(double *x, double *y, double *z, int n) {
	int j;

	for (j = 0; j < n; j++) {
		int i;

		for (i = 0; i < n; i++) {
			int k;

			for (k = 0; k < n; k++) {
				z[i + j*n] += x[k + j*n] * y[i + k*n]; 
			}
		}
	}
}
#endif

