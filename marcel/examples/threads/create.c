
#include "thread.h"
#include "utils.h"

#include <stdio.h>

tick_t t1, t2, t3;

void * f(void * arg)
{
	GET_TICK(t3);
	return NULL;
}

int marcel_main(int argc, char *argv[])
{
	marcel_t pid;
	int nb;
	int essais = 3;
	
	timing_init();

	marcel_init(&argc, argv);
	
	if(argc != 2) {
		fprintf(stderr, "Usage: %s <nb>\n", argv[0]);
		exit(1);
	}
	
	while(essais--) {
		nb = atoi(argv[1]);
		
		if(nb == 0)
			break;
		
		while(nb--) {
			
#ifdef PROFILE
			if (!nb)
				profile_activate(FUT_ENABLE, MARCEL_PROF_MASK, 0);
#endif
			GET_TICK(t1);
			marcel_create(&pid, NULL, f, NULL);
			marcel_join(pid, NULL);
			GET_TICK(t2);
				
		}
		printf("creation  time =  %fus\n", TIMING_DELAY(t1, t3));
		printf("execution time =  %fus\n", TIMING_DELAY(t1, t2));
		fflush(stdout);
	}

	marcel_end();
	
	return 0;
}

