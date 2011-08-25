
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

#include "tbx.h"
#include "pm2.h"
#include "sys/pm2_printf.h"
#include "sys/pm2_migr.h"
#include "sys/netserver.h"
#include "isomalloc.h"
#include "block_alloc.h"
#include "pm2_sync.h"
#include "pm2_common.h"

#include <fcntl.h>
#include <stdarg.h>
#include <unistd.h>

#include "pm2_timing.h"

#ifdef PROFILE
#include "pm2_profile.h"
#endif

static int PM2_COMPLETION;

static unsigned nb_startup_funcs = 0;
static pm2_startup_func_t startup_funcs[MAX_STARTUP_FUNCS];
static void *startup_args[MAX_STARTUP_FUNCS];

int __pm2_self, __pm2_conf_size;

marcel_key_t _pm2_lrpc_num_key,
  _pm2_mad_key,
  _pm2_block_key,
  _pm2_isomalloc_nego_key;

static block_descr_t _pm2_main_block_descr;

#define MAX_RAWRPC     50

pm2_rawrpc_func_t pm2_rawrpc_func[MAX_RAWRPC];

void pm2_push_startup_func(pm2_startup_func_t f, void *args)
{
  if(nb_startup_funcs < MAX_STARTUP_FUNCS)
    {
      startup_funcs[nb_startup_funcs] = f;
      startup_args[nb_startup_funcs++] = args;
    }
  else
    MARCEL_EXCEPTION_RAISE(MARCEL_CONSTRAINT_ERROR);
}

static void pm2_completion_service(void)
{
  pm2_completion_t *c_ptr;

  pm2_unpack_byte(SEND_CHEAPER, RECV_EXPRESS, (char *)&c_ptr, sizeof(c_ptr));

  if(c_ptr->handler)
    (*c_ptr->handler)(c_ptr->arg);

  pm2_rawrpc_waitdata();

  marcel_sem_V(&c_ptr->sem);
}

/* BEURK!!! This should *go away* ASAP!!! */
extern unsigned __pm2_rpc_init_called;
extern void pm2_rpc_init(void);
/* EOB */

void pm2_init_data(int *argc, char *argv[])
{
  if(!__pm2_rpc_init_called)
    pm2_rpc_init();
}

void pm2_init_set_rank(int *argc, char *argv[],
		       unsigned pm2_self,
		       unsigned pm2_config_size)
{
  __pm2_self = pm2_self;
  __pm2_conf_size = pm2_config_size;
}

void pm2_init_thread_related(int *argc, char *argv[])
{
  block_init(__pm2_self, __pm2_conf_size);

  marcel_key_create(&_pm2_lrpc_num_key, NULL);
  marcel_key_create(&_pm2_mad_key, NULL);
  marcel_key_create(&_pm2_block_key, NULL);
  marcel_key_create(&_pm2_isomalloc_nego_key, NULL);

  marcel_setspecific(_pm2_block_key, (any_t)(&_pm2_main_block_descr));
  block_init_list(&_pm2_main_block_descr);

  pm2_rawrpc_register(&PM2_COMPLETION, pm2_completion_service);

  pm2_thread_init();
  pm2_printf_init();
  pm2_migr_init();

  pm2_sync_init(__pm2_self, __pm2_conf_size);
}

void pm2_init_exec_startup_funcs(int *argc, char *argv[])
{
  while(nb_startup_funcs--)
    (*(startup_funcs[nb_startup_funcs]))(*argc, argv,
					 startup_args[nb_startup_funcs]);
}

void pm2_init_purge_cmdline(int *argc, char *argv[])
{}

#if 0
// removed by O. A. for common_exit support
static void pm2_wait_end(void)
{
  char mess[128];
  static tbx_bool_t already_called = tbx_false;

  if(!already_called) {

    LOG_IN();
    pm2_net_request_end();
    pm2_net_wait_end();
    pm2debug("pm2_wait_end netserver_wait_end completed\n");

    marcel_end();

    sprintf(mess, "[Threads : %ld created, %d imported (%ld cached)]\n",
	    marcel_createdthreads(),
	    pm2_migr_imported_threads(),
	    marcel_cachedthreads());

    fprintf(stderr, mess);
    already_called = tbx_true;

    LOG_OUT();
  }
}
#endif // 0

