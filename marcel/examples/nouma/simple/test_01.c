#include "marcel.h"
#include <numaif.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>

#define DOWN 2
#define NB_THREADS 16
#define SIZE 2*MA_CACHE_SIZE

marcel_bubble_t b0;

marcel_t t1;
marcel_t t2;
marcel_t t3;
marcel_t t4;
marcel_t t5;
marcel_t t6;
marcel_t t7;
marcel_t t8;
marcel_t t9;
marcel_t t10;
marcel_t t11;
marcel_t t12;
marcel_t t13;
marcel_t t14;
marcel_t t15;
marcel_t t16;

marcel_barrier_t barrier;
marcel_barrier_t allbarrier;
marcel_cond_t cond;
marcel_mutex_t mutex;

struct timeval start, finish;

int finished = 0;

any_t alloc(any_t foo) {
	void *data;
	int i;
   int id = (intptr_t)foo;

	marcel_fprintf(stderr,"launched for i %d\n", id);

	marcel_barrier_wait(&allbarrier);
	data = marcel_malloc_customized(SIZE, HIGH_WEIGHT, 1, -1, 0);

	marcel_fprintf(stderr,"thread %p fait malloc -> data %p\n", MARCEL_SELF, data);
	marcel_see_allocated_memory(&MARCEL_SELF->sched.internal.entity);

	/* ready for main */

	marcel_barrier_wait(&barrier);
	//marcel_start_remix();
	if (id == 1)
	{
		marcel_cond_signal(&cond);
	}

	int load = *marcel_stats_get(MARCEL_SELF, load);
	
	int sum;
	for (i = 0 ; i < load*10000 ; ++i)
	{
		 if (i % 10000 == 0)
		 {
			 marcel_fprintf(stderr,".");
			 --*marcel_stats_get(MARCEL_SELF, load);
		 }	
		 sum += i;
	}

	/* liberation */
	marcel_free_customized(data);
	marcel_fprintf(stderr,"thread %p fait free -> data %p\n", MARCEL_SELF, data);

	marcel_fprintf(stderr,"*\n");
	finished ++;
	
	if (finished == NB_THREADS)
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

   /* lancement des threads pour allouer */
   {
		marcel_attr_t attr;
      marcel_attr_init(&attr);
      marcel_attr_setinitbubble(&attr, &b0);
      marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr,0);
      marcel_attr_setname(&attr,"thread");
      marcel_create(&t1, &attr, alloc, (any_t)1);
		*marcel_stats_get(t1, load) = 1000;
	}

	{
      marcel_attr_t attr;
      marcel_attr_init(&attr);
      marcel_attr_setinitbubble(&attr, &b0);
      marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr,0);
      marcel_attr_setname(&attr,"thread");
      marcel_create(&t2, &attr, alloc, (any_t)2);
      *marcel_stats_get(t2, load) = 1000;
   }

   {
		marcel_attr_t attr;
      marcel_attr_init(&attr);
      marcel_attr_setinitbubble(&attr, &b0);
      marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr,0);
      marcel_attr_setname(&attr,"thread");
		marcel_create(&t3, &attr, alloc, (any_t)3);
		*marcel_stats_get(t3, load) = 1000;
	}

   {
		marcel_attr_t attr;
      marcel_attr_init(&attr);
      marcel_attr_setinitbubble(&attr, &b0);
      marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr,0);
      marcel_attr_setname(&attr,"thread");
		marcel_create(&t4, &attr, alloc, (any_t)4);
		*marcel_stats_get(t4, load) = 1000;
	}

	{
		marcel_attr_t attr;
      marcel_attr_init(&attr);
      marcel_attr_setinitbubble(&attr, &b0);
      marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr,0);
      marcel_attr_setname(&attr,"thread");
      marcel_create(&t5, &attr, alloc, (any_t)5);
		*marcel_stats_get(t5, load) = 1000;
	}

	{
      marcel_attr_t attr;
      marcel_attr_init(&attr);
      marcel_attr_setinitbubble(&attr, &b0);
      marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr,0);
      marcel_attr_setname(&attr,"thread");
      marcel_create(&t6, &attr, alloc, (any_t)6);
      *marcel_stats_get(t6, load) = 1000;
   }

   {
		marcel_attr_t attr;
      marcel_attr_init(&attr);
      marcel_attr_setinitbubble(&attr, &b0);
      marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr,0);
      marcel_attr_setname(&attr,"thread");
		marcel_create(&t7, &attr, alloc, (any_t)7);
		*marcel_stats_get(t7, load) = 1000;
	}

   {
		marcel_attr_t attr;
      marcel_attr_init(&attr);
      marcel_attr_setinitbubble(&attr, &b0);
      marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr,0);
      marcel_attr_setname(&attr,"thread");
		marcel_create(&t8, &attr, alloc, (any_t)8);
		*marcel_stats_get(t8, load) = 1000;
	}

	{
		marcel_attr_t attr;
      marcel_attr_init(&attr);
      marcel_attr_setinitbubble(&attr, &b0);
      marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr,0);
      marcel_attr_setname(&attr,"thread");
		marcel_create(&t9, &attr, alloc, (any_t)9);
		*marcel_stats_get(t9, load) = 1000;
	}

	{
		marcel_attr_t attr;
      marcel_attr_init(&attr);
      marcel_attr_setinitbubble(&attr, &b0);
      marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr,0);
      marcel_attr_setname(&attr,"thread");
      marcel_create(&t10, &attr, alloc, (any_t)10);
		*marcel_stats_get(t10, load) = 1000;
	}

	{
      marcel_attr_t attr;
      marcel_attr_init(&attr);
      marcel_attr_setinitbubble(&attr, &b0);
      marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr,0);
      marcel_attr_setname(&attr,"thread");
      marcel_create(&t11, &attr, alloc, (any_t)11);
      *marcel_stats_get(t11, load) = 1000;
   }

   {
		marcel_attr_t attr;
      marcel_attr_init(&attr);
      marcel_attr_setinitbubble(&attr, &b0);
      marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr,0);
      marcel_attr_setname(&attr,"thread");
		marcel_create(&t12, &attr, alloc, (any_t)12);
		*marcel_stats_get(t12, load) = 1000;
	}

   {
		marcel_attr_t attr;
      marcel_attr_init(&attr);
      marcel_attr_setinitbubble(&attr, &b0);
      marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr,0);
      marcel_attr_setname(&attr,"thread");
		marcel_create(&t13, &attr, alloc, (any_t)13);
		*marcel_stats_get(t13, load) = 1000;
	}

	{
		marcel_attr_t attr;
      marcel_attr_init(&attr);
      marcel_attr_setinitbubble(&attr, &b0);
      marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr,0);
      marcel_attr_setname(&attr,"thread");
      marcel_create(&t14, &attr, alloc, (any_t)14);
		*marcel_stats_get(t14, load) = 1000;
	}

	{
      marcel_attr_t attr;
      marcel_attr_init(&attr);
      marcel_attr_setinitbubble(&attr, &b0);
      marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr,0);
      marcel_attr_setname(&attr,"thread");
      marcel_create(&t15, &attr, alloc, (any_t)15);
      *marcel_stats_get(t15, load) = 1000;
   }

   {
		marcel_attr_t attr;
      marcel_attr_init(&attr);
      marcel_attr_setinitbubble(&attr, &b0);
      marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr,0);
      marcel_attr_setname(&attr,"thread");
		marcel_create(&t16, &attr, alloc, (any_t)16);
		*marcel_stats_get(t16, load) = 1000;
	}

