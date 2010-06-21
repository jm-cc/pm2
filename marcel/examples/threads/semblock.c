#include "thread.h"
#include "utils.h"

#include <stdio.h>

tick_t t1, t2;

marcel_sem_t s1, s2;

void * f(void * arg)
{
	register int n = (int)arg;

	GET_TICK(t1);
	while(--n) {
		marcel_sem_V(&s1);
		marcel_sem_P(&s2);
#ifdef DEBUG
		fprintf(stderr, "Son...\n");
#endif
	}
	GET_TICK(t2);
	
	printf("time =  %fus\n", TIMING_DELAY(t1, t2));
	return NULL;
}

int marcel_main(int argc, char *argv[])
{
	marcel_t pid;
	int nb;
	int essais = 3;
	
	if(argc != 2) {
		fprintf(stderr, "Usage: %s <nb>\n", argv[0]);
		exit(1);
	}
	
	timing_init();
	
	marcel_init(argc, argv);
	
	marcel_sem_init(&s1,0);
	marcel_sem_init(&s2,0);

	while(essais--) {
		nb = atoi(argv[1]);
		
		if(nb == 0)
			break;
		
		nb >>= 1;
		nb++;
		
		marcel_create(&pid, NULL, f, (void *)nb);
		
		while(--nb) {
#ifdef DEBUG
			fprintf(stderr, "Father...\n");
#endif
			marcel_sem_V(&s2);
			marcel_sem_P(&s1);
		}
		
		marcel_join(pid, NULL);
	}
	
	marcel_end();
	
	return 0;
}

