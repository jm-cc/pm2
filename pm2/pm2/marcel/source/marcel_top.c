
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


#define _GNU_SOURCE
#include "marcel.h"

#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>

int marcel_top_file=-1;
static unsigned long lastms, lastjiffies, djiffies;
static struct ma_timer_list timer;

int top_printf (char *fmt, ...) __attribute__((__format__(printf, 1, 2)));
int top_printf (char *fmt, ...) {
	char buf[81];
	va_list va;
	int n;
	va_start(va,fmt);
	n=tbx_vsnprintf(buf,sizeof(buf),fmt,va);
	va_end(va);
	write(marcel_top_file,buf,n+1);
	return n;
}

void printtask(marcel_task_t *t) {
	unsigned long utime = ma_atomic_read(&t->top_utime);

	top_printf(" %s(%d,%lu%%)", t->name, t->sched.internal.prio, djiffies?(utime*100)/djiffies:0);
	ma_atomic_sub(utime, &t->top_utime);
}
void printrq(ma_runqueue_t *rq) {
	int prio;
	marcel_task_t *t;
	ma_spin_lock(&rq->lock);
	int somebody = 0;
	for (prio=0; prio<MA_MAX_PRIO; prio++) {
		list_for_each_entry(t, rq->active->queue+prio, sched.internal.run_list) {
			printtask(t);
			somebody = 1;
		}
	}
	for (prio=0; prio<MA_MAX_PRIO; prio++) {
		list_for_each_entry(t, rq->expired->queue+prio, sched.internal.run_list) {
			printtask(t);
			somebody = 1;
		}
	}
	ma_spin_unlock(&rq->lock);
	if (somebody)
		top_printf("\r\n");
}

void marcel_top_tick(unsigned long foo) {
#ifdef MA__LWPS
	marcel_lwp_t *lwp;
#endif
	unsigned long now;

	lastms = marcel_clock();
	now = ma_jiffies;
	djiffies = now - lastjiffies;

	top_printf("\e[H\e[J");
	top_printf("top - up %02lu:%02lu:%02lu\r\n", lastms/1000/60/60, (lastms/1000/60)%60, (lastms/1000)%60);
	printrq(&ma_main_runqueue);
#ifndef MA__LWPS
	printtask(ma_per_lwp__current_thread);
	top_printf("\r\n");
#endif
	printrq(&ma_dontsched_runqueue);
#ifdef MA__LWPS
	for_all_lwp(lwp) {
		// cette lecture n'est pas atomique, il peut y avoir un tick
		// entre-temps, ce n'est pas extrêmement grave...
		struct ma_lwp_usage_stat lst = ma_per_lwp(lwp_usage,lwp);
		unsigned long long tot = lst.user + lst.nice + lst.softirq +
			lst.irq + lst.idle;
		if (tot)
			top_printf("\
lwp %u, %3llu%% user %3llu%% nice %3llu%% sirq %3llu%% irq %3llu%% idle\r\n",
			LWP_NUMBER(lwp), lst.user*100/tot, lst.nice*100/tot,
			lst.softirq*100/tot, lst.irq*100/tot, lst.idle*100/tot);
		memset(&ma_per_lwp(lwp_usage,lwp), 0, sizeof(lst));
		printtask(ma_per_lwp(current_thread,lwp));
		top_printf("\r\n");
		printrq(&ma_per_lwp(runqueue,lwp));
		printrq(&ma_per_lwp(dontsched_runqueue,lwp));
	}
#endif
	lastjiffies = now;

	timer.expires += JIFFIES_FROM_US(1000000);
	ma_add_timer(&timer);
}

int marcel_init_top(char *outfile) {
	mdebug("marcel_init_top(%s)\n",outfile);
	if (*outfile=='|') {
		int fds[2];
		outfile++;
		if (socketpair(PF_UNIX,SOCK_STREAM,0,fds)<0) {
			perror("socketpair");
			return -1;
		}
		marcel_top_file=fds[0];
		if (!fork()) {
			close(marcel_top_file);
			dup2(fds[1],STDIN_FILENO);
			dup2(fds[1],STDOUT_FILENO);
			dup2(fds[1],STDERR_FILENO);
			system(outfile);
			exit(0);
		}
		close(fds[1]);
		// TODO récupérer le pid et le tuer proprement (il se termine dès qu'on tape dedans...)
	} else if ((marcel_top_file=open(outfile,O_WRONLY))<0) {
		perror("opening top file");
		return -1;
	}

	ma_init_timer(&timer);
	timer.expires = ma_jiffies + JIFFIES_FROM_US(1000000);
	timer.function = marcel_top_tick;
	ma_add_timer(&timer);
	return 0;
}
