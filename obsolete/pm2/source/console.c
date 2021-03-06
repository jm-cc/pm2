
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

#define PM2_CONSOLE_C
#include "pm2.h"

#include <ctype.h>

static long xtoi(char *p)
{
	long i = 0;
	char c;

	while (isxdigit((int)(c = *p++))) {
		i = (i << 4) + c - (isdigit((int)c) ? '0' : (isupper((int)c) ? 'A' : 'a') - 10);
	}
	return i;
}

int nb_thread;    

static void snap(marcel_t p)
{
  long tamp;
  int temp = 1;
  char buf[16];

   old_mad_pack_int(MAD_IN_HEADER, &temp, 1);
   sprintf(buf, "%lx", (unsigned long)p);
   old_mad_pack_str(MAD_IN_HEADER, buf);
   tamp=p->number;
   old_mad_pack_long(MAD_IN_HEADER, &tamp, 1);
   temp=1;
   old_mad_pack_int(MAD_IN_HEADER, &temp, 1);
   temp=p->state;
   old_mad_pack_int(MAD_IN_HEADER, &temp, 1);
   temp=64*1024;
   old_mad_pack_int(MAD_IN_HEADER, &temp, 1);
   old_mad_pack_str(MAD_IN_HEADER, pm2_lrpc_name(pm2_lrpc_runned_by(p)));
   temp = p->not_migratable;
   old_mad_pack_int(MAD_IN_HEADER, &temp, 1);
}


static BEGIN_SERVICE(LRPC_CONSOLE_THREADS)
END_SERVICE(LRPC_CONSOLE_THREADS)


PACK_REQ_STUB(LRPC_CONSOLE_THREADS)
END_STUB

UNPACK_REQ_STUB(LRPC_CONSOLE_THREADS)
END_STUB

PACK_RES_STUB(LRPC_CONSOLE_THREADS)
 int zero=0;
  
  marcel_snapshot(snap);
  old_mad_pack_int(MAD_IN_HEADER, &zero, 1);
END_STUB

UNPACK_RES_STUB(LRPC_CONSOLE_THREADS)
 char s1[5], s2[5], rpc_name[64], thread_id[16];
 long num_threads;
 int encore, prio_threads, state_threads, pile_threads, not_migratable;

  do {
      old_mad_unpack_int(MAD_IN_HEADER, &encore, 1);
      if(encore==0)
		break;
      old_mad_unpack_str(MAD_IN_HEADER, thread_id);
      old_mad_unpack_long(MAD_IN_HEADER, &num_threads, 1);
      old_mad_unpack_int(MAD_IN_HEADER, &prio_threads, 1);
      old_mad_unpack_int(MAD_IN_HEADER, &state_threads, 1);
      old_mad_unpack_int(MAD_IN_HEADER, &pile_threads, 1);
      old_mad_unpack_str(MAD_IN_HEADER, rpc_name);
      old_mad_unpack_int(MAD_IN_HEADER, &not_migratable, 1);
      if(state_threads == MA_TASK_RUNNING)
		sprintf(s1, "run");
      else if(state_threads == MA_TASK_UNINTERRUPTIBLE)
	        sprintf(s1, "blocked");
      else
		sprintf(s1, "asleep");

      if(not_migratable)
      		sprintf(s2, "no");
	else
		sprintf(s2, "yes");

      tprintf("\t%16s  %6ld  %4d  %7s  %6d  %10s  %s\n", thread_id,  num_threads, prio_threads, s1, pile_threads, s2, rpc_name);
 } while(1);
END_STUB

 
static BEGIN_SERVICE(LRPC_CONSOLE_PING)
   res.version = req.version;
   res.tid = pm2_self();
END_SERVICE(LRPC_CONSOLE_PING)


PACK_REQ_STUB(LRPC_CONSOLE_PING)
   old_mad_pack_int(MAD_IN_HEADER, &arg->version, 1);
END_STUB

UNPACK_REQ_STUB(LRPC_CONSOLE_PING)
	old_mad_unpack_int(MAD_IN_HEADER, &arg->version, 1);
END_STUB

PACK_RES_STUB(LRPC_CONSOLE_PING)
	old_mad_pack_int(MAD_IN_HEADER, &arg->version, 1);
	old_mad_pack_int(MAD_IN_HEADER, &arg->tid, 1);
END_STUB

marcel_sem_t _ping_sem;
LRPC_REQ(LRPC_CONSOLE_PING) _ping_req = { 0 };

