
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
#include "madeleine.h"
#include "sys/netserver.h"
#include "isomalloc.h"
#include "pm2_timing.h"


/*
 * Constants
 * ---------
 */
#ifdef MAD3
static const char *pm2_net_pm2_channel_name = "pm2";
#endif // MAD3


/*
 * Static variables
 * ----------------
 */
static marcel_t            *pm2_net_server_pid_array = NULL;
static unsigned int         pm2_net_server_nb        =    1;
static p_mad_channel_t     *pm2_net_channel_array    = NULL;
static volatile tbx_bool_t  pm2_net_finished         = tbx_false;
static marcel_mutex_t       pm2_net_halt_lock        = MARCEL_MUTEX_INITIALIZER;
static int                  pm2_net_zero_halt        =    0;

#ifdef MAD3
static p_tbx_slist_t        pm2_net_channel_slist  = NULL;
static p_tbx_htable_t       pm2_net_channel_htable = NULL;
#endif // MAD3


/*
 * Extern variables
 * ----------------
 */
extern pm2_rawrpc_func_t pm2_rawrpc_func[];

/*
 * Static functions
 * ----------------
 */
static
int
pm2_net_single_mode(void)
{
#ifdef FORCE_NET_THREADS
  return 0;
#else // FORCE_NET_THREADS
  return pm2_config_size() == 1;
#endif // FORCE_NET_THREADS
}


static
void
pm2_net_server_raw_rpc(int num)
{
  marcel_sem_t sem;

  LOG_IN();
  marcel_sem_init(&sem, 0);
  marcel_setspecific(_pm2_mad_key, &sem);

  (*pm2_rawrpc_func[num])();

  marcel_sem_P(&sem);
  LOG_OUT();
}

static
void
pm2_net_server_term_func(void *arg)
{
  LOG_IN();
  LOG("netserver is ending.");
  slot_free(NULL, marcel_stackbase((marcel_t)arg));
  LOG_OUT();
}

static
void
pm2_net_send_server_end_request(void)
{
  int tag  = NETSERVER_END;
  int c    = 0;
  int node = 0;

  LOG_IN();
  for (c = 0; c < pm2_net_server_nb; c++)
    {
      p_mad_channel_t channel = NULL;

      channel = pm2_net_channel_array[c];

      for (node = 1; node < __pm2_conf_size; node++)
	{
	  pm2_begin_packing(channel, node);
	  pm2_pack_int(SEND_SAFER, RECV_EXPRESS, &tag, 1);
	  pm2_end_packing();
	}
    }
  LOG_OUT();
}

static
void
pm2_net_halt_request(void)
{
  LOG_IN();
  if (__pm2_self)
    {
      DISP("NETSERVER ERROR: NETSERVER_REQUEST_HALT tag on node %i",
	   __pm2_self);
      goto end;
    }

  marcel_mutex_lock(&pm2_net_halt_lock);
  pm2_net_zero_halt++;

  if (pm2_net_zero_halt == (__pm2_conf_size + 1) * pm2_net_server_nb)
    {
      /* +1 car tous les pm2_exit() en envoient un + pm2_halt() */
      pm2_thread_create((pm2_func_t)pm2_net_send_server_end_request, NULL);
    }

  marcel_mutex_unlock(&pm2_net_halt_lock);

 end:
  LOG_OUT();
}

static
any_t
pm2_net_server(any_t arg)
{
  int tag;

  marcel_postexit(pm2_net_server_term_func, marcel_self());

  while (!pm2_net_finished)
    {
      pm2_begin_unpacking((p_mad_channel_t)arg);

      pm2_unpack_int(SEND_SAFER, RECV_EXPRESS, &tag, 1);

      if (tag >= NETSERVER_RAW_RPC)
	{
	  pm2_net_server_raw_rpc(tag - NETSERVER_RAW_RPC);
	}
      else
	{
	  switch (tag)
	    {

	    case NETSERVER_REQUEST_HALT:
	      {
		pm2_end_unpacking();
		pm2_net_halt_request();
	      }
	      break;

	    case NETSERVER_END:
	      {
		pm2_end_unpacking();

		if (__pm2_self == 1)
		  {
		    /* On arrête le noeud 0... */
		    pm2_begin_packing(arg, 0);
		    pm2_pack_int(SEND_SAFER, RECV_EXPRESS, &tag, 1);
		    pm2_end_packing();
		  }

		pm2_net_finished = tbx_true;
	      }
	      break;

	    default:
	      {
		pm2_end_unpacking();
		DISP("NETSERVER ERROR: %d is not a valid tag.", tag);
	      }
	    }
	}
    }

  return NULL;
}

static
marcel_t
pm2_net_server_start(p_mad_channel_t channel)
{
  marcel_t      pid = NULL;
  marcel_attr_t attr;

  LOG_IN();
  marcel_attr_init(&attr);

#ifdef ONE_VP_PER_NET_THREAD
  {
    unsigned vp = marcel_sched_add_vp();
    extern marcel_vpset_t __pm2_global_vpset; // declared in pm2_thread.c

    marcel_vpset_clr(&__pm2_global_vpset, vp);

    marcel_apply_vpset(&__pm2_global_vpset);

    marcel_attr_setvpset(&attr, MARCEL_VPSET_VP(vp));

    pm2debug("Extra vp (%d) allocated for netserver thread\n", vp);
  }
#endif // ONE_VP_PER_NET_THREAD

#ifdef REALTIME_NET_THREADS
  marcel_attr_setrealtime(&attr, MARCEL_CLASS_REALTIME);
#endif // REALTIME_NET_THREADS

  {
    size_t  granted = 0;
    void         *slot    = NULL;

    slot = slot_general_alloc(NULL, 0, &granted, NULL, NULL);
    marcel_attr_setstackaddr(&attr, slot);
    marcel_attr_setstacksize(&attr, granted);
  }

  marcel_create(&pid, &attr, pm2_net_server, (any_t)channel);
  LOG_OUT();

  return pid;
}

