#include "thread.h"
#include "utils.h"

#include <stdio.h>

tick_t t1, t2;

marcel_mutex_t m;

void * f(void * arg)
{
	register int n = (int)arg;

	marcel_yield();
	marcel_yield();
	marcel_yield();
	marcel_yield();
	marcel_yield();

	GET_TICK(t1);
	while(--n) {
		marcel_mutex_lock(&m);
		marcel_mutex_unlock(&m);
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
	
	marcel_init(&argc, argv);
	
	marcel_mutex_init(&m, NULL);

	while(essais--) {
		nb = atoi(argv[1]);
		
		if(nb == 0)
			break;
		
		nb >>= 1;
		nb++;
		
		marcel_create(&pid, NULL, f, (void *)nb);
		
		marcel_join(pid, NULL);
	}
	
	marcel_end();
	
	return 0;
}

