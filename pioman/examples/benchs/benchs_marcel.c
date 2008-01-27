#define MARCEL_INTERNAL_INCLUDE
#include "marcel.h"
#include "linux_spinlock.h"

#include "pm2_common.h"

#define SEM_LOOPS  50000
#define SPIN_LOOPS 50000
#define SPIN_SOFT_LOOPS 50000
#define TASKLET_LOOPS 50000
#define TASKLET2_LOOPS 50000
#define RTASKLET_LOOPS 50000


void foo_tasklet(){
}

/* TODO : faire les tests avec plusieurs threads */

int 
main(int argc, char** argv) 
{
	common_init(&argc, argv, NULL);
	int i;
	tbx_tick_t t1, t2;


/* cout d'un sem_V() sem_P() */
	marcel_sem_t sem;
	marcel_sem_init(&sem, 0);
	printf("sem_V/P (%d loops):", SEM_LOOPS);

	TBX_GET_TICK(t1);
	for(i=0;i<SEM_LOOPS;i++){
		marcel_sem_V(&sem);
		marcel_sem_P(&sem);
	}
	TBX_GET_TICK(t2);
	printf("\t\t\t%f\n", 
	       SEM_LOOPS, 
	       tbx_tick2usec(TBX_TICK_RAW_DIFF(t1, t2))/SEM_LOOPS);
	

/* cout d'un ma_spin_lock_softirq/spin_unlock */
	ma_spinlock_t lock=MA_SPIN_LOCK_UNLOCKED;
	printf("spin_lock/unlock (%d loops):", SPIN_LOOPS);

	TBX_GET_TICK(t1);
	for(i=0;i<SPIN_LOOPS;i++){
		ma_spin_lock(&lock);
		ma_spin_unlock(&lock);
	}
	TBX_GET_TICK(t2);	
	printf("\t\t%f\n", 
	       SPIN_LOOPS, 
	       tbx_tick2usec(TBX_TICK_RAW_DIFF(t1, t2))/SPIN_LOOPS);


/* cout d'un ma_spin_lock_softirq/spin_unlock */
	printf("spin_lock/unlock_softirq (%d loops):", SPIN_SOFT_LOOPS);

	TBX_GET_TICK(t1);
	for(i=0;i<SPIN_SOFT_LOOPS;i++){
		ma_spin_lock_softirq(&lock);
		ma_spin_unlock_softirq(&lock);
	}
	TBX_GET_TICK(t2);	
	printf("\t%f\n", 
	       SPIN_SOFT_LOOPS, 
	       tbx_tick2usec(TBX_TICK_RAW_DIFF(t1, t2))/SPIN_SOFT_LOOPS);
	
	
/* tasklet disable/enable */
	struct ma_tasklet_struct tasklet;
	ma_tasklet_init(&tasklet, &foo_tasklet, 0);
	printf("Tasklet enabling (%d loops):", TASKLET_LOOPS);

	TBX_GET_TICK(t1);
	for(i=0;i<TASKLET_LOOPS;i++){
		ma_tasklet_disable(&tasklet);
		ma_tasklet_enable(&tasklet);
	}
	TBX_GET_TICK(t2);
	printf("\t\t%f\n", 
	       TASKLET_LOOPS,
	       tbx_tick2usec(TBX_TICK_RAW_DIFF(t1, t2))/TASKLET_LOOPS);


/* Lancement d'une tasklet */
	printf("Tasklet scheduling (%d loops):", TASKLET2_LOOPS);
	TBX_GET_TICK(t1);
	for(i=0;i<TASKLET2_LOOPS;i++){
		ma_tasklet_schedule(&tasklet);
	}
	TBX_GET_TICK(t2);
	printf("\t%f\n", 
	       TASKLET2_LOOPS,
	       tbx_tick2usec(TBX_TICK_RAW_DIFF(t1, t2))/TASKLET2_LOOPS);
	

#ifdef MARCEL_REMOTE_TASKLETS
/* Lancement d'une tasklet à distance */	
	marcel_vpset_t set;
	marcel_vpset_all_but_vp(&set, 1);
	set_vpset(&tasklet, &set);		
	printf("tasklet remote scheduling (%d loops):", RTASKLET_LOOPS);
	TBX_GET_TICK(t1);
	for(i=0;i<RTASKLET_LOOPS;i++){
		ma_tasklet_schedule(&tasklet);
	}
	TBX_GET_TICK(t2);
	printf("%f\n", 
	       RTASKLET_LOOPS,
	       tbx_tick2usec(TBX_TICK_RAW_DIFF(t1, t2))/RTASKLET_LOOPS);
#endif	
	
	common_exit(NULL);
	return 0;
}