static
void
pm2_net_halt_or_exit_request(void)
{
  int c = 0;

  LOG_IN();
  for (c = 0; c < pm2_net_server_nb; c++)
    {
      if (!__pm2_self)
	{
	  pm2_net_halt_request();
	}
      else
	{
	  int             tag     = NETSERVER_REQUEST_HALT;
	  p_mad_channel_t channel = NULL;

	  channel = pm2_net_channel_array[c];

	  pm2_begin_packing(channel, 0);
	  pm2_pack_int(SEND_SAFER, RECV_EXPRESS, &tag, 1);
	  pm2_end_packing();
	}
    }
  LOG_OUT();
}


/*
 * Functions
 * ---------
 */
void
pm2_net_init_channels(int   *argc,
		      char **argv)
{
  LOG_IN();
  if (pm2_net_single_mode())
    goto end;

  pm2_net_channel_array =
    TBX_CALLOC(pm2_net_server_nb, sizeof(p_mad_channel_t));

#ifdef MAD3
  {
    char *name = NULL;

    if (!pm2_net_channel_slist)
      {
	pm2_net_channel_slist  = tbx_slist_nil();
	pm2_net_channel_htable = tbx_htable_empty_table();
      }

    name = tbx_strdup(pm2_net_pm2_channel_name);
    tbx_slist_enqueue(pm2_net_channel_slist, name);
    tbx_htable_add(pm2_net_channel_htable, name, name);
  }

  {
    p_mad_madeleine_t madeleine = NULL;
    p_tbx_slist_t     slist     = NULL;
    p_tbx_htable_t    htable    = NULL;
    int               i         =    0;

    madeleine = mad_get_madeleine();
    slist     = pm2_net_channel_slist;
    htable    = pm2_net_channel_htable;

    TBX_ASSERT(pm2_net_server_nb == tbx_slist_get_length(slist));

    for (i = 0; i < pm2_net_server_nb; i++)
      {
	p_mad_channel_t  channel = NULL;
	char            *name    = NULL;

	name                     = tbx_slist_extract(slist);
	LOG("channel %d = %s\n", i, name);
	channel                  = mad_get_channel(madeleine, name);
	pm2_net_channel_array[i] = channel;
	tbx_htable_replace(htable, name, channel);
	tbx_slist_append(slist, channel);
      }
  }
#endif // MAD3

 end:
  LOG_OUT();
}

void
pm2_net_servers_start(int   *argc,
		      char **argv)
{
  LOG_IN();
  if (!pm2_net_single_mode())
    {
      int i;

      pm2_net_server_pid_array =
	TBX_CALLOC(pm2_net_server_nb, sizeof(marcel_t));

      for (i = 0; i < pm2_net_server_nb; i++)
	{
	  p_mad_channel_t channel = NULL;
	  marcel_t        pid     = NULL;

	  channel                     = pm2_net_channel_array[i];
	  pid                         = pm2_net_server_start(channel);
	  pm2_net_server_pid_array[i] = pid;
	}
    }
  LOG_OUT();
}

void
pm2_net_request_end(void)
{
  LOG_IN();
  if (!pm2_net_single_mode())
    {
      pm2_net_halt_or_exit_request();
    }
  LOG_OUT();
}

void
pm2_net_wait_end(void)
{
  LOG_IN();
  if (!pm2_net_single_mode())
    {
      int i = 0;

      for (i = 0; i < pm2_net_server_nb; i++)
	{
	  marcel_t pid = NULL;

	  pid = pm2_net_server_pid_array[i];
	  marcel_join(pid, NULL);
	  pm2_net_server_pid_array[i] = NULL;
	}
    }
  tbx_slist_clear_and_free(pm2_net_channel_slist);
  pm2_net_channel_slist = NULL;
  tbx_htable_cleanup_and_free(pm2_net_channel_htable);
  pm2_net_channel_htable = NULL;
  LOG_OUT();
}

p_mad_channel_t
pm2_net_get_channel(pm2_channel_t c)
{
  p_mad_channel_t channel = NULL;

  LOG_IN();
  channel = pm2_net_channel_array[c];
  LOG_OUT();

  return channel;
}

/*
 * Interface
 * ---------
 */
void
pm2_channel_alloc(pm2_channel_t *channel,
		  char          *name)
{
  LOG_IN();
#ifdef MAD3
  {
    if (!pm2_net_channel_slist)
      {
	pm2_net_channel_slist  = tbx_slist_nil();
	pm2_net_channel_htable = tbx_htable_empty_table();
      }
  else
    {
      if (tbx_htable_get(pm2_net_channel_htable, name))
	TBX_FAILURE("duplicate channel allocation");
    }

    name = tbx_strdup(name);
    tbx_slist_append(pm2_net_channel_slist, name);
    tbx_htable_add(pm2_net_channel_htable, name, name);
  }
#endif // MAD3

  *channel = pm2_net_server_nb++;
  LOG_OUT();
}
