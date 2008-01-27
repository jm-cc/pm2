
#include "thread.h"
#include "utils.h"

#include <stdio.h>

int nb;

void * f(void * arg)
{
	tick_t *t3 = (tick_t *)arg;
	GET_TICK(*t3);
	return NULL;
}

void * run(void * arg) {
	marcel_t pid;
	int my_nb;
	int essais = 3;
	tick_t t1, t2, t3, t4, t5;
	int num = (int) arg;
	marcel_vpset_t set = 1<<num;

	if(nb == 0)
		return NULL;

	marcel_apply_vpset(&set);

	while(essais--) {
		
		my_nb = nb;

		GET_TICK(t4);
		while(my_nb--) {
			
			GET_TICK(t1);
			marcel_create(&pid, NULL, f, &t3);
			marcel_join(pid, NULL);
			GET_TICK(t2);
				
		}
		GET_TICK(t5);
		marcel_printf("last creation  time =  %fus\n", TIMING_DELAY(t1, t3));
		marcel_printf("last execution time =  %fus\n", TIMING_DELAY(t1, t2));
		marcel_printf("mean execution time =  %fus\n", TIMING_DELAY(t4, t5)/(float)nb);
		marcel_fflush(stdout);
	}
	return NULL;
}

int marcel_main(int argc, char *argv[])
{
	int i;
	int ncpus;
	
	timing_init();

	marcel_init(&argc, argv);

	if(argc < 2) {
		fprintf(stderr, "Usage: %s <nb>\n", argv[0]);
		exit(1);
	}

	nb = atoi(argv[1]);

	if (argc==3)
		ncpus = atoi(argv[2]);
	else
		ncpus = marcel_nbvps();

	{
		marcel_t pids[ncpus];

		for (i=0; i<ncpus; i++)
			marcel_create(&pids[i], NULL, run, (void *)(i));

		for (i=0; i<ncpus; i++)
			marcel_join(pids[i], NULL);
	}
	
	marcel_end();
	
	return 0;
}