void pm2_exit(void)
{
  LOG_IN();
  // pm2_wait_end();
  common_exit(NULL);
  LOG_OUT();
}

void pm2_halt()
{
  LOG_IN();
  pm2_net_request_end();
  LOG_OUT();
}

void pm2_freeze(void)
{
  marcel_freeze_sched();
}

void pm2_unfreeze(void)
{
  marcel_unfreeze_sched();
}

void pm2_rawrpc_register(int *num, pm2_rawrpc_func_t func)
{
  static unsigned rawrpc_service_number = NETSERVER_RAW_RPC;

  *num = rawrpc_service_number++;
  pm2_rawrpc_func[*num - NETSERVER_RAW_RPC] = func;
}

void pm2_rawrpc_begin(int module, int num,
		      pm2_attr_t *pm2_attr)
{
#ifdef PM2DEBUG
  if(module == __pm2_self)
	  MARCEL_EXCEPTION_RAISE(MARCEL_NOT_IMPLEMENTED);
#endif // PM2DEBUG

  if(pm2_attr == NULL)
    pm2_attr = &pm2_attr_default;

  pm2_disable_migration();

  pm2_begin_packing(pm2_net_get_channel(pm2_attr->channel), module);
  pm2_pack_int(SEND_SAFER, RECV_EXPRESS, &num, 1);
}

void pm2_rawrpc_end(void)
{
  pm2_end_packing();
  pm2_enable_migration();
}

void pm2_completion_init(pm2_completion_t *c,
			 pm2_completion_handler_t handler,
			 void *arg)
{
  marcel_sem_init(&c->sem, 0);
  c->proc = pm2_self();
  c->c_ptr = c;
  c->handler = handler;
  c->arg = arg;
}

/* Added by Luc Bouge. Dec. 14, 2000 */

void pm2_completion_copy(pm2_completion_t *to, 
			 pm2_completion_t *from)
{
 *to = *from;
}

/* End of addition */

void pm2_pack_completion(mad_send_mode_t sm,
			 mad_receive_mode_t rm,
			 pm2_completion_t *c)
{
  pm2_pack_int(sm, rm, &c->proc, 1);
  pm2_pack_byte(sm, rm, (char *)&c->c_ptr, sizeof(c->c_ptr));
}

void pm2_unpack_completion(mad_send_mode_t sm,
			   mad_receive_mode_t rm,
			   pm2_completion_t *c)
{
  pm2_unpack_int(sm, rm, &c->proc, 1);
  pm2_unpack_byte(sm, rm, (char *)&c->c_ptr, sizeof(c->c_ptr));
}

void pm2_completion_wait(pm2_completion_t *c)
{
  marcel_sem_P(&c->sem);
}

void pm2_completion_signal(pm2_completion_t *c)
{
  if(c->proc == pm2_self())
    marcel_sem_V(&c->c_ptr->sem);
  else {
    pm2_rawrpc_begin(c->proc, PM2_COMPLETION, NULL);
    pm2_pack_byte(SEND_CHEAPER, RECV_EXPRESS, (char *)&c->c_ptr, sizeof(c->c_ptr));
    pm2_rawrpc_end();
  }
}

void pm2_completion_signal_begin(pm2_completion_t *c)
{
  pm2_rawrpc_begin(c->proc, PM2_COMPLETION, NULL);
  pm2_pack_byte(SEND_CHEAPER, RECV_EXPRESS, (char *)&c->c_ptr, sizeof(c->c_ptr));
}

void pm2_completion_signal_end(void)
{
  pm2_rawrpc_end();
}


/************** pm2_malloc **********************/

void *pm2_malloc(size_t size, isoaddr_attr_t *attr)
{
  switch(attr->status){
  case ISO_SHARED: MARCEL_EXCEPTION_RAISE(MARCEL_PROGRAM_ERROR);break;
  case ISO_PRIVATE:
  case ISO_DEFAULT:return block_alloc((block_descr_t *)marcel_getspecific(_pm2_block_key), size, attr); break;
  default: MARCEL_EXCEPTION_RAISE(MARCEL_NOT_IMPLEMENTED);
  }
  return (void *)NULL;
}

void pm2_empty()
{
  LOG_IN();
  LOG_OUT();
}
