#include "marcel.h"
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>

#include <numaif.h>

#define NB_THREADS 32
#define DOWN 4

#define MAXMEMSHIFT (4+20)
//#define MAXMEMSHIFT (4+6)
#define STARTMEMSHIFT (10)
#define MEMSHIFT 1
#define MEMSIZE (1 << MAXMEMSHIFT)
#define STARTSIZE (1 << STARTMEMSHIFT)

#define NBSHIFTS (MAXMEMSHIFT - STARTMEMSHIFT + 1)

#define NLOOPS 4

#define TIME_DIFF(t1, t2)					\
	((t2.tv_sec - t1.tv_sec) * 1000000 + (t2.tv_usec - t1.tv_usec))

marcel_bubble_t b0;
marcel_bubble_t b1;
marcel_bubble_t b2;
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
marcel_t t17;
marcel_t t18;
marcel_t t19;
marcel_t t20;
marcel_t t21;
marcel_t t22;
marcel_t t23;
marcel_t t24;
marcel_t t25;
marcel_t t26;
marcel_t t27;
marcel_t t28;
marcel_t t29;
marcel_t t30;
marcel_t t31;
marcel_t t32;

marcel_barrier_t barrier;
marcel_barrier_t allbarrier;
marcel_cond_t cond;
marcel_mutex_t mutex;

int finished = 0;

any_t run(any_t foo) {
	volatile char *mem_array_high[32];
	volatile char *mem_array_medium[32];
	volatile char *mem_array_low[32];
	int i, node;
	int id = (intptr_t)foo;

	/* allocation memoire */

	for (node = 0 ; node <= numa_max_node() ; ++node)
		{
			mem_array_low[node] = (volatile char *) marcel_malloc_customized(MEMSIZE, LOW_WEIGHT, 0, node, 0);
			mem_array_medium[node] = (volatile char *) marcel_malloc_customized(MEMSIZE, MEDIUM_WEIGHT, 0, node, 0);
			mem_array_high[node] = (volatile char *) marcel_malloc_customized(MEMSIZE, HIGH_WEIGHT, 0, node, 0);
		}

	int onnode = ma_node_entity(&MARCEL_SELF->as_entity);
	fprintf(stderr,"thread %p %d sur node %d\n", MARCEL_SELF, id, onnode);
	marcel_see_allocated_memory(&MARCEL_SELF->as_entity);

	struct timeval start, finish;
	long time, mem;
	int mems, k, m;
	unsigned int gold;
	char sum;
	for (node = 0 ; node <= numa_max_node() ; ++node)
		{
			memset((void*) mem_array_high[node], 0, MEMSIZE);
			memset((void*) mem_array_medium[node], 0, MEMSIZE);
			memset((void*) mem_array_low[node], 0, MEMSIZE);
		}
	/* ici commence le test de perf */
	marcel_barrier_wait(&allbarrier);
	//marcel_start_remix();

	gettimeofday(&start, NULL);
	i = 0;
	for (mems = STARTMEMSHIFT, mem = 1<<mems ; mems <= MAXMEMSHIFT ; mems += MEMSHIFT, mem<<=MEMSHIFT)
		{
			gold = ((float) mem *((sqrt(5)-1.)/2.));
			for (m = 0 ; m < NLOOPS ; m++)
				{
					//gettimeofday(&start, NULL);
					for ( k = 0 ; k < mem ; k++)
						{
							sum += mem_array_high[onnode][i];
							i += gold;
							while (i >= mem)
								i -= mem;
						}
					//gettimeofday(&finish, NULL);
					//time = (TIME_DIFF(start, finish) * 1000UL) >> (mems);
				}
			//fprintf(stderr,"time for id %d : %ld\n", id, time);
			fprintf(stderr,"%d ",id);
			*marcel_stats_get(MARCEL_SELF, load) -= 1000;
		}
	gettimeofday(&finish, NULL);
	time = TIME_DIFF(start, finish);
	fprintf(stderr,"\ntime for id %d : %ld\n", id, time);

	/* liberation memoire */
	for (node = 0 ; node <= numa_max_node() ; ++node)
		{
			marcel_free_customized((void* ) mem_array_high[node]);
			marcel_free_customized((void* ) mem_array_medium[node]);
			marcel_free_customized((void* ) mem_array_low[node]);
		}


	marcel_see_allocated_memory(&MARCEL_SELF->as_entity);

	/* fin */
	fprintf(stderr,"*\n");
	finished ++;

	if (finished == NB_THREADS)
		{
			//marcel_stop_remix();
			marcel_cond_signal(&cond);
		}

	return NULL;
}

