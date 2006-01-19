
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

#undef MA__BUBBLES

#define NWORKS 2
#define NWORKERS_POW 3
#define NWORKERS (1<<NWORKERS_POW)
//#define TREE
#if 0
#define BARRIER
#else
#define PIPE
#endif

int iterations = 3;

#ifdef BARRIER
unsigned long works[]  = { 5000, 7000 };
unsigned long delays[] = { 5000, 7000 };
marcel_barrier_t barrier[NWORKS];
#endif
#ifdef PIPE
#define DATASIZE (512<<10)
#endif

#ifdef MA__BUBBLES
marcel_bubble_t bubbles[
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
any_t work(any_t arg) {
	int i = (int) arg;
	unsigned long start;
	int n = iterations;
	int num;
	while (n--) {
		tprintf("group %d begin\n",i);
		num = marcel_barrier_begin(&barrier[i]);
		start = marcel_clock();
		marcel_barrier_end(&barrier[i], num);
		tprintf("group %d do\n",i);
		while(marcel_clock() < start + works[i]);
		tprintf("group %d done\n",i);
		marcel_barrier_wait(&barrier[i]);
		tprintf("group %d wait\n",i);
		marcel_delay(delays[i]);
		tprintf("group %d done\n",i);
	}
	tprintf("group %d finished\n",i);
	return NULL;
}
#endif

#ifdef PIPE
any_t producer(any_t arg) {
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
			dataw[i]=random();
		buffer_write = (buffer_write+1)%NBBUFS;
		marcel_sem_V(&reader[group][me]);
		marcel_fprintf(stderr,"%d: %d produced unit\n",group,me);
	}
	return NULL;
}
any_t consumer(any_t arg) {
	int i = (int) arg;
	int group = i/NWORKERS;
	int me = i%NWORKERS;
	int n = iterations;
	int buffer_read = 0;
	unsigned char *datar;
	unsigned long sum;
	while (n--) {
		marcel_sem_P(&reader[group][me-1]);
		datar = data[group][me]+buffer_read*DATASIZE;
		sum = 0;
		for (i=0;i<DATASIZE;i++)
			sum+=datar[i]+random();
		buffer_read = (buffer_read+1)%NBBUFS;
		marcel_sem_V(&writer[group][me-1]);
		marcel_fprintf(stderr,"%d: %d consumed unit\n",group,me);
	}
	return NULL;
}
any_t piper(any_t arg) {
	int i = (int) arg;
	int group = i/NWORKERS;
	int me = i%NWORKERS;
	int n = iterations;
	int buffer_read = 0;
	int buffer_write = 0;
	unsigned char *datar, *dataw;
	while (n--) {
		marcel_sem_P(&reader[group][me-1]);
		datar = data[group][me]+buffer_read*DATASIZE;
		marcel_sem_P(&writer[group][me]);
		dataw = data[group][me]+buffer_write*DATASIZE;
		for (i=0;i<DATASIZE;i++)
			dataw[i]=datar[i]+random();
		buffer_read = (buffer_read+1)%NBBUFS;
		buffer_write = (buffer_write+1)%NBBUFS;
		marcel_sem_V(&reader[group][me]);
		marcel_sem_V(&writer[group][me-1]);
		marcel_fprintf(stderr,"%d: %d piped unit\n",group,me);
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
				data[i][j] = malloc(NBBUFS*DATASIZE);
				marcel_sem_init(&reader[i][j],0);
				marcel_sem_init(&writer[i][j],NBBUFS);
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
					j ?
						j<NWORKERS-1?piper:consumer
						: producer,
					(any_t) (i*NWORKERS+j));
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
