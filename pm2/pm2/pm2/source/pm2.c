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
$Log: pm2.c,v $
Revision 1.14  2000/05/24 15:35:57  rnamyst
Enhanced Marcel polling + various directories cleaning

Revision 1.13  2000/02/28 11:17:07  rnamyst
Changed #include <> into #include "".

Revision 1.12  2000/02/14 10:04:54  rnamyst
Modified to propose a new interface to "pm2_completion" facilities...

Revision 1.11  2000/01/31 15:58:26  oaumage
- ajout du Log CVS


______________________________________________________________________________
*/

#include "pm2.h"
#include "sys/pm2_printf.h"
#include "sys/pm2_migr.h"
#include "sys/netserver.h"
#include "isomalloc.h"
#include "block_alloc.h"

#include <fcntl.h>
#include <stdarg.h>
#include <unistd.h>

#include "pm2_timing.h"

#ifdef DSM
#include "dsm_pm2.h"
#endif

#undef min
#define min(a, b)	((a) < (b) ? (a) : (b))
#undef max
#define max(a, b)	((a) > (b) ? (a) : (b))

static unsigned PM2_COMPLETION;

static pm2_startup_func_t startup_func = NULL;

static int spmd_conf[MAX_MODULES];

unsigned __pm2_self, __conf_size;

marcel_key_t _pm2_lrpc_num_key,
  _pm2_mad_key,
  _pm2_block_key,
  _pm2_isomalloc_nego_key;

static block_descr_t _pm2_main_block_descr;

#define MAX_RAWRPC     50

pm2_rawrpc_func_t pm2_rawrpc_func[MAX_RAWRPC];

#define MAX_CHANNELS    16

#ifdef MAD2
static p_mad_channel_t pm2_channel[MAX_CHANNELS];

static __inline__ p_mad_channel_t channel(pm2_channel_t c)
{
  return pm2_channel[c];
}
#endif

static unsigned nb_of_channels = 1;

void pm2_channel_alloc(pm2_channel_t *channel)
{
  *channel = nb_of_channels++;
}

int pm2_main_module(void)
{
  return spmd_conf[0];
}

void pm2_set_startup_func(pm2_startup_func_t f)
{
  startup_func = f;
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

/* BEURK!!! This should *go away* very soon!!! */
extern unsigned __pm2_rpc_init_called;
extern void pm2_rpc_init(void);
/* EOB */

void pm2_init(int *argc, char **argv)
{
  if(!__pm2_rpc_init_called)
    pm2_rpc_init();

  timing_init();

#ifdef USE_SAFE_MALLOC
  safe_malloc_init();
#endif   

  marcel_init(argc, argv);

  mad_init(argc, argv, 0, spmd_conf, &__conf_size, &__pm2_self);

#if MAD2
  {
    int i;

    for(i=0; i<nb_of_channels; i++) {
      pm2_channel[i] = mad_open_channel(mad_get_madeleine(), 0);
      mdebug("Channel %d created.\n", i);
    }
  }
#endif

  marcel_strip_cmdline(argc, argv);

  mad_buffers_init();

  block_init(__pm2_self, __conf_size);

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

#ifdef DSM
  dsm_pm2_init(__pm2_self, __conf_size);
#endif

  if(startup_func != NULL)
    (*startup_func)();

#ifdef MAD2
  {
    int i;

    for(i=0; i<nb_of_channels; i++)
      netserver_start(channel(i), MAX_PRIO);
  }
#else
  netserver_start(MAX_PRIO);
#endif
}

static void pm2_wait_end(void)
{
  char mess[128];
  static boolean already_called = FALSE;

  if(!already_called) {

    netserver_wait_end();

    marcel_end();

    sprintf(mess, "[Threads : %ld created, %d imported (%ld cached)]\n",
	    marcel_createdthreads(),
	    pm2_migr_imported_threads(),
	    marcel_cachedthreads());

    fprintf(stderr, mess);

    already_called = TRUE;
  }
}

void pm2_exit(void)
{
  pm2_wait_end();

  mdebug("pm2_wait_end completed\n");

  pm2_thread_exit();

  mdebug("pm2_thread_exit completed\n");

  mad_exit();

  mdebug("mad_exit completed\n");

  block_exit();

  mdebug("block_exit completed\n");

#ifdef DSM
  dsm_pm2_exit();

  mdebug("dsm_pm2_exit completed\n");
#endif
}

void pm2_halt()
{
  int i;
  unsigned tag = NETSERVER_END;

  if (!mad_can_send_to_self())
    {
      netserver_stop();
    }
  
  for(i=0; i<__conf_size; i++) {
    if (!(i == __pm2_self && !mad_can_send_to_self())) {
#ifdef MAD2
      int c;

      for(c=0; c<nb_of_channels; c++) {
	mad_sendbuf_init(channel(c), i);
	pm2_pack_int(SEND_SAFER, RECV_EXPRESS, &tag, 1);
	mad_sendbuf_send();
      }
#else
      mad_sendbuf_init(i);
      pm2_pack_int(SEND_SAFER, RECV_EXPRESS, &tag, 1);
      mad_sendbuf_send();
#endif
    }
  }
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
#ifdef DEBUG
  if(module == __pm2_self && !mad_can_send_to_self())
    RAISE(NOT_IMPLEMENTED);
#endif

  if(pm2_attr == NULL)
    pm2_attr = &pm2_attr_default;

  pm2_disable_migration();

#ifdef MAD2
  mad_sendbuf_init(channel(pm2_attr->channel), module);
#else
  mad_sendbuf_init(module);
#endif
  pm2_pack_int(SEND_SAFER, RECV_EXPRESS, &num, 1);
}

void pm2_rawrpc_end(void)
{
  mad_sendbuf_send();
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