int main(int argc, char *argv[])
{
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

	marcel_bubble_insertbubble(&b0,&b1);
	marcel_bubble_insertbubble(&b0,&b2);

	/* lancement des threads pour allouer */
	{
		marcel_attr_t attr;
		marcel_attr_init(&attr);
		marcel_attr_setinitbubble(&attr, &b1);
		marcel_attr_setid(&attr,0);
		marcel_attr_setprio(&attr,0);
		marcel_attr_setname(&attr,"thread");
		marcel_create(&t1, &attr, run, (any_t)1);
		*marcel_stats_get(t1, load) = NBSHIFTS*1000;
	}

	{
		marcel_attr_t attr;
		marcel_attr_init(&attr);
		marcel_attr_setinitbubble(&attr, &b1);
		marcel_attr_setid(&attr,0);
		marcel_attr_setprio(&attr,0);
		marcel_attr_setname(&attr,"thread");
		marcel_create(&t2, &attr, run, (any_t)2);
		*marcel_stats_get(t2, load) = NBSHIFTS*1000;
	}

	{
		marcel_attr_t attr;
		marcel_attr_init(&attr);
		marcel_attr_setinitbubble(&attr, &b1);
		marcel_attr_setid(&attr,0);
		marcel_attr_setprio(&attr,0);
		marcel_attr_setname(&attr,"thread");
		marcel_create(&t3, &attr, run, (any_t)3);
		*marcel_stats_get(t3, load) = NBSHIFTS*1000;
	}

	{
		marcel_attr_t attr;
		marcel_attr_init(&attr);
		marcel_attr_setinitbubble(&attr, &b1);
		marcel_attr_setid(&attr,0);
		marcel_attr_setprio(&attr,0);
		marcel_attr_setname(&attr,"thread");
		marcel_create(&t4, &attr, run, (any_t)4);
		*marcel_stats_get(t4, load) = NBSHIFTS*1000;
	}

	{
		marcel_attr_t attr;
		marcel_attr_init(&attr);
		marcel_attr_setinitbubble(&attr, &b1);
		marcel_attr_setid(&attr,0);
		marcel_attr_setprio(&attr,0);
		marcel_attr_setname(&attr,"thread");
		marcel_create(&t5, &attr, run, (any_t)5);
		*marcel_stats_get(t5, load) = NBSHIFTS*1000;
	}

	{
		marcel_attr_t attr;
		marcel_attr_init(&attr);
		marcel_attr_setinitbubble(&attr, &b1);
		marcel_attr_setid(&attr,0);
		marcel_attr_setprio(&attr,0);
		marcel_attr_setname(&attr,"thread");
		marcel_create(&t6, &attr, run, (any_t)6);
		*marcel_stats_get(t6, load) = NBSHIFTS*1000;
	}

	{
		marcel_attr_t attr;
		marcel_attr_init(&attr);
		marcel_attr_setinitbubble(&attr, &b1);
		marcel_attr_setid(&attr,0);
		marcel_attr_setprio(&attr,0);
		marcel_attr_setname(&attr,"thread");
		marcel_create(&t7, &attr, run, (any_t)7);
		*marcel_stats_get(t7, load) = NBSHIFTS*1000;
	}

	{
		marcel_attr_t attr;
		marcel_attr_init(&attr);
		marcel_attr_setinitbubble(&attr, &b1);
		marcel_attr_setid(&attr,0);
		marcel_attr_setprio(&attr,0);
		marcel_attr_setname(&attr,"thread");
		marcel_create(&t8, &attr, run, (any_t)8);
		*marcel_stats_get(t8, load) = NBSHIFTS*1000;
	}

	{
		marcel_attr_t attr;
		marcel_attr_init(&attr);
		marcel_attr_setinitbubble(&attr, &b1);
		marcel_attr_setid(&attr,0);
		marcel_attr_setprio(&attr,0);
		marcel_attr_setname(&attr,"thread");
		marcel_create(&t17, &attr, run, (any_t)17);
		*marcel_stats_get(t17, load) = NBSHIFTS*1000;
	}

	{
		marcel_attr_t attr;
		marcel_attr_init(&attr);
		marcel_attr_setinitbubble(&attr, &b1);
		marcel_attr_setid(&attr,0);
		marcel_attr_setprio(&attr,0);
		marcel_attr_setname(&attr,"thread");
		marcel_create(&t18, &attr, run, (any_t)18);
		*marcel_stats_get(t18, load) = NBSHIFTS*1000;
	}

	{
		marcel_attr_t attr;
		marcel_attr_init(&attr);
		marcel_attr_setinitbubble(&attr, &b1);
		marcel_attr_setid(&attr,0);
		marcel_attr_setprio(&attr,0);
		marcel_attr_setname(&attr,"thread");
		marcel_create(&t19, &attr, run, (any_t)19);
		*marcel_stats_get(t19, load) = NBSHIFTS*1000;
	}

	{
		marcel_attr_t attr;
		marcel_attr_init(&attr);
		marcel_attr_setinitbubble(&attr, &b1);
		marcel_attr_setid(&attr,0);
		marcel_attr_setprio(&attr,0);
		marcel_attr_setname(&attr,"thread");
		marcel_create(&t20, &attr, run, (any_t)20);
		*marcel_stats_get(t20, load) = NBSHIFTS*1000;
	}

	{
		marcel_attr_t attr;
		marcel_attr_init(&attr);
		marcel_attr_setinitbubble(&attr, &b1);
		marcel_attr_setid(&attr,0);
		marcel_attr_setprio(&attr,0);
		marcel_attr_setname(&attr,"thread");
		marcel_create(&t5, &attr, run, (any_t)5);
		*marcel_stats_get(t5, load) = NBSHIFTS*1000;
	}

	{
		marcel_attr_t attr;
		marcel_attr_init(&attr);
		marcel_attr_setinitbubble(&attr, &b1);
		marcel_attr_setid(&attr,0);
		marcel_attr_setprio(&attr,0);
		marcel_attr_setname(&attr,"thread");
		marcel_create(&t21, &attr, run, (any_t)21);
		*marcel_stats_get(t21, load) = NBSHIFTS*1000;
	}

	{
		marcel_attr_t attr;
		marcel_attr_init(&attr);
		marcel_attr_setinitbubble(&attr, &b1);
		marcel_attr_setid(&attr,0);
		marcel_attr_setprio(&attr,0);
		marcel_attr_setname(&attr,"thread");
		marcel_create(&t22, &attr, run, (any_t)22);
		*marcel_stats_get(t22, load) = NBSHIFTS*1000;
	}

	{
		marcel_attr_t attr;
		marcel_attr_init(&attr);
		marcel_attr_setinitbubble(&attr, &b1);
		marcel_attr_setid(&attr,0);
		marcel_attr_setprio(&attr,0);
		marcel_attr_setname(&attr,"thread");
		marcel_create(&t23, &attr, run, (any_t)23);
		*marcel_stats_get(t23, load) = NBSHIFTS*1000;
	}

	{
		marcel_attr_t attr;
		marcel_attr_init(&attr);
		marcel_attr_setinitbubble(&attr, &b1);
		marcel_attr_setid(&attr,0);
		marcel_attr_setprio(&attr,0);
		marcel_attr_setname(&attr,"thread");
		marcel_create(&t24, &attr, run, (any_t)24);
		*marcel_stats_get(t24, load) = NBSHIFTS*1000;
	}

	{
		marcel_attr_t attr;
		marcel_attr_init(&attr);
		marcel_attr_setinitbubble(&attr, &b2);
		marcel_attr_setid(&attr,0);
		marcel_attr_setprio(&attr,0);
		marcel_attr_setname(&attr,"thread");
		marcel_create(&t9, &attr, run, (any_t)9);
		*marcel_stats_get(t9, load) = NBSHIFTS*1000;
	}

	{
		marcel_attr_t attr;
		marcel_attr_init(&attr);
		marcel_attr_setinitbubble(&attr, &b2);
		marcel_attr_setid(&attr,0);
		marcel_attr_setprio(&attr,0);
		marcel_attr_setname(&attr,"thread");
		marcel_create(&t10, &attr, run, (any_t)10);
		*marcel_stats_get(t10, load) = NBSHIFTS*1000;
	}

	{
		marcel_attr_t attr;
		marcel_attr_init(&attr);
		marcel_attr_setinitbubble(&attr, &b2);
		marcel_attr_setid(&attr,0);
		marcel_attr_setprio(&attr,0);
		marcel_attr_setname(&attr,"thread");
		marcel_create(&t11, &attr, run, (any_t)11);
		*marcel_stats_get(t11, load) = NBSHIFTS*1000;
	}

	{
		marcel_attr_t attr;
		marcel_attr_init(&attr);
		marcel_attr_setinitbubble(&attr, &b2);
		marcel_attr_setid(&attr,0);
		marcel_attr_setprio(&attr,0);
		marcel_attr_setname(&attr,"thread");
		marcel_create(&t12, &attr, run, (any_t)12);
		*marcel_stats_get(t12, load) = NBSHIFTS*1000;
	}

	{
		marcel_attr_t attr;
		marcel_attr_init(&attr);
		marcel_attr_setinitbubble(&attr, &b2);
		marcel_attr_setid(&attr,0);
		marcel_attr_setprio(&attr,0);
		marcel_attr_setname(&attr,"thread");
		marcel_create(&t13, &attr, run, (any_t)13);
		*marcel_stats_get(t13, load) = NBSHIFTS*1000;
	}

	{
		marcel_attr_t attr;
		marcel_attr_init(&attr);
		marcel_attr_setinitbubble(&attr, &b2);
		marcel_attr_setid(&attr,0);
		marcel_attr_setprio(&attr,0);
		marcel_attr_setname(&attr,"thread");
		marcel_create(&t14, &attr, run, (any_t)14);
		*marcel_stats_get(t14, load) = NBSHIFTS*1000;
	}

	{
		marcel_attr_t attr;
		marcel_attr_init(&attr);
		marcel_attr_setinitbubble(&attr, &b2);
		marcel_attr_setid(&attr,0);
		marcel_attr_setprio(&attr,0);
		marcel_attr_setname(&attr,"thread");
		marcel_create(&t15, &attr, run, (any_t)15);
		*marcel_stats_get(t15, load) = NBSHIFTS*1000;
	}

	{
		marcel_attr_t attr;
		marcel_attr_init(&attr);
		marcel_attr_setinitbubble(&attr, &b2);
		marcel_attr_setid(&attr,0);
		marcel_attr_setprio(&attr,0);
		marcel_attr_setname(&attr,"thread");
		marcel_create(&t16, &attr, run, (any_t)16);
		*marcel_stats_get(t16, load) = NBSHIFTS*1000;
	}

	{
		marcel_attr_t attr;
		marcel_attr_init(&attr);
		marcel_attr_setinitbubble(&attr, &b2);
		marcel_attr_setid(&attr,0);
		marcel_attr_setprio(&attr,0);
		marcel_attr_setname(&attr,"thread");
		marcel_create(&t25, &attr, run, (any_t)25);
		*marcel_stats_get(t25, load) = NBSHIFTS*1000;
	}

	{
		marcel_attr_t attr;
		marcel_attr_init(&attr);
		marcel_attr_setinitbubble(&attr, &b2);
		marcel_attr_setid(&attr,0);
		marcel_attr_setprio(&attr,0);
		marcel_attr_setname(&attr,"thread");
		marcel_create(&t26, &attr, run, (any_t)26);
		*marcel_stats_get(t26, load) = NBSHIFTS*1000;
	}

	{
		marcel_attr_t attr;
		marcel_attr_init(&attr);
		marcel_attr_setinitbubble(&attr, &b2);
		marcel_attr_setid(&attr,0);
		marcel_attr_setprio(&attr,0);
		marcel_attr_setname(&attr,"thread");
		marcel_create(&t27, &attr, run, (any_t)27);
		*marcel_stats_get(t27, load) = NBSHIFTS*1000;
	}

	{
		marcel_attr_t attr;
		marcel_attr_init(&attr);
		marcel_attr_setinitbubble(&attr, &b2);
		marcel_attr_setid(&attr,0);
		marcel_attr_setprio(&attr,0);
		marcel_attr_setname(&attr,"thread");
		marcel_create(&t28, &attr, run, (any_t)28);
		*marcel_stats_get(t28, load) = NBSHIFTS*1000;
	}

	{
		marcel_attr_t attr;
		marcel_attr_init(&attr);
		marcel_attr_setinitbubble(&attr, &b2);
		marcel_attr_setid(&attr,0);
		marcel_attr_setprio(&attr,0);
		marcel_attr_setname(&attr,"thread");
		marcel_create(&t29, &attr, run, (any_t)29);
		*marcel_stats_get(t29, load) = NBSHIFTS*1000;
	}

	{
		marcel_attr_t attr;
		marcel_attr_init(&attr);
		marcel_attr_setinitbubble(&attr, &b2);
		marcel_attr_setid(&attr,0);
		marcel_attr_setprio(&attr,0);
		marcel_attr_setname(&attr,"thread");
		marcel_create(&t30, &attr, run, (any_t)30);
		*marcel_stats_get(t30, load) = NBSHIFTS*1000;
	}

	{
		marcel_attr_t attr;
		marcel_attr_init(&attr);
		marcel_attr_setinitbubble(&attr, &b2);
		marcel_attr_setid(&attr,0);
		marcel_attr_setprio(&attr,0);
		marcel_attr_setname(&attr,"thread");
		marcel_create(&t31, &attr, run, (any_t)31);
		*marcel_stats_get(t31, load) = NBSHIFTS*1000;
	}

	{
		marcel_attr_t attr;
		marcel_attr_init(&attr);
		marcel_attr_setinitbubble(&attr, &b2);
		marcel_attr_setid(&attr,0);
		marcel_attr_setprio(&attr,0);
		marcel_attr_setname(&attr,"thread");
		marcel_create(&t32, &attr, run, (any_t)32);
		*marcel_stats_get(t32, load) = NBSHIFTS*1000;
	}


	/* lancer la bulle */
	marcel_wake_up_bubble(&b0);

	/* ordonnancement */
	marcel_start_playing();
        //marcel_bubble_badspread(&b0, marcel_topo_level(DOWN,0), 0);

	struct timeval start, finish;
	gettimeofday(&start, NULL);

	/* threads places */
	marcel_barrier_wait(&allbarrier);

	/* execution des threads */

	/* attente de liberation */
	marcel_cond_wait(&cond, &mutex);
	gettimeofday(&finish, NULL);

	long time = TIME_DIFF(start, finish);
	fprintf(stderr,"TOTAL TIME : %ld\n", time);

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
