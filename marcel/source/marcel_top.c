
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
#include "tbx_compiler.h"

#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#ifndef WIN_SYS
#include <sys/socket.h>
#endif

#define TOP_SEC 3

static int top_pid;
static int top_file=-1;
static unsigned long lastms, lastjiffies, djiffies;
static struct ma_timer_list timer;
#ifdef MA__BUBBLES
static int bubbles;
#endif

static int top_printf (char *fmt, ...) TBX_FORMAT(printf, 1, 2);
static int top_printf (char *fmt, ...) {
	char buf[120];
	va_list va;
	int n;
	va_start(va,fmt);
	n=tbx_vsnprintf(buf,sizeof(buf),fmt,va);
	va_end(va);
	write(top_file,buf,n+1);
	return n;
}

static char *get_holder_name(ma_holder_t *h, char *buf, int size) {
	if (!h)
		return "nil";

	if (ma_holder_type(h) == MA_RUNQUEUE_HOLDER)
		return ma_rq_holder(h)->name;

#ifdef MA__BUBBLES
	snprintf(buf,size,"%p",ma_bubble_holder(h));
	return buf;
#else
	return "??";
#endif
}

static void printtask(marcel_task_t *t) {
	unsigned long utime;
	char state;
	char buf1[32];
	char buf2[32];
	char buf3[32];
	unsigned long cpu; /* en pour mille */

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
	cpu = djiffies?(utime*1000UL)/djiffies:0;
	top_printf("%#*lx %*s %2d %3lu.%1lu %c %2d %10s %10s %10s\r\n",
		(int) (2*sizeof(void*)), (unsigned long) t,
        	MARCEL_MAXNAMESIZE, t->name,
		t->sched.internal.entity.prio, cpu/10UL, cpu%10UL,
		state, GET_LWP_NUMBER(t),
		get_holder_name(ma_task_init_holder(t),buf1,sizeof(buf1)),
		get_holder_name(ma_task_sched_holder(t),buf2,sizeof(buf2)),
		get_holder_name(ma_task_run_holder(t),buf3,sizeof(buf3)));
	ma_atomic_sub(utime, &t->top_utime);
}

#ifdef MA__BUBBLES
static void printbubble(marcel_bubble_t *b, int indent) {
	marcel_entity_t *e;
	char buf1[32];
	char buf2[32];
	char buf3[32];
	top_printf("%#*lx %*s %2d            %10s %10s %10s\r\n",
		(int) (2*sizeof(void*)), (unsigned long) b,
        	MARCEL_MAXNAMESIZE, "bubble",
		b->sched.prio,
		get_holder_name(b->sched.init_holder,buf1,sizeof(buf1)),
		get_holder_name(b->sched.sched_holder,buf2,sizeof(buf2)),
		get_holder_name(b->sched.run_holder,buf3,sizeof(buf3)));
	list_for_each_entry(e, &b->heldentities, bubble_entity_list) {
		if (e->type == MA_TASK_ENTITY) {
			top_printf("%*s", indent, "");
			printtask(ma_task_entity(e));
		} else {
			printbubble(ma_bubble_entity(e), indent+1);
		}
	}
}
#endif

static void marcel_top_tick(unsigned long foo) {
	marcel_lwp_t *lwp;
	unsigned long now;
#define NBPIDS 22
	marcel_t pids[NBPIDS];
	int nbpids;
	unsigned char cmd;

	top_printf("\e[H\e[J");
	while (read(top_file,&cmd,1)==1)
		switch(cmd) {
#ifdef MA__BUBBLES
		case 'b':
			bubbles = !bubbles;
			top_printf("bubble view %sactivated\r\n",bubbles?"":"de");
			break;
#endif
		case 'h':
		case '?':
#ifdef MA__BUBBLES
			top_printf("b:\ttoggle bubble / lwp view\r\n");
#endif
			top_printf("h/?:\thelp\r\n");
			break;
		default:
			top_printf("unknown command %c\r\n",cmd);
			break;
		}

	lastms = marcel_clock();
	now = ma_jiffies;
	djiffies = now - lastjiffies;
	lastjiffies = now;

	top_printf("top - up %02lu:%02lu:%02lu\r\n", lastms/1000/60/60, (lastms/1000/60)%60, (lastms/1000)%60);

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
	}
	top_printf("  %*s %*s %2s %4s%% %s %2s %10s %10s %10s\r\n",
		(int) (2*sizeof(void*)), "self", MARCEL_MAXNAMESIZE,
		"name", "pr", "cpu", "s", "lc", "init", "sched", "run");
	marcel_freeze_sched();
#ifdef MA__BUBBLES
	if (bubbles)
		printbubble(&marcel_root_bubble, 0);
	else
#endif
	{
		marcel_threadslist(NBPIDS,pids,&nbpids,0);
		if (nbpids > NBPIDS)
			nbpids = NBPIDS;
		for (;nbpids;nbpids--)
			printtask(pids[nbpids-1]);
	}
	marcel_unfreeze_sched();

	timer.expires += JIFFIES_FROM_US(TOP_SEC * 1000000);
	ma_add_timer(&timer);
}

int marcel_init_top(char *outfile) {
	int fl;
	mdebug("marcel_init_top(%s)\n",outfile);
#ifndef WIN_SYS
	if (*outfile=='|') {
		int fds[2];
		outfile++;
		if (socketpair(AF_UNIX,SOCK_STREAM,0,fds)<0) {
			perror("socketpair");
			return -1;
		}
		top_file=fds[0];
		if (!(top_pid = fork())) {
			setpgid(0,0);
			close(top_file);
			dup2(fds[1],STDIN_FILENO);
			dup2(fds[1],STDOUT_FILENO);
			//dup2(fds[1],STDERR_FILENO);
			if (system(outfile) == -1) {
				perror("system");
				fprintf(stderr,"couldn't execute %s\n", outfile);
			}
			exit(0);
		}
		close(fds[1]);
		// TODO récupérer le pid et le tuer proprement (il se termine dès qu'on tape dedans...)
	} else 
#endif
	if ((top_file=open(outfile,O_WRONLY|O_APPEND|O_CREAT, 0644))<0) {
		perror("opening top file");
		return -1;
	}

	fl = fcntl(top_file, F_GETFL);
	fcntl(top_file, F_SETFL, fl | O_NONBLOCK);

	ma_init_timer(&timer);
	timer.expires = ma_jiffies + JIFFIES_FROM_US(1000000);
	timer.function = marcel_top_tick;
	ma_add_timer(&timer);
	return 0;
}

void marcel_exit_top(void) {
#ifndef WIN_SYS
	if (top_pid) {
		mdebug("killing top program %d\n", top_pid);
		kill(-top_pid, SIGTERM);
	}
#endif
}
