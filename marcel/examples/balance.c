
/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 "the PM2 team" (see AUTHORS file)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

#include "marcel.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

//#undef MA__BUBBLES

#define NWORKS 2
#define NWORKERS_POW 2
#define NWORKERS (1<<NWORKERS_POW)
#define TREE
#if 1

#define BARRIER
#if 1
#define CACHEMISS
#else
#define CACHESHARE
#endif

#else

#define PIPE

#endif

static int iterations = 3;

#ifdef BARRIER
static unsigned long works[]  = { 5000, 7000, 12000, 5000 };
static unsigned long delays[] = { 0, 0, 0, 5000 };
static marcel_barrier_t barrier[NWORKS];
#endif
#ifdef PIPE
#define DATASIZE (3000<<10)
#endif

#ifdef MA__BUBBLES
static marcel_bubble_t bubbles[
#ifdef TREE
	(NWORKERS-1)*
#endif
	NWORKS];
#endif


#ifdef PIPE
#define NBBUFS 3
static void *data[NWORKS][NWORKERS-1];
static marcel_sem_t reader[NWORKS][NWORKERS-1];
static marcel_sem_t writer[NWORKS][NWORKERS-1];
#endif

#ifdef BARRIER
#ifdef CACHEMISS
#define MULT 1U
#define CACHESIZE ((MULT*128U)<<10)
typedef int data_t;
static volatile data_t data[NWORKS][CACHESIZE/sizeof(data_t)];
#else
static volatile float data[NWORKS][128];
#endif

static any_t work(any_t arg) {
	int i = (int)(intptr_t) arg;
	int group = i/NWORKERS;
	int me = i%NWORKERS;
	unsigned long start;
	int n = iterations;
	unsigned num,num2;
#ifdef CACHEMISS
	data_t f,g;
	data_t sum = 0;
	data_t gold = (data_t) fmod(((float)(CACHESIZE/sizeof(*data)))*((sqrt(5)+1.)/2.) , (float)(CACHESIZE/sizeof(*data)));
#endif
	while (n--) {
		tprintf("group %d (%d) begin %p\n",group,me,&data[group][me]);
		//num = marcel_barrier_begin(&barrier[group]);
		start = marcel_clock();
		//marcel_barrier_end(&barrier[group], num);
		tprintf("group %d (%d) do\n",group,me);
		i = 0;
		sum = 0;
		g = 0;
		while(marcel_clock() < start + works[group]) {
#ifdef CACHEMISS
#if 0
			for (num2=0;num2<128U/MULT;num2++)
			for (num=0;num<CACHESIZE;num++)
				sum += data[group][(num*gold)%(CACHESIZE/sizeof(char))];
#endif
			for (f=0;f< 1<<20;f++) {
				g=g+gold;
				if (g>CACHESIZE/sizeof(*data)) g-= CACHESIZE/sizeof(*data);
				//for (g=CACHESIZE/sizeof(*data);g>0;g-=64/sizeof(*data))
				//for (g=0;g!=CACHESIZE/sizeof(*data);g+=64/sizeof(*data))
				//for (g=0;g<CACHESIZE/sizeof(*data);g+=87*64/sizeof(*data))
					sum+=data[group][(int)(g)];
			}
#else
			data[group][me] = 10000.;
			while(data[group][me]>1.) {
				data[group][me]*=1.00001;
				data[group][me]/=1.0001;
			}
#endif
			i++;
		}
		tprintf("group %d (%d) done (%ul)\n", group, me, i);
		marcel_barrier_wait(&barrier[group]);
		tprintf("group %d (%d) wait\n",group,me);
		marcel_delay(delays[group]);
		tprintf("group %d (%d) done\n",group,me);
	}
	tprintf("group %d (%d) finished\n",group,me);
	return NULL;
}
#endif

