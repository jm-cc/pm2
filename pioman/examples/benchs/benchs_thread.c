#define MARCEL_INTERNAL_INCLUDE
#include "marcel.h"
#include "linux_spinlock.h"

#include "pm2_common.h"

#define SEM_LOOPS  100000
#define COND_LOOPS 100000


void 
foo_tasklet(){

}

/* TODO : faire les tests avec plusieurs threads */

static marcel_sem_t sem;
static marcel_sem_t sem2;

static marcel_cond_t cond, cond2;
static marcel_mutex_t mutex, mutex2;

void 
client(){
	int i;
	for(i=0;i<SEM_LOOPS;i++){
		marcel_sem_P(&sem);
		marcel_sem_V(&sem2);		
	}
	
	marcel_sem_P(&sem);
	marcel_mutex_lock(&mutex);
	for(i=0;i<COND_LOOPS;i++){
		marcel_cond_signal(&cond);
		marcel_cond_wait(&cond,&mutex);
	}
	marcel_mutex_unlock(&mutex);
}

int 
main(int argc, char** argv) 
{
	common_init(&argc, argv, NULL);
	int i;
	tbx_tick_t t1, t2;
	marcel_t pid_client;
	marcel_attr_t attr;
	
	marcel_create(&pid_client, NULL, (void*)client, NULL);


/* cout d'un sem_V() sem_P() */
	marcel_sem_init(&sem, 0);
	marcel_sem_init(&sem2, 0);
	printf("concurent sem_V/P (%d loops):", SEM_LOOPS);

	TBX_GET_TICK(t1);
	for(i=0;i<SEM_LOOPS;i++){
		marcel_sem_V(&sem);
		marcel_sem_P(&sem2);
	}
	TBX_GET_TICK(t2);
	printf("\t\t\t%f\n", 
	       SEM_LOOPS, 
	       tbx_tick2usec(TBX_TICK_RAW_DIFF(t1, t2))/(2*SEM_LOOPS));
	

/* Cout d'un cond_signal() cond_wait() */
	marcel_mutex_init(&mutex, NULL);
	marcel_cond_init(&cond, NULL);

	marcel_mutex_init(&mutex2, NULL);
	marcel_cond_init(&cond2, NULL);
	printf("concurent cond_wait/signal (%d loops):", COND_LOOPS);

	marcel_sem_V(&sem);
	marcel_mutex_lock(&mutex);
	TBX_GET_TICK(t1);
	for(i=0;i<COND_LOOPS;i++){
		marcel_cond_wait(&cond,&mutex);
		marcel_cond_signal(&cond);
	}
	TBX_GET_TICK(t2);
	marcel_mutex_unlock(&mutex);

	printf("\t\t%f\n", 
	       COND_LOOPS, 
	       tbx_tick2usec(TBX_TICK_RAW_DIFF(t1, t2))/(2*COND_LOOPS));
	

	common_exit(NULL);
	return 0;
}
