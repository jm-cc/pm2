/*
                      PM2 HIGH-PERF/ISOMALLOC
           High Performance Parallel Multithreaded Machine
                           version 3.0

     Gabriel Antoniu, Olivier Aumage, Luc Bouge, Vincent Danjean,
       Christian Perez, Jean-Francois Mehaut, Raymond Namyst

            Laboratoire de l'Informatique du Parallelisme
                        UMR 5668 CNRS-INRIA
                 Ecole Normale Superieure de Lyon

                      External Contributors:
                 Yves Denneulin (LMC - Grenoble),
                 Benoit Planquelle (LIFL - Lille)

                    1998 All Rights Reserved


                             NOTICE

 Permission to use, copy, modify, and distribute this software and
 its documentation for any purpose and without fee is hereby granted
 provided that the above copyright notice appear in all copies and
 that both the copyright notice and this permission notice appear in
 supporting documentation.

 Neither the institutions (Ecole Normale Superieure de Lyon,
 Laboratoire de L'informatique du Parallelisme, Universite des
 Sciences et Technologies de Lille, Laboratoire d'Informatique
 Fondamentale de Lille), nor the Authors make any representations
 about the suitability of this software for any purpose. This
 software is provided ``as is'' without express or implied warranty.
*/

#include <pm2.h>
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

   mad_pack_int(MAD_IN_HEADER, &temp, 1);
   sprintf(buf, "%lx", (unsigned long)p);
   mad_pack_str(MAD_IN_HEADER, buf);
   tamp=p->number;
   mad_pack_long(MAD_IN_HEADER, &tamp, 1);
   mad_pack_int(MAD_IN_HEADER, &p->prio, 1);
   temp=p->state;
   mad_pack_int(MAD_IN_HEADER, &temp, 1);
   temp=64*1024;
   mad_pack_int(MAD_IN_HEADER, &temp, 1);
   mad_pack_str(MAD_IN_HEADER, pm2_lrpc_name(pm2_lrpc_runned_by(p)));
   temp = p->not_migratable;
   mad_pack_int(MAD_IN_HEADER, &temp, 1);
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
  mad_pack_int(MAD_IN_HEADER, &zero, 1);
END_STUB

UNPACK_RES_STUB(LRPC_CONSOLE_THREADS)
 char s1[5], s2[5], rpc_name[64], thread_id[16];
 long num_threads;
 int encore, prio_threads, state_threads, pile_threads, not_migratable;

  do {
      mad_unpack_int(MAD_IN_HEADER, &encore, 1);
      if(encore==0)
		break;
      mad_unpack_str(MAD_IN_HEADER, thread_id);
      mad_unpack_long(MAD_IN_HEADER, &num_threads, 1);
      mad_unpack_int(MAD_IN_HEADER, &prio_threads, 1);
      mad_unpack_int(MAD_IN_HEADER, &state_threads, 1);
      mad_unpack_int(MAD_IN_HEADER, &pile_threads, 1);
      mad_unpack_str(MAD_IN_HEADER, rpc_name);
      mad_unpack_int(MAD_IN_HEADER, &not_migratable, 1);
      if(state_threads == RUNNING)
		sprintf(s1, "run");
      else if(state_threads == BLOCKED)
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
   mad_pack_int(MAD_IN_HEADER, &arg->version, 1);
END_STUB

UNPACK_REQ_STUB(LRPC_CONSOLE_PING)
	mad_unpack_int(MAD_IN_HEADER, &arg->version, 1);
END_STUB

PACK_RES_STUB(LRPC_CONSOLE_PING)
	mad_pack_int(MAD_IN_HEADER, &arg->version, 1);
	mad_pack_int(MAD_IN_HEADER, &arg->tid, 1);
END_STUB

marcel_sem_t _ping_sem;
LRPC_REQ(LRPC_CONSOLE_PING) _ping_req = { 0 };

UNPACK_RES_STUB(LRPC_CONSOLE_PING)
   mad_unpack_int(MAD_IN_HEADER, &arg->version, 1);
   mad_unpack_int(MAD_IN_HEADER, &arg->tid, 1);
   if(arg->version == _ping_req.version)
      marcel_sem_V(&_ping_sem);
   else
      tfprintf(stderr, "\n<Reponse tardive recue pour le ping sur [%lx]>\n", arg->tid);
END_STUB

static pm2_user_func user_function = NULL;

static BEGIN_SERVICE(LRPC_CONSOLE_USER)
   memcpy(res.tab, req.tab, sizeof(_user_args_tab));
   res.tid = pm2_self();
END_SERVICE(LRPC_CONSOLE_USER)

PACK_REQ_STUB(LRPC_CONSOLE_USER)
   mad_pack_byte(MAD_BY_COPY, arg->tab, sizeof(_user_args_tab));
END_STUB

UNPACK_REQ_STUB(LRPC_CONSOLE_USER)
   mad_unpack_byte(MAD_BY_COPY, arg->tab, sizeof(_user_args_tab));
END_STUB

PACK_RES_STUB(LRPC_CONSOLE_USER)
   int i = 0, argc = 0;
   char *argv[128];

	mad_pack_int(MAD_IN_HEADER, &arg->tid, 1);
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
		mad_pack_str(MAD_IN_HEADER, "<User function not defined>");
	mad_pack_str(MAD_IN_HEADER, "");
END_STUB

UNPACK_RES_STUB(LRPC_CONSOLE_USER)
   char str[255];
   int tid;

   mad_unpack_int(MAD_IN_HEADER, &tid, 1);
   mad_unpack_str(MAD_IN_HEADER, str);
   if(strcmp(str, "")) {
      tprintf("Task [%lx]\n", tid);
      do {
	tprintf("\t%s\n",str);
	mad_unpack_str(MAD_IN_HEADER, str);
      } while (strcmp(str,""));
   }
END_STUB

static BEGIN_SERVICE(LRPC_CONSOLE_MIGRATE)
   pm2_freeze();
   pm2_migrate((marcel_t)req.thread, req.destination);    
END_SERVICE(LRPC_CONSOLE_MIGRATE)

PACK_REQ_STUB(LRPC_CONSOLE_MIGRATE)
	mad_pack_int(MAD_BY_COPY, &arg->destination, 1);
        mad_pack_str(MAD_IN_HEADER, arg->thread_id);
END_STUB

UNPACK_REQ_STUB(LRPC_CONSOLE_MIGRATE)
	char thread_id[16];

	mad_unpack_int(MAD_BY_COPY, &arg->destination, 1);
        mad_unpack_str(MAD_IN_HEADER, thread_id);
	arg->thread = xtoi(thread_id);
END_STUB

PACK_RES_STUB(LRPC_CONSOLE_MIGRATE)
END_STUB

UNPACK_RES_STUB(LRPC_CONSOLE_MIGRATE)
END_STUB

static BEGIN_SERVICE(LRPC_CONSOLE_FREEZE)
   marcel_freeze((marcel_t *)&req.thread, 1);
END_SERVICE(LRPC_CONSOLE_FREEZE)

PACK_REQ_STUB(LRPC_CONSOLE_FREEZE)
        mad_pack_str(MAD_IN_HEADER, arg->thread_id);
END_STUB

UNPACK_REQ_STUB(LRPC_CONSOLE_FREEZE)
	char thread_id[16];

        mad_unpack_str(MAD_IN_HEADER, thread_id);
	arg->thread = xtoi(thread_id);
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
