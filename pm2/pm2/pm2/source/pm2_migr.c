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
#include <sys/pm2_migr.h>

static unsigned PM2_MIGR;

static unsigned _pm2_imported_threads = 0;
static pm2_pre_migration_hook _pm2_pre_migr_func = NULL;
static pm2_post_migration_hook _pm2_post_migr_func = NULL;
static pm2_post_post_migration_hook _pm2_post_post_migr_func = NULL;

typedef struct {
  int nb;
  marcel_t *pids;
  boolean do_send;
} migration_arg;

#ifdef MIGRATE_IN_HEADER
#define MIGR_MODE  MAD_IN_HEADER
#else
#define MIGR_MODE  MAD_IN_PLACE
#endif

static void netserver_migration(void)
{
  int nb, i;
  pm2_migr_ctl mctl;

  mad_unpack_int(MAD_IN_HEADER, &nb, 1);

  {
    marcel_t pids[nb];
    any_t keys[nb];

    for(i=0; i<nb; i++) {

      mad_unpack_byte(MAD_IN_HEADER, (char *)&mctl, sizeof(mctl));
      pids[i] = mctl.task;

      slot_general_alloc(NULL, 0, NULL, mctl.stack_base);

      mad_unpack_byte(MIGR_MODE,
		      (char *)mctl.stack_base + mctl.depl,
		      mctl.blk);

      block_unpack_all();

      if(_pm2_post_migr_func != NULL)
	keys[i] = (*_pm2_post_migr_func)((marcel_t)mctl.task);
    }

    pm2_rawrpc_waitdata();

    for(i=0; i<nb; i++)
      marcel_end_hibernation(pids[i],
			     (post_migration_func_t)_pm2_post_post_migr_func,
			     keys[i]);
  }
  _pm2_imported_threads += nb;
}

static void migrate_func(marcel_t task, unsigned long depl, unsigned long blk, void *arg)
{ 
  migration_arg *args = (migration_arg *)arg;
  pm2_migr_ctl mctl;
  block_descr_t *block_descr_ptr;

  mctl.stack_base = marcel_stackbase(task);
  mctl.task = task;
  mctl.depl = depl;
  mctl.blk = blk;

  mad_pack_byte(MAD_IN_HEADER, (char *)&mctl, sizeof(mctl));

  mad_pack_byte(MIGR_MODE, (char *)mctl.stack_base + depl, blk);

  block_descr_ptr = (block_descr_t *)(*marcel_specificdatalocation(task, _pm2_block_key));
  block_pack_all(block_descr_ptr);

  if(_pm2_pre_migr_func != NULL)
    (*_pm2_pre_migr_func)(task);

  slot_slot_busy(mctl.stack_base);
  slot_list_busy(&block_descr_ptr->slot_descr);

  if(args->do_send) {
    pm2_rawrpc_end();

#ifndef MIGRATE_IN_HEADER
    {
      int i;

      for(i=0; i<args->nb; i++)
	if(args->pids[i] != marcel_self())
	  marcel_cancel(args->pids[i]);
    }
#endif
  }
}

void pm2_migrate_group(marcel_t *pids, int nb, int module)
{ 
  migration_arg arg;
  int i;
  boolean me_too = FALSE;

  if(module == __pm2_self) {
    for(i=0; i<nb; i++) {
      pids[i]->depl = 0;
      marcel_unfreeze(pids, nb);
    }
    pm2_unfreeze();
  } else {

    for(i=0; i<nb; i++) {
      if(pids[i]->not_migratable > ((pids[i] == marcel_self()) ? 1 : 0)
	 || !pids[i]->detached) {
	RAISE(CONSTRAINT_ERROR);
      }
    }

    marcel_freeze(pids, nb);
    pm2_unfreeze();

    arg.do_send = FALSE;
    arg.pids = pids;
    arg.nb = nb;

    pm2_rawrpc_begin(module, PM2_MIGR, NULL);

    mad_pack_int(MAD_IN_HEADER, &nb, 1);

    for(i=0; i<nb; i++) {
      if(pids[i] == marcel_self())
	me_too = TRUE;
      else
#ifdef MIGRATE_IN_HEADER
	marcel_begin_hibernation(pids[i], migrate_func, &arg, FALSE);
#else
      /* 
       * On met le flag "fork" à VRAI parce qu'il faut maintenir ces threads
       * en vie tant que le message n'est pas envoyé (IN_PLACE).
       */
      marcel_begin_hibernation(pids[i], migrate_func, &arg, TRUE);
#endif
    }

    if(me_too) {
      arg.do_send = TRUE;
      marcel_begin_hibernation(marcel_self(), migrate_func, &arg, FALSE);
      pm2_enable_migration();
    } else {
      pm2_rawrpc_end();

#ifndef MIGRATE_IN_HEADER
      for(i=0; i<nb; i++)
	marcel_cancel(pids[i]);
#endif
    }
  }
}

void pm2_migrate(marcel_t pid, int module)
{
  pm2_migrate_group(&pid, 1, module);
}

void pm2_migr_init(void)
{
  pm2_rawrpc_register(&PM2_MIGR, netserver_migration);
}

void pm2_threads_list(int max, marcel_t *pids, int *nb, int which)
{ int mask = which | DETACHED_ONLY;

   if(which == MIGRATABLE_ONLY)
      mask |= NOT_BLOCKED_ONLY;

   marcel_threadslist(max, pids, nb, mask);
}

void pm2_set_pre_migration_func(pm2_pre_migration_hook f)
{
   _pm2_pre_migr_func = f;
}

void pm2_set_post_migration_func(pm2_post_migration_hook f)
{
   _pm2_post_migr_func = f;
}

void pm2_set_post_post_migration_func(pm2_post_post_migration_hook f)
{
   _pm2_post_post_migr_func = f;
}

unsigned pm2_migr_imported_threads(void)
{
  return _pm2_imported_threads;
}
