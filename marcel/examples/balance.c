
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
#define NWORKERS 10

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

void marcel_barrier_init(marcel_barrier_t *b, marcel_barrier_attr_t *attr, int num) {
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
marcel_bubble_t bubble[NWORKS];

any_t work(any_t arg) {
	int i = (int) arg;
	unsigned long start;
	int n = iterations;
	int num;
	while (n--) {
		num = marcel_barrier_begin(&barrier[i]);
		start = marcel_clock();
		marcel_barrier_end(&barrier[i], num);
		tprintf("group %d start\n",i);
		while(marcel_clock() < start + works[i]);
		marcel_barrier_wait(&barrier[i]);
		tprintf("group %d wait\n",i);
		marcel_delay(delays[i]);
	}
	tprintf("group %d done\n",i);
	return NULL;
}

int main(int argc, char *argv[]) {
	int i,j;
	marcel_attr_t attr;
	char s[MARCEL_MAXNAMESIZE];

	marcel_init(&argc, argv);

	if (argc>1)
		iterations = atoi(argv[1]);

	marcel_setname(marcel_self(), "balance");

	marcel_attr_init(&attr);

	for (i=0;i<NWORKS;i++) {
		marcel_barrier_init(&barrier[i], NULL, NWORKERS);
		marcel_bubble_init(&bubble[i]);
		marcel_bubble_setprio(&bubble[i], MA_DEF_PRIO);
		marcel_attr_setinitbubble(&attr,&bubble[i]);
		for (j=0;j<NWORKERS;j++) {
			snprintf(s,sizeof(s),"%d-%d",i,j);
			marcel_attr_setname(&attr,s);
			marcel_create(NULL,&attr,work,(any_t) i);
		}
		marcel_wake_up_bubble(&bubble[i]);
	}

	marcel_end();
	return 0;
}