/*    { */
/* 		marcel_attr_t attr; */
/*       marcel_attr_init(&attr); */
/*       marcel_attr_setinitbubble(&attr, &b9); */
/*       marcel_attr_setid(&attr,0); */
/*       marcel_attr_setprio(&attr,0); */
/*       marcel_attr_setname(&attr,"thread"); */
/* 		marcel_create(&t17, &attr, alloc, (any_t)17); */
/* 		*marcel_stats_get(t17, load) = 1000; */
/* 	} */

/* 	{ */
/* 		marcel_attr_t attr; */
/*       marcel_attr_init(&attr); */
/*       marcel_attr_setinitbubble(&attr, &b9); */
/*       marcel_attr_setid(&attr,0); */
/*       marcel_attr_setprio(&attr,0); */
/*       marcel_attr_setname(&attr,"thread"); */
/* 		marcel_create(&t18, &attr, alloc, (any_t)18); */
/* 		*marcel_stats_get(t18, load) = 1000; */
/* 	} */
 
	/* lancer la bulle */
	marcel_wake_up_bubble(&b0);

	/* ordonnancement */
	marcel_start_playing();
	//marcel_bubble_badspread(&b0, marcel_topo_level(2,0), 0);

	/* threads places */
	marcel_barrier_wait(&allbarrier);

	/* commencer par les threads */
	marcel_cond_wait(&cond, &mutex);
	gettimeofday(&start, NULL);

	/* attente de liberation */
	marcel_cond_wait(&cond, &mutex);
	gettimeofday(&finish, NULL);

	long time = (1000000 * finish.tv_sec + finish.tv_usec) - (1000000 * start.tv_sec + start.tv_usec);
	marcel_fprintf(stderr,"TIME %ld\n", time);	
	
   /* destroy */
	marcel_barrier_destroy(&barrier);
	marcel_barrier_destroy(&allbarrier);
	marcel_cond_destroy(&cond);
	marcel_mutex_destroy(&mutex);

	profile_stop();
   return 0;
}
