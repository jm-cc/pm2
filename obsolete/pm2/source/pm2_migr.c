
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

#include "pm2.h"
#include "sys/pm2_migr.h"

static int PM2_MIGR;

static unsigned _pm2_imported_threads = 0;
static pm2_pre_migration_hook _pm2_pre_migr_func = NULL;
static pm2_post_migration_hook _pm2_post_migr_func = NULL;
static pm2_post_post_migration_hook _pm2_post_post_migr_func = NULL;

typedef struct {
  int dest;
  int nb;
  marcel_t *pids;
  tbx_bool_t do_send;
} migration_arg;

static void netserver_migration(void)
{
  int nb, i;
  pm2_migr_ctl mctl;

  pm2_unpack_int(SEND_SAFER, RECV_EXPRESS, &nb, 1);

  {
    marcel_t pids[nb];
    any_t keys[nb];

    for(i=0; i<nb; i++) {

      pm2_unpack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&mctl, sizeof(mctl));
      pids[i] = mctl.task;

      slot_general_alloc(NULL, 0, NULL, mctl.stack_base, NULL);

      pm2_unpack_byte(SEND_CHEAPER, RECV_CHEAPER,
		      (char *)mctl.stack_base + mctl.depl,
		      mctl.blk);
      isoaddr_page_set_owner(isoaddr_page_index(mctl.stack_base), pm2_self());
      isoaddr_page_set_status(isoaddr_page_index(mctl.stack_base), ISO_PRIVATE);
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

  pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, (char *)&mctl, sizeof(mctl));

  pm2_pack_byte(SEND_CHEAPER, RECV_CHEAPER, (char *)mctl.stack_base + depl, blk);

  block_descr_ptr = (block_descr_t *)(*marcel_specificdatalocation(task, _pm2_block_key));
  block_pack_all(block_descr_ptr, args->dest);
  isoaddr_page_set_owner(isoaddr_page_index(mctl.stack_base), args->dest);


  if(_pm2_pre_migr_func != NULL)
    (*_pm2_pre_migr_func)(task);

  slot_slot_busy(mctl.stack_base);
  slot_list_busy(&block_descr_ptr->slot_descr);

  if(args->do_send) {
    int i;

    pm2_rawrpc_end();

    for(i=0; i<args->nb; i++)
      if(args->pids[i] != marcel_self())
	marcel_cancel(args->pids[i]);
  }
}

void pm2_migrate_group(marcel_t *pids, int nb, int module)
{ 
  migration_arg arg;
  int i;
  tbx_bool_t me_too = tbx_false;

  if(module == __pm2_self) {
    marcel_unfreeze(pids, nb);
    pm2_unfreeze();
  } else {

    for(i=0; i<nb; i++) {
      if(pids[i]->not_migratable > ((pids[i] == marcel_self()) ? 1 : 0)
	 || !pids[i]->detached) {
	MARCEL_EXCEPTION_RAISE(MARCEL_CONSTRAINT_ERROR);
      }
    }

    marcel_freeze(pids, nb);
    pm2_unfreeze();

    arg.do_send = tbx_false;
    arg.pids = pids;
    arg.nb = nb;
    arg.dest = module;

    pm2_rawrpc_begin(module, PM2_MIGR, NULL);

    pm2_pack_int(SEND_SAFER, RECV_EXPRESS, &nb, 1);

    for(i=0; i<nb; i++) {
      if(pids[i] == marcel_self())
	me_too = tbx_true;
      else
      /* 
       * On met le flag "fork" à VRAI parce qu'il faut maintenir ces threads
       * en vie tant que le message n'est pas envoyé (IN_PLACE).
       */
      marcel_begin_hibernation(pids[i], migrate_func, &arg, tbx_true);
    }

    if(me_too) {
      arg.do_send = tbx_true;
      marcel_begin_hibernation(marcel_self(), migrate_func, &arg, tbx_false);
      pm2_enable_migration();
    } else {
      pm2_rawrpc_end();

      for(i=0; i<nb; i++)
	marcel_cancel(pids[i]);
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
      mask |= READY_ONLY;

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
