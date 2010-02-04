#define MARCEL_INTERNAL_INCLUDE

#include <errno.h>
#include <sys/time.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

#include "pm2_common.h"

#ifdef MARCEL

/* This program tests the behavior of piom_cond*
 * it performs a pingpong between 2 threads
 */
#define DEFAULT_LOOPS 10000

static unsigned loops = DEFAULT_LOOPS;

static void usage(void) {
  fprintf(stderr, "-L loops - number of loops [%d]\n", DEFAULT_LOOPS);
}

piom_cond_t cond[DEFAULT_LOOPS];

marcel_barrier_t barrier;

/* compute for usec microsecondes  */
static void work(unsigned usec)
{
	tbx_tick_t t1, t2;
	TBX_GET_TICK(t1);
	do{
		TBX_GET_TICK(t2);
	} while(TBX_TIMING_DELAY(t1, t2) < usec);
}

static void* th_func(void* arg)
{
	marcel_barrier_wait(&barrier);
	int i;
	for(i=0;i< loops; i++) {
		piom_cond_wait(&cond[i], 1);
		/* as soon as the thread is unblocked, the condition may be re-used,
		 *  for instance, we could call piom_cond_init.
		 */
		memset(&cond[i], 0x42, sizeof(cond[i]));
	}
	printf("cond_wait ok\n");
	marcel_barrier_wait(&barrier);
	for(i=0;i< loops; i++) {
		while(! piom_cond_test(&cond[i], 1));
		memset(&cond[i], 0x42, sizeof(cond[i]));
	}
	printf("cond_test ok\n");
}

int main(int argc, char ** argv)
{
	common_init(&argc, argv, NULL);
	int i;
	
	if (argc > 1 && !strcmp(argv[1], "--help")) {
		usage();
		goto out;
	}
	for(i=1 ; i<argc ; i+=2) {
		if (!strcmp(argv[i], "-L")) {
			loops = atoi(argv[i+1]);
		}
		else {
			fprintf(stderr, "Illegal argument %s\n", argv[i]);
			usage();
			goto out;
		}
	}

	for(i=0; i< loops; i++)
		piom_cond_init(&cond[i], 0);

	marcel_barrier_init(&barrier, NULL, 2);
	
	marcel_t tid;
	marcel_create(&tid, NULL, th_func, NULL);

	/* Wait the consumer thread */
	marcel_barrier_wait(&barrier);	
	for(i=0;i< loops; i++) {
		piom_cond_signal(&cond[i], 1);
	}

	marcel_barrier_wait(&barrier);
	for(i=0; i< loops; i++)
		piom_cond_init(&cond[i], 0);

	for(i=0;i< loops; i++) {
		piom_cond_signal(&cond[i], 1);
	}
	
	marcel_join(tid, NULL);
out:
	return 0;
}

#else
#warning test_cond is only available with Marcel
int main(){ return 0;}
#endif
