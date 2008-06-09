#include "marcel.h"
#include <errno.h>
#include <time.h>
#include <sys/time.h>

#define NB_THREADS 9
#define SIZE 2*MA_CACHE_SIZE

marcel_bubble_t b0;
marcel_bubble_t b1;
marcel_bubble_t b2;
marcel_bubble_t b3;
marcel_bubble_t b4;
marcel_t t1;
marcel_t t2;
marcel_t t3;
marcel_t t4;
marcel_t t5;
marcel_t t6;
marcel_t t7;
marcel_t t8;
marcel_t t9;

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

	sleep(1);

	fprintf(stderr,"launched for i %d\n", id);

	marcel_barrier_wait(&allbarrier);

	if (id == 2 || id == 3 || id == 4 || id == 5 || id == 6)
		data = marcel_malloc_customized(SIZE, LOW_WEIGHT, 1, -1, 0);
	if (id == 1 || id == 7 || id == 8 || id == 9)
		data = marcel_malloc_customized(SIZE, HIGH_WEIGHT, 1, -1, 0);

	fprintf(stderr,"thread %p fait malloc -> data %p\n", MARCEL_SELF, data);

	if (id == 1 || id == 7)
	{
		bdata = marcel_malloc_customized(SIZE, MEDIUM_WEIGHT, 1, -1, 1);
		fprintf(stderr,"thread %p fait malloc pour bulle -> data %p\n", MARCEL_SELF, bdata);
	}

	if (id == 2)
	{
		bdata = marcel_malloc_customized(SIZE, LOW_WEIGHT, 1, -1, 1);
		fprintf(stderr,"thread %p fait malloc pour bulle -> data %p\n", MARCEL_SELF, bdata);
	}

	if (id == 3 || id == 5)
	{
		bdata = marcel_malloc_customized(SIZE, HIGH_WEIGHT, 1, -1, 1);
		fprintf(stderr,"thread %p fait malloc pour bulle -> data %p\n", MARCEL_SELF, bdata);
	}

	marcel_see_allocated_memory(&MARCEL_SELF->as_entity);

	/* ready for main */
	marcel_barrier_wait(&barrier);
	if (id == 1){
	  //marcel_start_remix();
		marcel_cond_signal(&cond);
	}

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

	if (id == 1 || id == 7 || id == 2 || id == 3 || id == 5)
	{
		marcel_free_customized(bdata);
		fprintf(stderr,"thread %p fait free -> data %p\n", MARCEL_SELF, bdata);
	}
	//marcel_see_allocated_memory(&MARCEL_SELF->sched.internal.entity);

	/* ready for main */
	fprintf(stderr,"*\n");
	marcel_barrier_wait(&barrier);
	if (id == 1)
	{
	  //marcel_stop_remix();
		marcel_cond_signal(&cond);
	}
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
	marcel_bubble_init(&b3);
	marcel_bubble_init(&b4);

	fprintf(stderr,"bulle mere %p\n",&marcel_root_bubble);
	marcel_bubble_insertbubble(&b0, &b1);
	marcel_bubble_insertbubble(&b0, &b2);
	fprintf(stderr,"%p et %p dans %p\n",&b1,&b2,&b0);
	marcel_bubble_insertbubble(&b1, &b3);
	marcel_bubble_insertbubble(&b1, &b4);
	fprintf(stderr,"%p et %p dans %p\n",&b3,&b4,&b1);

   /* lancement des threads pour allouer */
   {
		marcel_attr_t attr;
      marcel_attr_init(&attr);
      marcel_attr_setinitbubble(&attr, &b0);
      marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr,0);
      marcel_attr_setname(&attr,"thread");
      marcel_create(&t1, &attr, alloc, (any_t)1);
		*marcel_stats_get(t1, load) = 20000;
	}

	{
      marcel_attr_t attr;
      marcel_attr_init(&attr);
      marcel_attr_setinitbubble(&attr, &b1);
      marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr,0);
      marcel_attr_setname(&attr,"thread");
      marcel_create(&t2, &attr, alloc, (any_t)2);
      *marcel_stats_get(t2, load) = 20000;
   }

   {
		marcel_attr_t attr;
      marcel_attr_init(&attr);
      marcel_attr_setinitbubble(&attr, &b2);
      marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr,0);
      marcel_attr_setname(&attr,"thread");
		marcel_create(&t3, &attr, alloc, (any_t)3);
		*marcel_stats_get(t3, load) = 10000;
	}

   {
		marcel_attr_t attr;
      marcel_attr_init(&attr);
      marcel_attr_setinitbubble(&attr, &b2);
      marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr,0);
      marcel_attr_setname(&attr,"thread");
		marcel_create(&t4, &attr, alloc, (any_t)4);
		*marcel_stats_get(t4, load) = 5000;
	}

	{
		marcel_attr_t attr;
      marcel_attr_init(&attr);
      marcel_attr_setinitbubble(&attr, &b3);
      marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr,0);
      marcel_attr_setname(&attr,"thread");
      marcel_create(&t5, &attr, alloc, (any_t)5);
		*marcel_stats_get(t5, load) = 10000;
	}

	{
      marcel_attr_t attr;
      marcel_attr_init(&attr);
      marcel_attr_setinitbubble(&attr, &b3);
      marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr,0);
      marcel_attr_setname(&attr,"thread");
      marcel_create(&t6, &attr, alloc, (any_t)6);
      *marcel_stats_get(t6, load) = 5000;
   }

   {
		marcel_attr_t attr;
      marcel_attr_init(&attr);
      marcel_attr_setinitbubble(&attr, &b4);
      marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr,0);
      marcel_attr_setname(&attr,"thread");
		marcel_create(&t7, &attr, alloc, (any_t)7);
		*marcel_stats_get(t7, load) = 10000;
	}

   {
		marcel_attr_t attr;
      marcel_attr_init(&attr);
      marcel_attr_setinitbubble(&attr, &b4);
      marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr,0);
      marcel_attr_setname(&attr,"thread");
		marcel_create(&t8, &attr, alloc, (any_t)8);
		*marcel_stats_get(t8, load) = 10000;
	}

	{
		marcel_attr_t attr;
      marcel_attr_init(&attr);
      marcel_attr_setinitbubble(&attr, &b4);
      marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr,0);
      marcel_attr_setname(&attr,"thread");
		marcel_create(&t9, &attr, alloc, (any_t)9);
		*marcel_stats_get(t9, load) = 10000;
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

#ifdef PROFILE
	profile_stop();
#endif
   return 0;
}
