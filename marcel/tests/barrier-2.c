/* Copyright (C) 2002 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@redhat.com>, 2002.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

/* Test of POSIX barriers.  */


#include "marcel.h"


#ifdef MARCEL_POSIX


#define NTHREADS 20
#define ROUNDS 20


static pmarcel_barrier_t barriers[NTHREADS];
static pmarcel_mutex_t lock = PMARCEL_MUTEX_INITIALIZER;
static int counters[NTHREADS];
static int serial[NTHREADS];


static void *worker(void *arg)
{
	void *result = NULL;
	int nr = (long int) arg;
	int i;

	for (i = 0; i < ROUNDS; ++i) {
		int j;
		int retval;

		if (nr == 0) {
			memset(counters, '\0', sizeof(counters));
			memset(serial, '\0', sizeof(serial));
		}

		retval = pmarcel_barrier_wait(&barriers[NTHREADS - 1]);
		if (retval != 0 && retval != PMARCEL_BARRIER_SERIAL_THREAD) {
			marcel_fprintf(stderr,
				       "thread %d failed to wait for all the others\n", nr);
			result = (void *) 1;
		}

		for (j = nr; j < NTHREADS; ++j) {
			/* Increment the counter for this round.  */
			pmarcel_mutex_lock(&lock);
			++counters[j];
			pmarcel_mutex_unlock(&lock);

			/* Wait for the rest.  */
			retval = pmarcel_barrier_wait(&barriers[j]);

			/* Test the result.  */
			if (nr == 0 && counters[j] != j + 1) {
				marcel_fprintf(stderr,
					       "barrier in round %d released but count is %d\n",
					       j, counters[j]);
				result = (void *) 1;
			}

			if (retval != 0) {
				if (retval != PMARCEL_BARRIER_SERIAL_THREAD) {
					marcel_fprintf(stderr,
						       "thread %d in round %d has nonzero return value != PMARCEL_BARRIER_SERIAL_THREAD\n",
						       nr, j);
					result = (void *) 1;
				} else {
					pmarcel_mutex_lock(&lock);
					++serial[j];
					pmarcel_mutex_unlock(&lock);
				}
			}

			/* Wait for the rest again.  */
			retval = pmarcel_barrier_wait(&barriers[j]);

			/* Now we can check whether exactly one thread was serializing.  */
			if (nr == 0 && serial[j] != 1) {
				marcel_fprintf(stderr,
					       "not exactly one serial thread in round %d\n", j);
				result = (void *) 1;
			}
		}
	}

	return result;
}

int marcel_main(int argc TBX_UNUSED, char *argv[]TBX_UNUSED)
{
	pmarcel_t threads[NTHREADS];
	pmarcel_barrierattr_t b_attr;
	int i;
	void *res;
	int result = 0;

	marcel_init(&argc, argv);

	/* Initialized the barrier variables.  */
	for (i = 0; i < NTHREADS; ++i) {
		pmarcel_barrierattr_init(&b_attr);
		if (pmarcel_barrier_init(&barriers[i], &b_attr, i + 1) != 0) {
			marcel_fprintf(stderr, "Failed to initialize barrier %d\n", i);
			exit(1);
		}
		pmarcel_barrierattr_destroy(&b_attr);
	}

	/* Start the threads.  */
	for (i = 0; i < NTHREADS; ++i)
		if (pmarcel_create(&threads[i], NULL, worker, (void *) (long int) i) != 0) {
			marcel_fprintf(stderr, "Failed to start thread %d\n", i);
			exit(1);
		}

	/* And wait for them.  */
	for (i = 0; i < NTHREADS; ++i)
		if (pmarcel_join(threads[i], &res) != 0 || res != NULL) {
			marcel_fprintf(stderr, "thread %d returned a failure\n", i);
			result = 1;
		}

	marcel_end();

	return result;
}


#else


int main(int argc TBX_UNUSED, char *argv[]TBX_UNUSED)
{
	return MARCEL_TEST_SKIPPED;
}


#endif
