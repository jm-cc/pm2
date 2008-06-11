#include "marcel.h"
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>

#include <numaif.h>

#define NB_THREADS 64

#define MAXMEMSHIFT (4+20)
//#define MAXMEMSHIFT (4+6)
#define STARTMEMSHIFT (10)
#define MEMSHIFT 1
#define MEMSIZE (1 << MAXMEMSHIFT)
#define STARTSIZE (1 << STARTMEMSHIFT)

#define NBSHIFTS (MAXMEMSHIFT - STARTMEMSHIFT + 1)

#define NLOOPS 4

#define TIME_DIFF(t1, t2) \
((t2.tv_sec - t1.tv_sec) * 1000000 + (t2.tv_usec - t1.tv_usec))

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
marcel_t t33;
marcel_t t34;
marcel_t t35;
marcel_t t36;
marcel_t t37;
marcel_t t38;
marcel_t t39;
marcel_t t40;
marcel_t t41;
marcel_t t42;
marcel_t t43;
marcel_t t44;
marcel_t t45;
marcel_t t46;
marcel_t t47;
marcel_t t48;
marcel_t t49;
marcel_t t50;
marcel_t t51;
marcel_t t52;
marcel_t t53;
marcel_t t54;
marcel_t t55;
marcel_t t56;
marcel_t t57;
marcel_t t58;
marcel_t t59;
marcel_t t60;
marcel_t t61;
marcel_t t62;
marcel_t t63;
marcel_t t64;

marcel_barrier_t barrier;
marcel_barrier_t allbarrier;
marcel_cond_t cond;
marcel_mutex_t mutex;

int finished = 0;
char *shared[64];

