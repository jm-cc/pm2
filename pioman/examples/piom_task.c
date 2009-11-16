#define MARCEL_INTERNAL_INCLUDE

#define __PROF_APP__
#include <errno.h>
#include <sys/time.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

#include <semaphore.h>
#include "pm2_common.h"

#define DEFAULT_LOOPS 1000000
#define DEFAULT_WARMUP 10000
#define DEFAULT_MAX_ID 1024
#define DEFAULT_VP1 0
#define task_max 8

static int loops=DEFAULT_LOOPS;
static int max_id=DEFAULT_MAX_ID;
static int warmup=DEFAULT_WARMUP;
static int vp1=DEFAULT_VP1;

static volatile  int plop=0;

static struct piom_ltask *task1;

static tbx_tick_t t1, t2;

static void usage(void) {
  fprintf(stderr, "-L loops - number of loops [%d]\n", DEFAULT_LOOPS);
  fprintf(stderr, "-W warmup - number of warmup iterations [%d]\n", DEFAULT_WARMUP);
  fprintf(stderr, "-M max_id - number of different args [%d]\n", DEFAULT_MAX_ID);
  fprintf(stderr, "-VP1 vp1 - vp on which the producer is bound [%d]\n", DEFAULT_VP1);
}

static int *score;

static ma_atomic_t nb_tasks=MA_ATOMIC_INIT(0);
static marcel_sem_t *done;

static int my_index[task_max];

int f1(void* args)
{
	int my_pos=*(int*)args;
	
	if(ma_atomic_inc_return(&nb_tasks) > loops) {
		piom_ltask_completed(&task1[my_pos]);
		marcel_sem_V(&done[my_pos]);
	}
	score[marcel_current_vp()]++;

	return 1;
}

void change_vp_set(marcel_vpset_t *vpset, int vp){
	switch(vp){
	case -1:
		*vpset = MARCEL_VPSET_FULL;
		break;
	default:
		*vpset = MARCEL_VPSET_VP(vp);
	}
}

int VP1=0;
int VP2=0;

void* th1_func(void *arg){
	int i;
	marcel_vpset_t vpset = MARCEL_VPSET_VP(vp1);
	marcel_apply_vpset(&vpset);

	marcel_vpset_t task_vpset;
	fprintf(stderr, "Benchamrk : binding tasks on %d, %d\n", VP1, VP2);
	TBX_GET_TICK(t1);
	for(i=0;i<task_max;i++) {
		task_vpset = MARCEL_VPSET_VP(VP1);
		//task_vpset = MARCEL_VPSET_FULL;
		marcel_vpset_set(&task_vpset, VP2);	
		piom_ltask_create(&task1[i], 
				  &f1, 
				  &my_index[i],
				  PIOM_LTASK_OPTION_REPEAT, 
				  task_vpset);
		piom_ltask_submit(&task1[i]);
	}
	while(piom_ltask_schedule());

	return NULL;
}

int main(int argc, char ** argv)
{
	common_init(&argc, argv, NULL);
	
	if (argc > 1 && !strcmp(argv[1], "--help")) {
		usage();
		goto out;
	}

	int i;
	for(i=1 ; i<argc ; i+=2) {
		if (!strcmp(argv[i], "-L")) {
			loops = atoi(argv[i+1]);
		}
		else if (!strcmp(argv[i], "-W")) {
			warmup = atoi(argv[i+1]);
		}
		else if (!strcmp(argv[i], "-M")) {
			max_id = atoi(argv[i+1]);
		}
		else if (!strcmp(argv[i], "-VP1")) {
			vp1 = atoi(argv[i+1]);
		}
		else {
			fprintf(stderr, "Illegal argument %s\n", argv[i]);
			usage();
			goto out;
		}
	}

	char* ENV_VP1=getenv("PIOM_VP1");
	if(ENV_VP1)
		VP1=atoi(ENV_VP1);

	char* ENV_VP2=getenv("PIOM_VP2");
	if(ENV_VP2)
		VP2=atoi(ENV_VP2);

	piom_init_ltasks();
	task1=malloc(sizeof(struct piom_ltask)*max_id);
	score=malloc(sizeof(int)*marcel_nbvps());

	marcel_t producer;
	int j;
	done=malloc(sizeof(marcel_sem_t)*task_max);
	for(j=0;j<task_max;j++){
		marcel_sem_init(&done[j], 0);
		my_index[j]=j;
	}

	ma_atomic_init(&nb_tasks, 0);
	for(j=0;j<marcel_nbvps();j++)
		score[j] = 0;
	
	marcel_create(&producer, NULL, th1_func, NULL);
	fprintf(stderr, "Benchmarking\n");	
	int score_total;


	for(j=0;j<task_max;j++)
		marcel_sem_P(&done[j]);
	TBX_GET_TICK(t2);
	
	score_total = 0;
	for(i=0;i<marcel_nbvps();i++){
		score_total += score[i];
		
	}

	double lat=TBX_TIMING_DELAY(t1, t2);
	fprintf(stderr, "%d tasks scheduled in %lf us (%lf/task)\n", 
		ma_atomic_read(&nb_tasks), lat, lat/ma_atomic_read(&nb_tasks));

	for(j=0;j<marcel_nbvps();j++)
		fprintf(stderr, "score[%d]=%d\n", j, score[j]);
	
	marcel_join(producer, NULL);
	piom_exit_ltasks();

out:
	common_exit(NULL);
	return 0;
}