#ifdef PIPE
static any_t producer(any_t arg) {
	int i = (int) arg;
	int group = i/NWORKERS;
	int me = i%NWORKERS;
	int n = iterations;
	int buffer_write = 0;
	unsigned char *dataw;
	while (n--) {
		marcel_sem_P(&writer[group][me]);
		dataw = data[group][me]+buffer_write*DATASIZE;
		for (i=0;i<DATASIZE;i++)
			dataw[i]=marcel_random();
		buffer_write = (buffer_write+1)%NBBUFS;
		marcel_sem_V(&reader[group][me]);
		if (!(n%10))
			marcel_fprintf(stderr,"%d: %d produced unit\n",group,me);
	}
	return NULL;
}
static any_t consumer(any_t arg) {
	int i = (int) arg;
	int group = i/NWORKERS;
	int me = i%NWORKERS;
	int n = iterations;
	int buffer_read = 0;
	unsigned char *datar;
	unsigned long sum;
	while (n--) {
		marcel_sem_P(&reader[group][me-1]);
		datar = data[group][me-1]+buffer_read*DATASIZE;
		sum = 0;
		for (i=0;i<DATASIZE;i++)
			sum+=datar[i]+marcel_random();
		buffer_read = (buffer_read+1)%NBBUFS;
		marcel_sem_V(&writer[group][me-1]);
	}
	return NULL;
}
static any_t piper(any_t arg) {
	int i = (int) arg;
	int group = i/NWORKERS;
	int me = i%NWORKERS;
	int n = iterations;
	int buffer_read = 0;
	int buffer_write = 0;
	unsigned char *datar, *dataw;
	while (n--) {
		marcel_sem_P(&reader[group][me-1]);
		datar = data[group][me-1]+buffer_read*DATASIZE;
		marcel_sem_P(&writer[group][me]);
		dataw = data[group][me]+buffer_write*DATASIZE;
		for (i=0;i<DATASIZE;i++)
			dataw[i]=datar[i]+marcel_random();
		buffer_read = (buffer_read+1)%NBBUFS;
		buffer_write = (buffer_write+1)%NBBUFS;
		marcel_sem_V(&reader[group][me]);
		marcel_sem_V(&writer[group][me-1]);
	}
	return NULL;
}
#endif

int marcel_main(int argc, char *argv[]) {
	int i,j;
	marcel_attr_t attr;
	char s[MARCEL_MAXNAMESIZE];
#ifdef MA__BUBBLES
#ifdef TREE
	marcel_bubble_t *b;
	int k,n,m;
#endif
#endif

	marcel_init(&argc, argv);

	if (argc>1)
		iterations = atoi(argv[1]);

	marcel_setname(marcel_self(), "balance");

	marcel_attr_init(&attr);

#ifdef PROFILE
   profile_activate(FUT_ENABLE, MARCEL_PROF_MASK, 0);
#endif

	for (i=0;i<NWORKS;i++) {
#ifdef BARRIER
		marcel_barrier_init(&barrier[i], NULL, NWORKERS);
#endif
#ifdef MA__BUBBLES
#ifdef TREE
		n = m = 0;
		for (j=0;j<NWORKERS_POW;j++) {
			for (k=0;k<1<<j;k++) {
				b = &bubbles[(NWORKERS-1)*i+n+k];
				marcel_bubble_init(b);
				marcel_bubble_setprio(b, MA_DEF_PRIO);
				marcel_bubble_setschedlevel(b, j);
				if (n)
					marcel_bubble_insertbubble(&bubbles[(NWORKERS-1)*i+m+k/2], b);
			}
			m = n;
			n += 1<<j;
		}
#else
		marcel_bubble_init(&bubbles[i]);
#endif
#endif
		for (j=0;j<NWORKERS;j++) {
			if (j < NWORKERS-1) {
#ifdef PIPE
				data[i][j] = malloc(NBBUFS*DATASIZE);
				marcel_sem_init(&reader[i][j],0);
				marcel_sem_init(&writer[i][j],NBBUFS);
#endif
			}
			snprintf(s,sizeof(s),"%d-%d",i,j);
			marcel_attr_setname(&attr,s);
#ifdef MA__BUBBLES
			marcel_attr_setinitbubble(&attr,
#ifdef TREE
					&bubbles[(NWORKERS-1)*i+m+j/2]
#else
					&bubbles[i]
#endif
					);
#endif
			marcel_create(NULL,&attr,
#ifdef BARRIER
					work,
#else
					j ?
						j<NWORKERS-1?piper:consumer
						: producer,
#endif
					(any_t)(intptr_t)(i*NWORKERS+j));
		}
#ifdef MA__BUBBLES
		marcel_wake_up_bubble(&bubbles[
#ifdef TREE
				(NWORKERS-1)*
#endif
				i]);
#endif
	}

	marcel_end();
	return 0;
}