any_t run(any_t foo) {
	volatile char *highown;
	
	int i, node;
	int id = (intptr_t)foo;

	/* allocation memoire */	
	highown = (volatile char *) marcel_malloc(MEMSIZE, __FILE__,__LINE__);

	switch (id)
	{
		case 1:
			shared[1] = ( char *) marcel_malloc(MEMSIZE, __FILE__,__LINE__);
			shared[11] = ( char *) marcel_malloc(MEMSIZE, __FILE__,__LINE__);
			memset((void*) shared[1], 0, MEMSIZE);
			memset((void*) shared[11], 0, MEMSIZE);
			break;
		case 11:
			shared[12] = ( char *) marcel_malloc(MEMSIZE, __FILE__,__LINE__);
			memset((void*) shared[12], 0, MEMSIZE);
			break;
		case 17:
			shared[13] = ( char *) marcel_malloc(MEMSIZE, __FILE__,__LINE__);
			memset((void*) shared[13], 0, MEMSIZE);
			break;
		case 21:
			shared[2] = ( char *) marcel_malloc(MEMSIZE, __FILE__,__LINE__);
			memset((void*) shared[2], 0, MEMSIZE);
			shared[21] = ( char *) marcel_malloc(MEMSIZE, __FILE__,__LINE__);
			memset((void*) shared[21], 0, MEMSIZE);
			break;
		case 29:
			shared[22] = ( char *) marcel_malloc(MEMSIZE, __FILE__,__LINE__);
			memset((void*) shared[22], 0, MEMSIZE);
			break;
		case 37:
			shared[3] = ( char *) marcel_malloc(MEMSIZE, __FILE__,__LINE__);
			shared[31] = ( char *) marcel_malloc(MEMSIZE, __FILE__,__LINE__);
			memset((void*) shared[3], 0, MEMSIZE);
			memset((void*) shared[31], 0, MEMSIZE);
			break;
		case 45:
			shared[32] = ( char *) marcel_malloc(MEMSIZE, __FILE__,__LINE__);
			memset((void*) shared[32], 0, MEMSIZE);
			break;
		case 49:
			shared[4] = ( char *) marcel_malloc(MEMSIZE, __FILE__,__LINE__);
			shared[41] = ( char *) marcel_malloc(MEMSIZE, __FILE__,__LINE__);
			memset((void*) shared[4], 0, MEMSIZE);
			memset((void*) shared[41], 0, MEMSIZE);
			break;
		case 53:
			shared[42] = ( char *) marcel_malloc(MEMSIZE, __FILE__,__LINE__);
			memset((void*) shared[42], 0, MEMSIZE);
			break;
		case 57:
			shared[5] = ( char *) marcel_malloc(MEMSIZE, __FILE__,__LINE__);
			shared[51] = ( char *) marcel_malloc(MEMSIZE, __FILE__,__LINE__);
			memset((void*) shared[5], 0, MEMSIZE);
			memset((void*) shared[51], 0, MEMSIZE);
			break;
	}

	marcel_barrier_wait(&barrier);
	//marcel_see_allocated_memory(&MARCEL_SELF->as_entity);

	struct timeval start, finish;
	long time, mem;
	int mems, k, m;
	unsigned int gold;
	char sum;
	char sum2;
	char sum3;

	memset((void*) highown, 0, MEMSIZE);

	/* ici commence le test de perf */
	marcel_barrier_wait(&allbarrier);
	
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
				sum += highown[i];

				if (id <= 10)
				{
					sum += shared[1][i];
					sum += shared[11][i];
				} else if (id <= 16)
				{
					sum += shared[1][i];
					sum += shared[12][i];
				} else if (id <= 20)
				{
					sum += shared[1][i];
					sum += shared[13][i];
				} else if (id <= 28)
				{
					sum += shared[2][i];
					sum += shared[21][i];
				} else if (id <= 36)
				{
					sum += shared[2][i];
					sum += shared[22][i];
				} else if (id <= 44)
				{
					sum += shared[3][i];
					sum += shared[31][i];
				} else if (id <= 48)
				{
					sum += shared[3][i];
					sum += shared[32][i];
				} else if (id <= 52)
				{
					sum += shared[4][i];
					sum += shared[41][i];
				} else if (id <= 56)
				{
					sum += shared[4][i];
					sum += shared[42][i];
				} else if (id <= 62)
				{
					sum += shared[5][i];
					sum += shared[51][i];
				} else {
					sum += shared[5][i];
				}

				i += gold;
				while (i >= mem)
					i -= mem;
			}
			//gettimeofday(&finish, NULL);
			//time = (TIME_DIFF(start, finish) * 1000UL) >> (mems);
		}
		//marcel_fprintf(stderr,"time for id %d : %ld\n", id, time);
		marcel_fprintf(stderr,"%d ",id);	
		*marcel_stats_get(MARCEL_SELF, load) -= 1000;
	}
	gettimeofday(&finish, NULL);
	time = TIME_DIFF(start, finish);
	marcel_fprintf(stderr,"\ntime for id %d : %ld\n", id, time);

   /* liberation memoire */

	marcel_free((void* ) highown);
	//marcel_see_allocated_memory(&MARCEL_SELF->as_entity);
	
	/* fin */
	finished ++;
	marcel_fprintf(stderr,"* %d *\n",finished);

	if (finished == NB_THREADS)
	{
		marcel_free(shared[1]);
		marcel_free(shared[11]);
		marcel_free(shared[12]);
		marcel_free(shared[13]);
		marcel_free(shared[2]);
		marcel_free(shared[21]);
		marcel_free(shared[22]);
		marcel_free(shared[3]);
		marcel_free(shared[31]);
		marcel_free(shared[32]);
		marcel_free(shared[4]);
		marcel_free(shared[41]);
		marcel_free(shared[42]);
		marcel_free(shared[5]);
		marcel_free(shared[51]);
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

   /* lancement des threads pour allouer */
   {
		marcel_attr_t attr;
      marcel_attr_init(&attr);
		marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr,0);
      marcel_attr_setname(&attr,"thread");
      marcel_create(&t1, &attr, run, (any_t)1);
		*marcel_stats_get(t1, load) = NBSHIFTS*1000;
	}

   {
      marcel_attr_t attr;
      marcel_attr_init(&attr);
      marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr,0);
      marcel_attr_setname(&attr,"thread");
      marcel_create(&t2, &attr, run, (any_t)2);
      *marcel_stats_get(t2, load) = NBSHIFTS*1000;
   }

	{
		marcel_attr_t attr;
      marcel_attr_init(&attr);
      marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr,0);
      marcel_attr_setname(&attr,"thread");
      marcel_create(&t3, &attr, run, (any_t)3);
		*marcel_stats_get(t3, load) = NBSHIFTS*1000;
	}

	{
		marcel_attr_t attr;
      marcel_attr_init(&attr);
      marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr,0);
      marcel_attr_setname(&attr,"thread");
      marcel_create(&t4, &attr, run, (any_t)4);
		*marcel_stats_get(t4, load) = NBSHIFTS*1000;
	}

	{
		marcel_attr_t attr;
      marcel_attr_init(&attr);
      marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr,0);
      marcel_attr_setname(&attr,"thread");
      marcel_create(&t5, &attr, run, (any_t)5);
		*marcel_stats_get(t5, load) = NBSHIFTS*1000;
	}

   {
      marcel_attr_t attr;
      marcel_attr_init(&attr);
      marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr,0);
      marcel_attr_setname(&attr,"thread");
      marcel_create(&t6, &attr, run, (any_t)6);
      *marcel_stats_get(t6, load) = NBSHIFTS*1000;
   }

	{
		marcel_attr_t attr;
      marcel_attr_init(&attr);
      marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr,0);
      marcel_attr_setname(&attr,"thread");
      marcel_create(&t7, &attr, run, (any_t)7);
		*marcel_stats_get(t7, load) = NBSHIFTS*1000;
	}

	{
		marcel_attr_t attr;
      marcel_attr_init(&attr);
      marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr,0);
      marcel_attr_setname(&attr,"thread");
      marcel_create(&t8, &attr, run, (any_t)8);
		*marcel_stats_get(t8, load) = NBSHIFTS*1000;
	}	 
 
	{
		marcel_attr_t attr;
      marcel_attr_init(&attr);
      marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr,0);
      marcel_attr_setname(&attr,"thread");
      marcel_create(&t9, &attr, run, (any_t)9);
		*marcel_stats_get(t9, load) = NBSHIFTS*1000;
	}

   {
      marcel_attr_t attr;
      marcel_attr_init(&attr);
      marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr,0);
      marcel_attr_setname(&attr,"thread");
      marcel_create(&t10, &attr, run, (any_t)10);
      *marcel_stats_get(t10, load) = NBSHIFTS*1000;
   }

	{
		marcel_attr_t attr;
      marcel_attr_init(&attr);
      marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr,0);
      marcel_attr_setname(&attr,"thread");
      marcel_create(&t11, &attr, run, (any_t)11);
		*marcel_stats_get(t11, load) = NBSHIFTS*1000;
	}

	{
		marcel_attr_t attr;
      marcel_attr_init(&attr);
      marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr,0);
      marcel_attr_setname(&attr,"thread");
      marcel_create(&t12, &attr, run, (any_t)12);
		*marcel_stats_get(t12, load) = NBSHIFTS*1000;
	}

	{
		marcel_attr_t attr;
      marcel_attr_init(&attr);
      marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr,0);
      marcel_attr_setname(&attr,"thread");
      marcel_create(&t13, &attr, run, (any_t)13);
		*marcel_stats_get(t13, load) = NBSHIFTS*1000;
	}

   {
      marcel_attr_t attr;
      marcel_attr_init(&attr);
      marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr,0);
      marcel_attr_setname(&attr,"thread");
      marcel_create(&t14, &attr, run, (any_t)14);
      *marcel_stats_get(t14, load) = NBSHIFTS*1000;
   }

	{
		marcel_attr_t attr;
      marcel_attr_init(&attr);
      marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr,0);
      marcel_attr_setname(&attr,"thread");
      marcel_create(&t15, &attr, run, (any_t)15);
		*marcel_stats_get(t15, load) = NBSHIFTS*1000;
	}

	{
		marcel_attr_t attr;
      marcel_attr_init(&attr);
      marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr,0);
      marcel_attr_setname(&attr,"thread");
      marcel_create(&t16, &attr, run, (any_t)16);
		*marcel_stats_get(t16, load) = NBSHIFTS*1000;
	}

   {
		marcel_attr_t attr;
      marcel_attr_init(&attr);
      marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr,0);
      marcel_attr_setname(&attr,"thread");
      marcel_create(&t17, &attr, run, (any_t)17);
		*marcel_stats_get(t17, load) = NBSHIFTS*1000;
	}

   {
      marcel_attr_t attr;
      marcel_attr_init(&attr);
      marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr,0);
      marcel_attr_setname(&attr,"thread");
      marcel_create(&t18, &attr, run, (any_t)18);
      *marcel_stats_get(t18, load) = NBSHIFTS*1000;
   }

	{
		marcel_attr_t attr;
      marcel_attr_init(&attr);
      marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr,0);
      marcel_attr_setname(&attr,"thread");
      marcel_create(&t19, &attr, run, (any_t)19);
		*marcel_stats_get(t19, load) = NBSHIFTS*1000;
	}

	{
		marcel_attr_t attr;
      marcel_attr_init(&attr);
      marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr,0);
      marcel_attr_setname(&attr,"thread");
      marcel_create(&t20, &attr, run, (any_t)20);
		*marcel_stats_get(t20, load) = NBSHIFTS*1000;
	}

	{
		marcel_attr_t attr;
      marcel_attr_init(&attr);
      marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr,0);
      marcel_attr_setname(&attr,"thread");
      marcel_create(&t21, &attr, run, (any_t)21);
		*marcel_stats_get(t21, load) = NBSHIFTS*1000;
	}

   {
      marcel_attr_t attr;
      marcel_attr_init(&attr);
      marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr,0);
      marcel_attr_setname(&attr,"thread");
      marcel_create(&t22, &attr, run, (any_t)22);
      *marcel_stats_get(t22, load) = NBSHIFTS*1000;
   }

	{
		marcel_attr_t attr;
      marcel_attr_init(&attr);
      marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr,0);
      marcel_attr_setname(&attr,"thread");
      marcel_create(&t23, &attr, run, (any_t)23);
		*marcel_stats_get(t23, load) = NBSHIFTS*1000;
	}

	{
		marcel_attr_t attr;
      marcel_attr_init(&attr);
      marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr,0);
      marcel_attr_setname(&attr,"thread");
      marcel_create(&t24, &attr, run, (any_t)24);
		*marcel_stats_get(t24, load) = NBSHIFTS*1000;
	}	 
 
	{
		marcel_attr_t attr;
      marcel_attr_init(&attr);
      marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr,0);
      marcel_attr_setname(&attr,"thread");
      marcel_create(&t25, &attr, run, (any_t)25);
		*marcel_stats_get(t25, load) = NBSHIFTS*1000;
	}

   {
      marcel_attr_t attr;
      marcel_attr_init(&attr);
      marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr,0);
      marcel_attr_setname(&attr,"thread");
      marcel_create(&t26, &attr, run, (any_t)26);
      *marcel_stats_get(t26, load) = NBSHIFTS*1000;
   }

	{
		marcel_attr_t attr;
      marcel_attr_init(&attr);
      marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr,0);
      marcel_attr_setname(&attr,"thread");
      marcel_create(&t27, &attr, run, (any_t)27);
		*marcel_stats_get(t27, load) = NBSHIFTS*1000;
	}

	{
		marcel_attr_t attr;
      marcel_attr_init(&attr);
      marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr,0);
      marcel_attr_setname(&attr,"thread");
      marcel_create(&t28, &attr, run, (any_t)28);
		*marcel_stats_get(t28, load) = NBSHIFTS*1000;
	}

	{
		marcel_attr_t attr;
      marcel_attr_init(&attr);
      marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr,0);
      marcel_attr_setname(&attr,"thread");
      marcel_create(&t29, &attr, run, (any_t)29);
		*marcel_stats_get(t29, load) = NBSHIFTS*1000;
	}

   {
      marcel_attr_t attr;
      marcel_attr_init(&attr);
      marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr,0);
      marcel_attr_setname(&attr,"thread");
      marcel_create(&t30, &attr, run, (any_t)30);
      *marcel_stats_get(t30, load) = NBSHIFTS*1000;
   }

	{
		marcel_attr_t attr;
      marcel_attr_init(&attr);
      marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr,0);
      marcel_attr_setname(&attr,"thread");
      marcel_create(&t31, &attr, run, (any_t)31);
		*marcel_stats_get(t31, load) = NBSHIFTS*1000;
	}

	{
		marcel_attr_t attr;
      marcel_attr_init(&attr);
      marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr,0);
      marcel_attr_setname(&attr,"thread");
      marcel_create(&t32, &attr, run, (any_t)32);
		*marcel_stats_get(t32, load) = NBSHIFTS*1000;
	}

   {
		marcel_attr_t attr;
      marcel_attr_init(&attr);
      marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr,0);
      marcel_attr_setname(&attr,"thread");
      marcel_create(&t33, &attr, run, (any_t)33);
		*marcel_stats_get(t33, load) = NBSHIFTS*1000;
	}

   {
      marcel_attr_t attr;
      marcel_attr_init(&attr);
		marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr,0);
      marcel_attr_setname(&attr,"thread");
      marcel_create(&t34, &attr, run, (any_t)34);
      *marcel_stats_get(t34, load) = NBSHIFTS*1000;
   }

	{
		marcel_attr_t attr;
      marcel_attr_init(&attr);
		marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr,0);
      marcel_attr_setname(&attr,"thread");
      marcel_create(&t35, &attr, run, (any_t)35);
		*marcel_stats_get(t35, load) = NBSHIFTS*1000;
	}

	{
		marcel_attr_t attr;
      marcel_attr_init(&attr);
		marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr,0);
      marcel_attr_setname(&attr,"thread");
      marcel_create(&t36, &attr, run, (any_t)36);
		*marcel_stats_get(t36, load) = NBSHIFTS*1000;
	}

	{
		marcel_attr_t attr;
      marcel_attr_init(&attr);
      marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr,0);
      marcel_attr_setname(&attr,"thread");
      marcel_create(&t37, &attr, run, (any_t)37);
		*marcel_stats_get(t37, load) = NBSHIFTS*1000;
	}

   {
      marcel_attr_t attr;
      marcel_attr_init(&attr);
      marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr,0);
      marcel_attr_setname(&attr,"thread");
      marcel_create(&t38, &attr, run, (any_t)38);
      *marcel_stats_get(t38, load) = NBSHIFTS*1000;
   }

	{
		marcel_attr_t attr;
      marcel_attr_init(&attr);
      marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr,0);
      marcel_attr_setname(&attr,"thread");
      marcel_create(&t39, &attr, run, (any_t)39);
		*marcel_stats_get(t39, load) = NBSHIFTS*1000;
	}

	{
		marcel_attr_t attr;
      marcel_attr_init(&attr);
		marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr,0);
      marcel_attr_setname(&attr,"thread");
      marcel_create(&t40, &attr, run, (any_t)40);
		*marcel_stats_get(t40, load) = NBSHIFTS*1000;
	}	 
 
	{
		marcel_attr_t attr;
      marcel_attr_init(&attr);
		marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr,0);
      marcel_attr_setname(&attr,"thread");
      marcel_create(&t41, &attr, run, (any_t)41);
		*marcel_stats_get(t41, load) = NBSHIFTS*1000;
	}

   {
      marcel_attr_t attr;
      marcel_attr_init(&attr);
		marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr,0);
      marcel_attr_setname(&attr,"thread");
      marcel_create(&t42, &attr, run, (any_t)42);
      *marcel_stats_get(t42, load) = NBSHIFTS*1000;
   }

	{
		marcel_attr_t attr;
      marcel_attr_init(&attr);
		marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr,0);
      marcel_attr_setname(&attr,"thread");
      marcel_create(&t43, &attr, run, (any_t)43);
		*marcel_stats_get(t43, load) = NBSHIFTS*1000;
	}

	{
		marcel_attr_t attr;
      marcel_attr_init(&attr);
		marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr,0);
      marcel_attr_setname(&attr,"thread");
      marcel_create(&t44, &attr, run, (any_t)44);
		*marcel_stats_get(t44, load) = NBSHIFTS*1000;
	}

	{
		marcel_attr_t attr;
      marcel_attr_init(&attr);
		marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr,0);
      marcel_attr_setname(&attr,"thread");
      marcel_create(&t45, &attr, run, (any_t)45);
		*marcel_stats_get(t45, load) = NBSHIFTS*1000;
	}

   {
      marcel_attr_t attr;
      marcel_attr_init(&attr);
		marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr,0);
      marcel_attr_setname(&attr,"thread");
      marcel_create(&t46, &attr, run, (any_t)46);
      *marcel_stats_get(t46, load) = NBSHIFTS*1000;
   }

	{
		marcel_attr_t attr;
      marcel_attr_init(&attr);
		marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr,0);
      marcel_attr_setname(&attr,"thread");
      marcel_create(&t47, &attr, run, (any_t)47);
		*marcel_stats_get(t47, load) = NBSHIFTS*1000;
	}

	{
		marcel_attr_t attr;
      marcel_attr_init(&attr);
		marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr,0);
      marcel_attr_setname(&attr,"thread");
      marcel_create(&t48, &attr, run, (any_t)48);
		*marcel_stats_get(t48, load) = NBSHIFTS*1000;
	}

   {
		marcel_attr_t attr;
      marcel_attr_init(&attr);
		marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr,0);
      marcel_attr_setname(&attr,"thread");
      marcel_create(&t49, &attr, run, (any_t)49);
		*marcel_stats_get(t49, load) = NBSHIFTS*1000;
	}

   {
      marcel_attr_t attr;
      marcel_attr_init(&attr);
		marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr,0);
      marcel_attr_setname(&attr,"thread");
      marcel_create(&t50, &attr, run, (any_t)50);
      *marcel_stats_get(t50, load) = NBSHIFTS*1000;
   }

	{
		marcel_attr_t attr;
      marcel_attr_init(&attr);
		marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr,0);
      marcel_attr_setname(&attr,"thread");
      marcel_create(&t51, &attr, run, (any_t)51);
		*marcel_stats_get(t51, load) = NBSHIFTS*1000;
	}

	{
		marcel_attr_t attr;
      marcel_attr_init(&attr);
		marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr,0);
      marcel_attr_setname(&attr,"thread");
      marcel_create(&t52, &attr, run, (any_t)52);
		*marcel_stats_get(t52, load) = NBSHIFTS*1000;
	}

	{
		marcel_attr_t attr;
      marcel_attr_init(&attr);
		marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr,0);
      marcel_attr_setname(&attr,"thread");
      marcel_create(&t53, &attr, run, (any_t)53);
		*marcel_stats_get(t53, load) = NBSHIFTS*1000;
	}

   {
      marcel_attr_t attr;
      marcel_attr_init(&attr);
		marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr,0);
      marcel_attr_setname(&attr,"thread");
      marcel_create(&t54, &attr, run, (any_t)54);
      *marcel_stats_get(t54, load) = NBSHIFTS*1000;
   }

	{
		marcel_attr_t attr;
      marcel_attr_init(&attr);
		marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr,0);
      marcel_attr_setname(&attr,"thread");
      marcel_create(&t55, &attr, run, (any_t)55);
		*marcel_stats_get(t55, load) = NBSHIFTS*1000;
	}

	{
		marcel_attr_t attr;
      marcel_attr_init(&attr);
		marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr,0);
      marcel_attr_setname(&attr,"thread");
      marcel_create(&t56, &attr, run, (any_t)56);
		*marcel_stats_get(t56, load) = NBSHIFTS*1000;
	}	 
 
	{
		marcel_attr_t attr;
      marcel_attr_init(&attr);
		marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr,0);
      marcel_attr_setname(&attr,"thread");
      marcel_create(&t57, &attr, run, (any_t)57);
		*marcel_stats_get(t57, load) = NBSHIFTS*1000;
	}

   {
      marcel_attr_t attr;
      marcel_attr_init(&attr);
		marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr,0);
      marcel_attr_setname(&attr,"thread");
      marcel_create(&t58, &attr, run, (any_t)58);
      *marcel_stats_get(t58, load) = NBSHIFTS*1000;
   }

	{
		marcel_attr_t attr;
      marcel_attr_init(&attr);
		marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr,0);
      marcel_attr_setname(&attr,"thread");
      marcel_create(&t59, &attr, run, (any_t)59);
		*marcel_stats_get(t59, load) = NBSHIFTS*1000;
	}

	{
		marcel_attr_t attr;
      marcel_attr_init(&attr);
		marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr,0);
      marcel_attr_setname(&attr,"thread");
      marcel_create(&t60, &attr, run, (any_t)60);
		*marcel_stats_get(t60, load) = NBSHIFTS*1000;
	}

	{
		marcel_attr_t attr;
      marcel_attr_init(&attr);
		marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr,0);
      marcel_attr_setname(&attr,"thread");
      marcel_create(&t61, &attr, run, (any_t)61);
		*marcel_stats_get(t61, load) = NBSHIFTS*1000;
	}

   {
      marcel_attr_t attr;
      marcel_attr_init(&attr);
		marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr,0);
      marcel_attr_setname(&attr,"thread");
      marcel_create(&t62, &attr, run, (any_t)62);
      *marcel_stats_get(t62, load) = NBSHIFTS*1000;
   }

	{
		marcel_attr_t attr;
      marcel_attr_init(&attr);
		marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr,0);
      marcel_attr_setname(&attr,"thread");
      marcel_create(&t63, &attr, run, (any_t)63);
		*marcel_stats_get(t63, load) = NBSHIFTS*1000;
	}

	{
		marcel_attr_t attr;
      marcel_attr_init(&attr);
		marcel_attr_setid(&attr,0);
      marcel_attr_setprio(&attr,0);
      marcel_attr_setname(&attr,"thread");
      marcel_create(&t64, &attr, run, (any_t)64);
		*marcel_stats_get(t64, load) = NBSHIFTS*1000;
	}

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
   marcel_fprintf(stderr,"TOTAL TIME : %ld\n", time);

   /* destroy */
	marcel_barrier_destroy(&barrier);
	marcel_barrier_destroy(&allbarrier);
	marcel_cond_destroy(&cond);
	marcel_mutex_destroy(&mutex);

   return 0;
}
