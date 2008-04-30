#include "marcel.h"
#include <errno.h>
#include <time.h>
#include <sys/time.h>

#define NB_THREADS 4
#define SIZE 2*MA_CACHE_SIZE

marcel_bubble_t b0;
marcel_bubble_t b1;
marcel_bubble_t b2;
marcel_t t1;
marcel_t t2;
marcel_t t3;
marcel_t t4;

marcel_barrier_t barrier;
marcel_barrier_t allbarrier;
marcel_cond_t cond;
marcel_mutex_t mutex;

struct timeval start, finish;

any_t alloc(any_t foo) {
	void *data;
	void *bdata;

	int i;
   int id = (intptr_t)foo;
	
	fprintf(stderr,"launched for i %d\n", id);

	marcel_barrier_wait(&allbarrier);
	   
	data = marcel_malloc_customized(SIZE, HIGH_WEIGHT, 1, -1, 0);
	fprintf(stderr,"thread %p fait malloc -> data %p\n", MARCEL_SELF, data);

	if (id == 1)
	{
		bdata = marcel_malloc_customized(SIZE, HIGH_WEIGHT, 1, -1, 1);
		fprintf(stderr,"thread %p fait malloc pour bulle -> data %p\n", MARCEL_SELF, bdata);
	}

	marcel_see_allocated_memory(&MARCEL_SELF->sched.internal.entity);

	/* ready for main */
	marcel_barrier_wait(&barrier);
	if (id == 1)
		marcel_cond_signal(&cond);

	int load = *marcel_stats_get(MARCEL_SELF, load);
	
	int sum;
	for (i = 0 ; i < load*100000 ; ++i)
	{
		if (i % 100000 == 0)
			--*marcel_stats_get(MARCEL_SELF, load);
		sum += i;
	}

	/* liberation */
	marcel_free_customized(data);
	fprintf(stderr,"thread %p fait free -> data %p\n", MARCEL_SELF, data);

	if (id == 1)
	{
		marcel_free_customized(bdata);
		fprintf(stderr,"thread %p fait free -> data %p\n", MARCEL_SELF, bdata);
	}
	//marcel_see_allocated_memory(&MARCEL_SELF->sched.internal.entity);

	/* ready for main */
	marcel_barrier_wait(&barrier);
	if (id == 1)
		marcel_cond_signal(&cond);
	
	return NULL;
}

int main(int argc, char *argv[]) {
	int i;
	
   marcel_init(&argc,argv);
#ifdef PROFILE
   profile_activate(FUT_ENABLE, MARCEL_PROF_MASK, 0);
#endif

	marcel_barrier_init(&barrier, NULL, NB_THREADS);
	marcel_barrier_init(&allbarrier, NULL, NB_THREADS + 1);
	marcel_cond_init(&cond, NULL);
	marcel_mutex_init(&mutex, NULL);

   /* construction */
   marcel_bubble_init(&b0);
	marcel_bubble_init(&b1);
	marcel_bubble_init(&b2);

	marcel_bubble_insertbubble(&b0, &b1);
	marcel_bubble_insertbubble(&b0, &b2);

   /* lancement des threads pour allouer */
   {
		marcel_attr_t attr;
      marcel_attr_init(&attr);
      marcel_attr_setinitbubble(&attr, &b1);
      marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr,0);
      marcel_attr_setname(&attr,"thread");
      marcel_create(&t1, &attr, alloc, (any_t)1);
		*marcel_stats_get(t1, load) = 1500;
	}

	{
      marcel_attr_t attr;
      marcel_attr_init(&attr);
      marcel_attr_setinitbubble(&attr, &b1);
      marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr,0);
      marcel_attr_setname(&attr,"thread");
      marcel_create(&t2, &attr, alloc, (any_t)2);
      *marcel_stats_get(t2, load) = 3000;
   }

   {
		marcel_attr_t attr;
      marcel_attr_init(&attr);
      marcel_attr_setinitbubble(&attr, &b2);
      marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr,0);
      marcel_attr_setname(&attr,"thread");
		marcel_create(&t3, &attr, alloc, (any_t)3);
		*marcel_stats_get(t3, load) = 700;
	}

   {
		marcel_attr_t attr;
      marcel_attr_init(&attr);
      marcel_attr_setinitbubble(&attr, &b2);
      marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr,0);
      marcel_attr_setname(&attr,"thread");
		marcel_create(&t4, &attr, alloc, (any_t)4);
		*marcel_stats_get(t4, load) = 800;
	}

	/* lancer la bulle */
	marcel_wake_up_bubble(&b0);

	/* ordonnancement */
	marcel_start_playing();
   marcel_bubble_spread(&b0, marcel_topo_level(0,0), 0);
	
	/* threads places */
	marcel_barrier_wait(&allbarrier);

	/* commencer par les threads */
	marcel_cond_wait(&cond, &mutex);
	gettimeofday(&start, NULL);

	/* attente de liberation */
	marcel_cond_wait(&cond, &mutex);
	gettimeofday(&finish, NULL);

	long time = (1000000 * finish.tv_sec + finish.tv_usec) - (1000000 * start.tv_sec + start.tv_usec);
	fprintf(stderr,"TIME %ld\n", time);	

   /* destroy */
	marcel_barrier_destroy(&barrier);
	marcel_barrier_destroy(&allbarrier);
	marcel_cond_destroy(&cond);
	marcel_mutex_destroy(&mutex);

	profile_stop();
   return 0;
}
