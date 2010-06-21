#define DEBUG

#include "thread.h"
#include "utils.h"

#include <stdio.h>

tick_t t1, t2;
int n;

int father_failed;
volatile int a;

void * f(void * arg)
{
	register int nb = (int)arg;
	int failed = 0;

	GET_TICK(t1);
	while(--nb) {
		marcel_yield();
#ifdef DEBUG
		if (a==0)
			failed++;
		a = 0;
		//fprintf(stderr, "Son...\n");
#endif
	}
	GET_TICK(t2);
	
	printf("schedule+switch time =  %fus (failed %d/%d)\n", TIMING_DELAY(t1, t2)/(int)n, failed, father_failed);
	return NULL;
}

void * f2(void * arg)
{
	register int nb = (int)arg;

	GET_TICK(t1);
	while(--nb) {
		marcel_yield();
#ifdef DEBUG
		//fprintf(stderr, "Son...\n");
#endif
	}
	GET_TICK(t2);
	
	printf("schedule time =  %fus\n", TIMING_DELAY(t1, t2)/(int)n);
	return NULL;
}

#ifdef MARCEL
void * f3(void * arg)
{
	register int nb = (int)arg;
	int failed = 0;

	GET_TICK(t1);
	while(--nb) {
		marcel_yield_to(__main_thread);
#ifdef DEBUG
		if (a==0)
			failed++;
		a = 0;
		//fprintf(stderr, "Son...\n");
#endif
	}
	GET_TICK(t2);
	
	printf("yield_to time =  %fus (failed %d/%d)\n", TIMING_DELAY(t1, t2)/(int)n, failed, father_failed);
	return NULL;
}
#endif

int marcel_main(int argc, char *argv[])
{
	marcel_t pid;
	int nb;
	int essais = 3;
	
	timing_init();
	
	marcel_init(argc, argv);
	
	if(argc != 2) {
		fprintf(stderr, "Usage: %s <nb>\n", argv[0]);
		exit(1);
	}

	n = atoi(argv[1]);
	
	if (n > 0) while(essais--) {
		nb = n>>1;
		
		father_failed = 0;
		marcel_create(&pid, NULL, f, (void *)nb);
		
		while(--nb) {
			marcel_yield();
#ifdef DEBUG
			if (a==1)
				father_failed++;
			a = 1;
			//fprintf(stderr, "Father...\n");
#endif
		}
		
		marcel_join(pid, NULL);
		
		nb = n;
		
		marcel_create(&pid, NULL, f2, (void *)nb);
		marcel_join(pid, NULL);

#ifdef MARCEL
		nb = n>>1;
		
		father_failed = 0;
		marcel_create(&pid, NULL, f3, (void *)nb);
		
		while(--nb) {
			marcel_yield_to(pid);
#ifdef DEBUG
			if (a==1)
				father_failed++;
			a = 1;
			//fprintf(stderr, "Father...\n");
#endif
		}
		
		marcel_join(pid, NULL);
#endif	
	}
	
	marcel_end();
	
	return 0;
}