UNPACK_RES_STUB(LRPC_CONSOLE_PING)
   old_mad_unpack_int(MAD_IN_HEADER, &arg->version, 1);
   old_mad_unpack_int(MAD_IN_HEADER, &arg->tid, 1);
   if(arg->version == _ping_req.version)
      marcel_sem_V(&_ping_sem);
   else
      tfprintf(stderr, "\n<Reponse tardive recue pour le ping sur [%d]>\n", arg->tid);
END_STUB

static pm2_user_func user_function = NULL;

static BEGIN_SERVICE(LRPC_CONSOLE_USER)
   memcpy(res.tab, req.tab, sizeof(_user_args_tab));
   res.tid = pm2_self();
END_SERVICE(LRPC_CONSOLE_USER)

PACK_REQ_STUB(LRPC_CONSOLE_USER)
   old_mad_pack_byte(MAD_BY_COPY, arg->tab, sizeof(_user_args_tab));
END_STUB

UNPACK_REQ_STUB(LRPC_CONSOLE_USER)
   old_mad_unpack_byte(MAD_BY_COPY, arg->tab, sizeof(_user_args_tab));
END_STUB

PACK_RES_STUB(LRPC_CONSOLE_USER)
   int i = 0, argc = 0;
   char *argv[128];

	old_mad_pack_int(MAD_IN_HEADER, &arg->tid, 1);
	if(user_function) {
                while(arg->tab[i] != 0) {
                        argv[argc++] = &arg->tab[i];
			do {
                           i++;
                        } while(arg->tab[i] != 0);
                        i++;
		}
		(*user_function)(argc, argv);
	} else
		old_mad_pack_str(MAD_IN_HEADER, "<User function not defined>");
	old_mad_pack_str(MAD_IN_HEADER, "");
END_STUB

UNPACK_RES_STUB(LRPC_CONSOLE_USER)
   char str[255];
   int tid;

   old_mad_unpack_int(MAD_IN_HEADER, &tid, 1);
   old_mad_unpack_str(MAD_IN_HEADER, str);
   if(strcmp(str, "")) {
      tprintf("Task [%d]\n", tid);
      do {
	tprintf("\t%s\n",str);
	old_mad_unpack_str(MAD_IN_HEADER, str);
      } while (strcmp(str,""));
   }
END_STUB

static BEGIN_SERVICE(LRPC_CONSOLE_MIGRATE)
   pm2_freeze();
   pm2_migrate(req.thread, req.destination);    
END_SERVICE(LRPC_CONSOLE_MIGRATE)

PACK_REQ_STUB(LRPC_CONSOLE_MIGRATE)
	old_mad_pack_int(MAD_BY_COPY, &arg->destination, 1);
        old_mad_pack_str(MAD_IN_HEADER, arg->thread_id);
END_STUB

UNPACK_REQ_STUB(LRPC_CONSOLE_MIGRATE)
	char thread_id[16];

	old_mad_unpack_int(MAD_BY_COPY, &arg->destination, 1);
        old_mad_unpack_str(MAD_IN_HEADER, thread_id);
	arg->thread = (marcel_t) xtoi(thread_id);
END_STUB

PACK_RES_STUB(LRPC_CONSOLE_MIGRATE)
END_STUB

UNPACK_RES_STUB(LRPC_CONSOLE_MIGRATE)
END_STUB

static BEGIN_SERVICE(LRPC_CONSOLE_FREEZE)
   marcel_freeze(&req.thread, 1);
END_SERVICE(LRPC_CONSOLE_FREEZE)

PACK_REQ_STUB(LRPC_CONSOLE_FREEZE)
        old_mad_pack_str(MAD_IN_HEADER, arg->thread_id);
END_STUB

UNPACK_REQ_STUB(LRPC_CONSOLE_FREEZE)
	char thread_id[16];

        old_mad_unpack_str(MAD_IN_HEADER, thread_id);
	arg->thread = (marcel_t) xtoi(thread_id);
END_STUB

PACK_RES_STUB(LRPC_CONSOLE_FREEZE)
END_STUB

UNPACK_RES_STUB(LRPC_CONSOLE_FREEZE)
END_STUB

void pm2_console_init_rpc()
{
	DECLARE_LRPC(LRPC_CONSOLE_THREADS);
	DECLARE_LRPC(LRPC_CONSOLE_PING);
	DECLARE_LRPC(LRPC_CONSOLE_USER);
	DECLARE_LRPC(LRPC_CONSOLE_MIGRATE);
	DECLARE_LRPC(LRPC_CONSOLE_FREEZE);
}

void pm2_set_user_func(pm2_user_func f)
{
   user_function = f;
}
