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

______________________________________________________________________________
$Log: pm2_thread.c,v $
Revision 1.4  2000/07/14 16:17:15  gantoniu
Merged with branch dsm3

Revision 1.3.10.1  2000/06/13 16:44:14  gantoniu
New dsm branch.

Revision 1.3.8.1  2000/06/07 09:19:40  gantoniu
Merging new dsm with current PM2 : first try.

Revision 1.3  2000/02/28 11:17:18  rnamyst
Changed #include <> into #include "".

Revision 1.2  2000/01/31 15:58:33  oaumage
- ajout du Log CVS


______________________________________________________________________________
*/

/* #define DEBUG */

#include "pm2.h"
#include "madeleine.h"

#define MAX_PARAMS   128

#define MAX_THREADS  512

extern marcel_key_t mad2_send_key, mad2_recv_key;

struct pm2_thread_arg {
  marcel_attr_t attr;
  pm2_func_t func;
  void *arg;
  struct pm2_thread_arg *next;
#ifdef MAD2
  void *mad2_specific;
#endif
  void *mad_sem;
} thread_param[MAX_PARAMS];

static struct pm2_thread_arg *next_free = NULL;
static unsigned next_unalloc = 0;

static unsigned the_granted_size;

static __inline__ void *pm2_thread_stack_alloc(void)
{
  return slot_general_alloc(NULL, 0, &the_granted_size, NULL, NULL);
}

static __inline__ void pm2_thread_stack_free(void *stack)
{
  slot_free(NULL, stack);
}

static __inline__ struct pm2_thread_arg *pm2_thread_alloc(void)
{
  struct pm2_thread_arg *res;

  lock_task();
  if(next_free != NULL) {
    res = next_free;
    next_free = next_free->next;
    unlock_task();

    mdebug("Params allocated in cache\n");

    marcel_attr_setstackaddr(&res->attr, pm2_thread_stack_alloc());
  } else {
    if(next_unalloc == MAX_PARAMS)
      RAISE(CONSTRAINT_ERROR);
    res = &thread_param[next_unalloc];
    next_unalloc++;
    unlock_task();

    marcel_attr_init(&res->attr);
    marcel_attr_setdetachstate(&res->attr, TRUE);
    marcel_attr_setuserspace(&res->attr, sizeof(block_descr_t));
    marcel_attr_setactivation(&res->attr, TRUE);
    marcel_attr_setstackaddr(&res->attr, pm2_thread_stack_alloc());
    marcel_attr_setstacksize(&res->attr, the_granted_size);
  }
  return res;
}

static __inline__ void pm2_thread_free(struct pm2_thread_arg *ta)
{
  lock_task();
  ta->next = next_free;
  next_free = ta;
  unlock_task();
}

static void pm2_thread_term_func(void *arg)
{
  block_flush_list((block_descr_t *)(*marcel_specificdatalocation((marcel_t)arg,
								  _pm2_block_key)));

  pm2_thread_stack_free(marcel_stackbase((marcel_t)arg));
}

static any_t pm2_thread_starter(any_t arg)
{
  struct pm2_thread_arg *ta = (struct pm2_thread_arg *)arg;
  pm2_func_t _func;
  void *_arg;
  block_descr_t *bdesc;

  _func = ta->func;
  _arg = ta->arg;

#ifdef MAD2
  marcel_setspecific(mad2_recv_key, ta->mad2_specific);
#endif
  marcel_setspecific(_pm2_mad_key, ta->mad_sem);

  pm2_thread_free(ta);

  marcel_getuserspace(marcel_self(), (void **)&bdesc);
  block_init_list(bdesc);
  marcel_setspecific(_pm2_block_key, (any_t)bdesc);

  marcel_cleanup_push(pm2_thread_term_func, marcel_self());

  (*_func)(_arg);

  return NULL;
}

marcel_t pm2_thread_create(pm2_func_t func, void *arg)
{
  struct pm2_thread_arg *ta;
  marcel_t pid;

  ta = pm2_thread_alloc();
  ta->func = func;
  ta->arg = arg;
#ifdef MAD2
  ta->mad2_specific = marcel_getspecific(mad2_recv_key);
#endif
  ta->mad_sem = marcel_getspecific(_pm2_mad_key);

  marcel_create(&pid, &ta->attr, (marcel_func_t)pm2_thread_starter, ta);

  return pid;
}

void pm2_thread_init(void)
{
}

void pm2_thread_exit(void)
{
}
