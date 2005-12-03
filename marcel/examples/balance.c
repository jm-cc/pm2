
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

#define NWORKS 2
#define NWORKERS_POW 3
#define NWORKERS (1<<NWORKERS_POW)
//#define TREE

unsigned long works[]  = { 5000, 7000 };
unsigned long delays[] = { 5000, 7000 };
int iterations = 3;

/* implémentation en 2s des barrières */
typedef struct {
	marcel_cond_t cond;
	marcel_mutex_t mutex;
	int num;
	int curwait;
} marcel_barrier_t;
typedef struct { int foo; } marcel_barrier_attr_t;

void marcel_barrier_init(marcel_barrier_t * __restrict b,
		marcel_barrier_attr_t * __restrict attr, int num) {
	marcel_cond_init(&b->cond, NULL);
	marcel_mutex_init(&b->mutex, NULL);
	b->num = num;
	b->curwait = 0;
}
int marcel_barrier_begin(marcel_barrier_t *b) {
	int num;
	marcel_mutex_lock(&b->mutex);
	if ((num = ++b->curwait) == b->num) {
		b->curwait = 0;
		marcel_cond_broadcast(&b->cond);
	}
	marcel_mutex_unlock(&b->mutex);
	return num;
}
void marcel_barrier_end(marcel_barrier_t *b, int num) {
	marcel_mutex_lock(&b->mutex);
	if (num != b->num)
		marcel_cond_wait(&b->cond,&b->mutex);
	marcel_mutex_unlock(&b->mutex);
}
void marcel_barrier_wait(marcel_barrier_t *b) {
	marcel_mutex_lock(&b->mutex);
	if (++b->curwait == b->num) {
		b->curwait = 0;
		marcel_cond_broadcast(&b->cond);
	} else
		marcel_cond_wait(&b->cond,&b->mutex);
	marcel_mutex_unlock(&b->mutex);
}
/* fin */

marcel_barrier_t barrier[NWORKS];
marcel_bubble_t bubbles[
#ifdef TREE
	(NWORKERS-1)*
#endif
	NWORKS];

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

int marcel_main(int argc, char *argv[]) {
	int i,j;
	marcel_attr_t attr;
	char s[MARCEL_MAXNAMESIZE];
#ifdef TREE
	marcel_bubble_t *b;
	int k,n,m;
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
		marcel_barrier_init(&barrier[i], NULL, NWORKERS);
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
		for (j=0;j<NWORKERS;j++) {
			snprintf(s,sizeof(s),"%d-%d",i,j);
			marcel_attr_setname(&attr,s);
			marcel_attr_setinitbubble(&attr,
#ifdef TREE
					&bubbles[(NWORKERS-1)*i+m+j/2]
#else
					&bubbles[i]
#endif
					);
			marcel_create(NULL,&attr,work,(any_t) i);
		}
		marcel_wake_up_bubble(&bubbles[
#ifdef TREE
				(NWORKERS-1)*
#endif
				i]);
	}

	marcel_end();
	return 0;
}
