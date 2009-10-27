#include <marcel.h>
#include <papi.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <papimar.h>

#include "example_common.h"

/*
 * example_3.c
 * - run several threads
 * - threads are _not_ bound
 * - each thread computes a basic matrix multiplication 8 times
 *
 * - arg 1 is the size of the matrix (10 by default)
 * - arg 2 is the number of threads to create (marcel_nbvps() by default)
 */

double *a = NULL;
double *b = NULL;
double *c = NULL;
int n;

#define NB_EVENTS 4

struct s_arg {
	struct s_papimar_context *ppm_context;
};

void
f_pre_switch(void *_arg) {
	struct s_arg *arg = _arg;
	papimar_suspend_context(arg->ppm_context);
}

void
f_post_switch(void *_arg) {
	struct s_arg *arg = _arg;
	papimar_resume_context(arg->ppm_context);
}

void *
g(void *_arg) {
	struct s_arg *arg = _arg;
	long long counters[NB_EVENTS];
	int i;

	for (i = 0; i < 8; i++) {
		basic_mmult(a, b, c, n);

		papimar_read_context(arg->ppm_context, counters);
		
		printf("\n%p: PAPI_TOT_INS: %lld\n", marcel_self(), counters[0]);
		printf(  "%p: PAPI_TOT_CYC: %lld\n", marcel_self(), counters[1]);
		printf(  "%p: PAPI_L1_DCM: %lld\n",  marcel_self(), counters[2]);
		printf(  "%p: PAPI_L2_DCM: %lld\n",  marcel_self(), counters[3]);
		
		papimar_reset_context(arg->ppm_context);
	}

	return NULL;
}

int
main(int argc, char **argv) {
	struct s_papimar_event_set *event_set = NULL;
	int nt;
	int r;

	r = PAPI_library_init(PAPI_VER_CURRENT);
	if (r != PAPI_VER_CURRENT) {
		papimar_error("PAPI_library_init", r);
	}

	extern int marcel_gettid(void);
	if ((r = PAPI_thread_init(marcel_gettid)) != PAPI_OK) {
		papimar_error("PAPI_thread_init", r);
	}

	marcel_init(&argc, argv);

	if (argc > 1) {
		n = atoi(argv[1]);
	} else {
		n = 10;
	}

	if (argc > 2) {
		nt = atoi(argv[2]);
	} else {
		nt = marcel_nbvps();
	}
	a = tmalloc(n*n * sizeof(double));
	b = tmalloc(n*n * sizeof(double));
	c = tmalloc(n*n * sizeof(double));

	fill(a, n);
	fill(b, n);
	zero(c, n);

	papimar_init_event_set(&event_set);
	papimar_add_event(event_set, PAPI_TOT_INS);
	papimar_add_event(event_set, PAPI_TOT_CYC);
	papimar_add_event(event_set, PAPI_L1_DCM);
	papimar_add_event(event_set, PAPI_L2_DCM);
	
	papimar_start(event_set);

	{
		marcel_attr_t	attr;
		marcel_t	t[nt];
		struct s_arg	arg[nt];
		int i;

		marcel_attr_init(&attr);
		marcel_attr_setfpreswitch(&attr, f_pre_switch);
		marcel_attr_setfpostswitch(&attr, f_post_switch);

		for (i = 0; i < nt; i++) {

			papimar_init_context(event_set, &arg[i].ppm_context);
			marcel_create(&t[i], &attr, g, &arg[i]);
		}

		for (i = 0; i < nt; i++) {
			marcel_join(t[i], NULL);
		}
	}

	papimar_stop(event_set);

	return 0;
}
