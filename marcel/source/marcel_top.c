
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
#include "tbx_compiler.h"

#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>

int marcel_top_file=-1;
static unsigned long lastms, lastjiffies, djiffies;
static struct ma_timer_list timer;

int top_printf (char *fmt, ...) TBX_FORMAT(printf, 1, 2);
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

static char *get_holder_name(ma_holder_t *h, char *buf, int size) {
	if (!h)
		return "nil";

	if (ma_holder_type(h) == MA_RUNQUEUE_HOLDER)
		return ma_rq_holder(h)->name;

	snprintf(buf,size,"%p",ma_bubble_holder(h));
	return buf;
}

static void printtask(marcel_task_t *t) {
	unsigned long utime;
	char state;
	char buf1[32];
	char buf2[32];

	switch (t->sched.state) {
		case MA_TASK_RUNNING: 		state = 'R'; break;
		case MA_TASK_INTERRUPTIBLE:	state = 'I'; break;
		case MA_TASK_UNINTERRUPTIBLE:	state = 'U'; break;
		//case MA_TASK_STOPPED:		state = 'S'; break;
		//case MA_TASK_TRACED:		state = 'T'; break;
		//case MA_TASK_ZOMBIE:		state = 'Z'; break;
		case MA_TASK_DEAD:		state = 'D'; break;
		//case MA_TASK_GHOST:		state = 'G'; break;
		case MA_TASK_MOVING:		state = 'M'; break;
		case MA_TASK_FROZEN:		state = 'F'; break;
		case MA_TASK_BORNING:		state = 'B'; break;
		default:			state = '?'; break;
	}
	utime = ma_atomic_read(&t->top_utime);
	top_printf("0x%p %16s %2d %3lu%% %c %2d %s %s\r\n", t, t->name,
		t->sched.internal.prio, djiffies?(utime*100)/djiffies:0,
		state, LWP_NUMBER(GET_LWP(t)),
		get_holder_name(ma_task_holder(t),buf1,sizeof(buf1)),
		get_holder_name(ma_task_cur_holder(t),buf2,sizeof(buf2)));
	ma_atomic_sub(utime, &t->top_utime);
}

#if 0
void printbubble(int sep, marcel_bubble_t *b) {
	int rsep=0;
	marcel_bubble_entity_t *e;

	top_printf("%s(",sep?" ":"");
	if (b->status == MA_BUBBLE_CLOSED)
		list_for_each_entry(e, &b->heldentities, entity_list) {
			printentity(rsep,e);
			rsep=1;
		}
	top_printf(")(%d)", b->sched.prio);
}

void printentity(int sep, marcel_bubble_entity_t *e) {
	if (e->type == MARCEL_TASK_ENTITY) {
		printtask(sep,tbx_container_of(e, marcel_task_t, sched.internal ));
	} else {
		printbubble(sep,tbx_container_of(e, marcel_bubble_t, sched));
	}
}

void printrq(ma_runqueue_t *rq) {
	int prio;
	marcel_bubble_entity_t *e;
	ma_spin_lock(&rq->lock);
	int somebody = 0;
	for (prio=0; prio<MA_MAX_PRIO; prio++) {
		list_for_each_entry(e, rq->active->queue+prio, run_list) {
			printentity(1,e);
			somebody = 1;
		}
	}
	for (prio=0; prio<MA_MAX_PRIO; prio++) {
		list_for_each_entry(e, rq->expired->queue+prio, run_list) {
			printentity(1,e);
			somebody = 1;
		}
	}
	ma_spin_unlock(&rq->lock);
	if (somebody)
		top_printf("\r\n");
}
#endif

void marcel_top_tick(unsigned long foo) {
	marcel_lwp_t *lwp;
	unsigned long now;
#define NBPIDS 22
	marcel_t pids[NBPIDS];
	int nbpids;

	lastms = marcel_clock();
	now = ma_jiffies;
	djiffies = now - lastjiffies;
	lastjiffies = now;

	top_printf("\e[H\e[J");
	top_printf("top - up %02lu:%02lu:%02lu\r\n", lastms/1000/60/60, (lastms/1000/60)%60, (lastms/1000)%60);

	//printrq(&ma_main_runqueue);
#if 0
#ifndef MA__LWPS
	printtask(1,ma_per_lwp__current_thread);
	top_printf("\r\n");
#endif
	printrq(&ma_dontsched_runqueue);
#endif
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
#if 0
		printtask(1,ma_per_lwp(current_thread,lwp));
		top_printf("\r\n");
		printrq(&ma_per_lwp(runqueue,lwp));
		printrq(&ma_per_lwp(dontsched_runqueue,lwp));
#endif
	}
	marcel_freeze_sched();
	marcel_threadslist(NBPIDS,pids,&nbpids,0);
	if (nbpids > NBPIDS)
		nbpids = NBPIDS;
	for (;nbpids;nbpids--)
		printtask(pids[nbpids-1]);
	marcel_unfreeze_sched();

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
	} else if ((marcel_top_file=open(outfile,O_WRONLY|O_APPEND|O_CREAT))<0) {
		perror("opening top file");
		return -1;
	}

	ma_init_timer(&timer);
	timer.expires = ma_jiffies + JIFFIES_FROM_US(1000000);
	timer.function = marcel_top_tick;
	ma_add_timer(&timer);
	return 0;
}

