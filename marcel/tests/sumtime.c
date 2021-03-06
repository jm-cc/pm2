
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
#include "tbx_types.h"

//#define CPU0ONLY
//#define dprintf(fmt,args...) marcel_fprintf(stderr,fmt,##args)
#define dprintf(fmt,args...) (void)0
//#undef MA__BUBBLES

#define MAX_BUBBLE_LEVEL 100
//#define MAX_BUBBLE_LEVEL 0

typedef struct {
	int inf, sup, res, display_time;
	marcel_sem_t sem;
	int level;
} job;

static marcel_attr_t attr;

static __inline__ void job_init(job * j, int inf, int sup, int level, int display_time)
{
	j->inf = inf;
	j->sup = sup;
	marcel_sem_init(&j->sem, 0);
	j->level = level;
	j->display_time = display_time;
}

static any_t sum(any_t arg)
{
	job *j = (job *) arg;
	job j1, j2;
	char name[MARCEL_MAXNAMESIZE];

	marcel_snprintf(name, sizeof(name), "%d-%d", j->inf, j->sup);
	marcel_setname(marcel_self(), name);

	dprintf("sum(%s,%p,%p,%p) started on %d\n", name, j, &j1, &j2,
		marcel_current_vp());

	if (j->inf == j->sup) {
		j->res = j->inf;
		dprintf("base %s\n", name);
		marcel_sem_V(&j->sem);
		return NULL;
	}

	job_init(&j1, j->inf, (j->inf + j->sup) / 2, j->level + 1, j->display_time);
	job_init(&j2, (j->inf + j->sup) / 2 + 1, j->sup, j->level + 1, j->display_time);

#ifdef MA__BUBBLES
	if (j->level < MAX_BUBBLE_LEVEL) {
		marcel_bubble_t b1, b2;
		marcel_attr_t commattr;

		marcel_attr_init(&commattr);
		marcel_attr_setdetachstate(&commattr, tbx_true);
#ifdef CPU0ONLY
		marcel_attr_setvpset(&commattr, MARCEL_VPSET_VP(0));
#endif

		marcel_bubble_init(&b1);
		marcel_bubble_init(&b2);
		marcel_bubble_setschedlevel(&b1, j->level);
		marcel_bubble_setschedlevel(&b2, j->level);
		marcel_bubble_insertbubble(marcel_bubble_holding_task(marcel_self()), &b1);
		marcel_bubble_insertbubble(marcel_bubble_holding_task(marcel_self()), &b2);
		marcel_attr_setprio(&commattr, MA_BATCH_PRIO);
		marcel_attr_setnaturalbubble(&commattr, &b1);
		marcel_create(NULL, &commattr, sum, (any_t) & j1);
		marcel_attr_setnaturalbubble(&commattr, &b2);
		marcel_create(NULL, &commattr, sum, (any_t) & j2);
		marcel_bubble_join(&b1);
		marcel_bubble_join(&b2);
	} else
#endif
	{
		marcel_create(NULL, &attr, sum, (any_t) & j1);
		marcel_create(NULL, &attr, sum, (any_t) & j2);
		marcel_sem_P(&j1.sem);
		marcel_sem_P(&j2.sem);
	}

	j->res = j1.res + j2.res;
	dprintf("res %d\n", j->res);
	marcel_sem_V(&j->sem);
	return NULL;
}

static tbx_tick_t t1, t2;

static int compute(job j)
{
	unsigned long temps;

	TBX_GET_TICK(t1);
	marcel_create(NULL, &attr, sum, (any_t) & j);
	marcel_sem_P(&j.sem);

	TBX_GET_TICK(t2);
#ifdef VERBOSE_TESTS
	marcel_printf("Sum from 1 to %d = %d\n", j.sup, j.res);
#endif
	temps = TBX_TIMING_DELAY(t1, t2);
	if (j.display_time)
                marcel_printf("sum(1..%d)=%d time=%ld.%03ldms\n", j.sup, j.res, temps / 1000, temps % 1000);
	return j.res;
}

int main(int argc, char **argv)
{
	int inf = 1;
	int sup = 100;
	int expected = 5050;
	int comp_res;
	job j;

	marcel_trace_on();
	marcel_init(&argc, argv);

	marcel_setname(marcel_self(), "sumtime");

	marcel_attr_init(&attr);
	marcel_attr_setdetachstate(&attr, tbx_true);
	marcel_attr_setschedpolicy(&attr, MARCEL_SCHED_AFFINITY);
	//marcel_attr_setschedpolicy(&attr, MARCEL_SCHED_SHARED);
	marcel_attr_setinheritholder(&attr, tbx_true);
	marcel_attr_setprio(&attr, MA_BATCH_PRIO);
#ifdef CPU0ONLY
	marcel_attr_setvpset(&attr, MARCEL_VPSET_VP(0));
#endif

	marcel_sem_init(&j.sem, 0);
	j.inf = inf;
	j.sup = sup;
	j.level = 0;
	j.display_time = 1;

#ifdef PROFILE
	profile_activate(FUT_ENABLE, MARCEL_PROF_MASK, 0);
#endif

	comp_res = compute(j);
#ifdef PROFILE
	profile_stop();
#endif

	marcel_fflush(stdout);
	marcel_end();
	return (comp_res == expected) ? MARCEL_TEST_SUCCEEDED : comp_res;
}
