/*
 * NewMadeleine
 * Copyright (C) 2006 (see AUTHORS file)
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

/*
 * Mad_nmad.c
 * =========
 */

#undef MAD_NMAD_SO_DEBUG

#include <stdint.h>
#include <sys/uio.h>
#include <assert.h>

#include <nm_public.h>
#if defined CONFIG_SCHED_MINI_ALT
#  include <nm_mini_alt_public.h>
#elif defined CONFIG_SCHED_OPT
#  include <nm_so_public.h>
#else
#  error mad_nmad.c requires the mini alt scheduler for now
#endif

#if defined CONFIG_MX
#  include <nm_mx_public.h>
#endif

#if defined CONFIG_GM
#  include <nm_gm_public.h>
#endif

#if defined CONFIG_QSNET
#  include <elan/elan.h>
#  include <elan/capability.h>
#  include <qsnet/fence.h>
#  include <nm_qsnet_public.h>
#  define MAD_QUADRICS_CONTEXT_ID_OFFSET	2
#endif

#if defined CONFIG_SISCI
#  include <nm_sisci_public.h>
#endif

#if defined CONFIG_IBVERBS
#  include <nm_ibverbs_public.h>
#endif

#if defined CONFIG_TCP
#  include <nm_tcpdg_public.h>
#endif

#include "madeleine.h"

#if defined CONFIG_SCHED_MINI_ALT
#  include <nm_basic_public.h>
#elif defined CONFIG_SCHED_OPT
#  include <nm_so_sendrecv_interface.h>
#endif

#include "nm_mad3_private.h"
#include "nm_predictions.h"

#include <errno.h>


#define MAD_NMAD_GROUP_MALLOC_THRESHOLD 256

/*
 * local structures
 * ----------------
 */
#ifdef CONFIG_SCHED_OPT
#  ifdef MAD_NMAD_SO_DEBUG
typedef struct s_mad_nmad_deferred {
        void			*ptr;
        size_t			 len;
        uint8_t			 seq;
        uint8_t			 tag_id;
} mad_nmad_deferred_t, *p_mad_nmad_deferred_t;
#  endif /* MAD_NMAD_SO_DEBUG */
#endif /* CONFIG_SCHED_OPT */

typedef struct s_mad_nmad_driver_specific {
        uint8_t			 drv_id;
        char			*l_url;
} mad_nmad_driver_specific_t, *p_mad_nmad_driver_specific_t;

typedef struct s_mad_nmad_adapter_specific {
        int 			 master_channel_id;
        p_tbx_darray_t		 cnx_darray;
} mad_nmad_adapter_specific_t, *p_mad_nmad_adapter_specific_t;

typedef struct s_mad_nmad_channel_specific {
        uint8_t			 tag_id;
} mad_nmad_channel_specific_t, *p_mad_nmad_channel_specific_t;

typedef struct s_mad_nmad_link_specific {
        int dummy;
} mad_nmad_link_specific_t, *p_mad_nmad_link_specific_t;


/*
 * forward declarations of driver functions
 * ----------------------------------------
 */
static void
mad_nmad_driver_init(p_mad_driver_t, int *, char ***);

static void
mad_nmad_driver_exit(p_mad_driver_t);

static void
mad_nmad_adapter_init(p_mad_adapter_t);

static void
mad_nmad_adapter_exit(p_mad_adapter_t);

static void
mad_nmad_channel_init(p_mad_channel_t);

static void
mad_nmad_before_open_channel(p_mad_channel_t);

static void
mad_nmad_connection_init(p_mad_connection_t,
                         p_mad_connection_t);

static void
mad_nmad_link_init(p_mad_link_t);

static void
mad_nmad_accept(p_mad_connection_t,
                p_mad_adapter_info_t);

static void
mad_nmad_connect(p_mad_connection_t,
                 p_mad_adapter_info_t);

static void
mad_nmad_after_open_channel(p_mad_channel_t);

static void
mad_nmad_disconnect(p_mad_connection_t);

#if defined CONFIG_SCHED_MINI_ALT
#  ifdef MAD_MESSAGE_POLLING
static p_mad_connection_t
mad_nmad_poll_message(p_mad_channel_t);
#  endif /* MAD_MESSAGE_POLLING */

static void
mad_nmad_new_message(p_mad_connection_t);

static p_mad_connection_t
mad_nmad_receive_message(p_mad_channel_t);

static void
mad_nmad_send_buffer(p_mad_link_t,
                     p_mad_buffer_t);

static void
mad_nmad_receive_buffer(p_mad_link_t,
                        p_mad_buffer_t *);

static void
mad_nmad_send_buffer_group(p_mad_link_t,
                           p_mad_buffer_group_t);

static void
mad_nmad_receive_sub_buffer_group(p_mad_link_t,
                                  tbx_bool_t,
                                  p_mad_buffer_group_t);
#endif

/* static vars
 */

/* core and proto objects */
static struct nm_core	*p_core		= NULL;
#if defined CONFIG_SCHED_MINI_ALT
static struct nm_proto	*p_proto	= NULL;
#elif defined CONFIG_SCHED_OPT
static struct nm_so_interface *p_so_if	= NULL;
#endif

/*
 * Driver private functions
 * ------------------------
 */


struct nm_core	*
mad_nmad_get_core(void) {
  return p_core;
}

#if defined CONFIG_SCHED_OPT
struct nm_so_interface	*
mad_nmad_get_sr_interface(void) {
  return p_so_if;
}
#endif

static
void
nm_mad3_init_core(int	 *argc,
                  char	**argv) {
        int err;

        TRACE("Initializing NMAD driver");
#if defined CONFIG_SCHED_MINI_ALT
        err = nm_core_init(argc, argv, &p_core, nm_mini_alt_load);
        if (err != NM_ESUCCESS) {
                DISP("nm_core_init returned err = %d\n", err);
                TBX_FAILURE("nmad error");
        }
        err = nm_core_proto_init(p_core, nm_basic_load, &p_proto);
        if (err != NM_ESUCCESS) {
                DISP("nm_core_proto_init returned err = %d\n", err);
                TBX_FAILURE("nmad error");
        }
#elif defined CONFIG_SCHED_OPT
        err = nm_core_init(argc, argv, &p_core, nm_so_load);
        if (err != NM_ESUCCESS) {
                DISP("nm_core_init returned err = %d\n", err);
                TBX_FAILURE("nmad error");
        }
	err = nm_so_sr_init(p_core, &p_so_if);
	if(err != NM_ESUCCESS) {
                DISP("nm_so_sr_init return err = %d\n", err);
                TBX_FAILURE("nmad error");
	}
        err = nm_ns_init(p_core);
	if(err != NM_ESUCCESS) {
                DISP("nm_ns_init return err = %d\n", err);
                TBX_FAILURE("nmad error");
	}

#endif
}

/*
 * Registration function
 * ---------------------
 */

char *
mad_nmad_register(p_mad_driver_interface_t interface) {
        NM_LOG_IN();
        TRACE("Registering NMAD driver");

        interface->driver_init                = mad_nmad_driver_init;
        interface->adapter_init               = mad_nmad_adapter_init;
        interface->channel_init               = mad_nmad_channel_init;
        interface->before_open_channel        = mad_nmad_before_open_channel;
        interface->connection_init            = mad_nmad_connection_init;
        interface->link_init                  = mad_nmad_link_init;
        interface->accept                     = mad_nmad_accept;
        interface->connect                    = mad_nmad_connect;
        interface->after_open_channel         = mad_nmad_after_open_channel;
        interface->before_close_channel       = NULL;
        interface->disconnect                 = mad_nmad_disconnect;
        interface->after_close_channel        = NULL;
        interface->link_exit                  = NULL;
        interface->connection_exit            = NULL;
        interface->channel_exit               = NULL;
        interface->adapter_exit               = mad_nmad_adapter_exit;
        interface->driver_exit                = mad_nmad_driver_exit;
        interface->choice                     = NULL;
        interface->get_static_buffer          = NULL;
        interface->return_static_buffer       = NULL;
#if defined CONFIG_SCHED_MINI_ALT
        interface->new_message                = NULL;
        interface->finalize_message           = NULL;
#  ifdef MAD_MESSAGE_POLLING
        interface->poll_message               = mad_nmad_poll_message;
#  endif // MAD_MESSAGE_POLLING
        interface->new_message                = mad_nmad_new_message;
        interface->receive_message            = mad_nmad_receive_message;
        interface->message_received           = NULL;
        interface->send_buffer                = mad_nmad_send_buffer;
        interface->receive_buffer             = mad_nmad_receive_buffer;
        interface->send_buffer_group          = mad_nmad_send_buffer_group;
        interface->receive_sub_buffer_group   = mad_nmad_receive_sub_buffer_group;
#else
        interface->new_message                = NULL;
        interface->finalize_message           = NULL;
#  ifdef MAD_MESSAGE_POLLING
        interface->poll_message               = NULL;
#  endif // MAD_MESSAGE_POLLING
        interface->new_message                = NULL;
        interface->receive_message            = NULL;
        interface->message_received           = NULL;
        interface->send_buffer                = NULL;
        interface->receive_buffer             = NULL;
        interface->send_buffer_group          = NULL;
        interface->receive_sub_buffer_group   = NULL;
#endif
        NM_LOG_OUT();

        return "nmad";
}

static
void
mad_nmad_driver_exit(p_mad_driver_t	   d) {
  p_mad_nmad_driver_specific_t	 ds		= d->specific;
  int err;

  TBX_FREE(ds->l_url);
  ds->l_url = NULL;

  if (p_core == NULL) return;

#ifdef CONFIG_SCHED_OPT
  err = nm_so_sr_exit(p_so_if);
  if(err != NM_ESUCCESS) {
    DISP("nm_so_sr_exit return err = %d\n", err);
    TBX_FAILURE("nmad error");
  }
#endif
  err = nm_core_driver_exit(p_core);
  if(err != NM_ESUCCESS) {
    DISP("nm_core_driver_exit return err = %d\n", err);
    TBX_FAILURE("nmad error");
  }
  err = nm_core_exit(p_core);
  if(err != NM_ESUCCESS) {
    DISP("nm_core__exit return err = %d\n", err);
    TBX_FAILURE("nmad error");
  }
  err = nm_ns_exit(p_core);
  if(err != NM_ESUCCESS) {
    DISP("nm_ns_exit return err = %d\n", err);
    TBX_FAILURE("nmad error");
  }
  p_core = NULL;
  NM_LOG_OUT();
}

static
void
mad_nmad_driver_init(p_mad_driver_t	   d,
                     int		  *argc,
                     char		***argv) {
        p_mad_nmad_driver_specific_t	 ds		= NULL;
        uint8_t			 	 drv_id;
        char				 *l_url;
        int                              err;
	puk_component_t                  driver_config = NULL;

        NM_LOG_IN();
        if (!p_core) {
                nm_mad3_init_core(argc, *argv);
#ifdef PROFILE
                DISP("mad3 emulation: profile on");

#  if 0
		/* Just to check profiling works as expected */
                NMAD_EVENT0();
                NMAD_EVENT1(14);
                NMAD_EVENT2(15, 43);
                NMAD_EVENTSTR("_texte_");
#  endif

                {
                        p_mad_madeleine_t	madeleine	= d->madeleine;
                        unsigned int		size		= tbx_slist_get_length(madeleine->dir->process_slist);
                        unsigned int		rank		= (unsigned int)madeleine->session->process_rank;

                        NMAD_EVENT_CONFIG(size, rank);
                }
#endif
        }

	driver_config = nm_core_component_load("driver", "custom");
	err = nm_core_driver_load_init(p_core, driver_config, &drv_id, &l_url);

        NMAD_EVENT_DRV_ID(drv_id);
        NMAD_EVENT_DRV_NAME(d->device_name);
        d->connection_type	= mad_bidirectional_connection;
        d->buffer_alignment	= 32;
        ds			= TBX_MALLOC(sizeof(mad_nmad_driver_specific_t));
        ds->drv_id	= drv_id;
        ds->l_url	= l_url;
        d->specific	= ds;
        NM_LOG_OUT();
}

static
void
mad_nmad_adapter_init(p_mad_adapter_t	a) {
        p_mad_nmad_driver_specific_t	ds	= NULL;
        p_mad_nmad_adapter_specific_t	as	= NULL;

        NM_LOG_IN();
        if (strcmp(a->dir_adapter->name, "default"))
                TBX_FAILURE("unsupported adapter");

        ds	= a->driver->specific;

        as	= TBX_MALLOC(sizeof(mad_nmad_adapter_specific_t));

        as->master_channel_id	= -1;
        as->cnx_darray	= tbx_darray_init();
        a->specific	= as;
        if (ds->l_url) {
                a->parameter	= tbx_strdup(ds->l_url);
        } else {
                a->parameter	= tbx_strdup("-");
        }
        NM_LOG_OUT();
}

static
void
mad_nmad_adapter_exit(p_mad_adapter_t	a) {
        p_mad_nmad_adapter_specific_t	as	= a->specific;

        tbx_darray_free(as->cnx_darray);

        TBX_FREE(as);
        a->specific = NULL;
}

static
void
mad_nmad_channel_init(p_mad_channel_t ch) {
        p_mad_nmad_adapter_specific_t	as	= NULL;
        p_mad_nmad_channel_specific_t	chs	= NULL;

        NM_LOG_IN();
        as		= ch->adapter->specific;
        chs		= TBX_MALLOC(sizeof(mad_nmad_channel_specific_t));
        chs->tag_id	= ch->dir_channel->id;
#ifdef CONFIG_MULTI_RAIL
        if (ch->adapter->driver->madeleine->master_channel_id == -1) {
                ch->adapter->driver->madeleine->master_channel_id = ch->dir_channel->id;
        }
#else
        if (as->master_channel_id == -1) {
                as->master_channel_id = ch->dir_channel->id;
        }
#endif
        ch->specific	= chs;
        NM_LOG_OUT();
}

static
void
mad_nmad_connection_init(p_mad_connection_t in,
                         p_mad_connection_t out) {
        p_mad_channel_t				ch	= NULL;
        p_mad_nmad_adapter_specific_t		as	= NULL;
        p_mad_nmad_channel_specific_t		chs	= NULL;
        p_mad_nmad_connection_specific_t	cs	= NULL;
        int err;
        p_tbx_darray_t                          cnx_darray;
        int                                     master_channel_id;

        NM_LOG_IN();
        ch	= in->channel;
        as	= in->channel->adapter->specific;
        chs	= in->channel->specific;
        cs	= TBX_MALLOC(sizeof(mad_nmad_connection_specific_t));

#ifdef CONFIG_MULTI_RAIL
        p_mad_madeleine_t madeleine = ch->adapter->driver->madeleine;
        master_channel_id = madeleine->master_channel_id;
        cnx_darray = madeleine->cnx_darray;
#else
        master_channel_id = as->master_channel_id;
        cnx_darray = as->cnx_darray;
#endif

        if (master_channel_id == ch->dir_channel->id) {
                err = nm_core_gate_init(p_core, &cs->gate_id);
                if (err != NM_ESUCCESS) {
                        printf("nm_core_gate_init returned err = %d\n", err);
                        TBX_FAILURE("nmad error");
                }

                tbx_darray_expand_and_set(cnx_darray, in->remote_rank,
                                          cs);
                cs->master_cnx	= cs;
        } else {
                cs->master_cnx	= NULL;
        }

#ifdef CONFIG_SCHED_OPT
        cs->in_next_seq		= 0;
        cs->in_wait_seq		= 0;
        cs->in_flow_ctrl	= 0;
#  ifdef MAD_NMAD_SO_DEBUG
        cs->in_deferred_slist	= tbx_slist_nil();
#  endif /* MAD_NMAD_SO_DEBUG */

        cs->out_next_seq	= 0;
        cs->out_wait_seq	= 0;
        cs->out_flow_ctrl	= 0;
#endif /* CONFIG_SCHED_OPT */
        in->specific	= out->specific	= cs;
        in->nb_link	= 1;
        out->nb_link	= 1;
        NM_LOG_OUT();
}

static
void
mad_nmad_link_init(p_mad_link_t lnk) {
        NM_LOG_IN();
        lnk->link_mode	= mad_link_mode_buffer;
        lnk->buffer_mode	= mad_buffer_mode_dynamic;
        lnk->group_mode	= mad_group_mode_split;
        NM_LOG_OUT();
}

static
void
mad_nmad_before_open_channel(p_mad_channel_t ch TBX_UNUSED) {
        /* nothing */
}

static
void
mad_nmad_accept(p_mad_connection_t   in,
                p_mad_adapter_info_t ai TBX_UNUSED) {
        p_mad_nmad_driver_specific_t		ds	= NULL;
        p_mad_nmad_adapter_specific_t		as	= NULL;
        p_mad_nmad_connection_specific_t	cs	= NULL;
        int err;

        NM_LOG_IN();
        cs	= in->specific;
        as	= in->channel->adapter->specific;
        ds	= in->channel->adapter->driver->specific;

#ifdef CONFIG_MULTI_RAIL
        p_mad_madeleine_t madeleine = in->channel->adapter->driver->madeleine;
        cs->master_cnx = tbx_darray_get(madeleine->cnx_darray, in->remote_rank);
        NM_TRACEF("accept: cnx_id = %d, remote node = %s, gate_id = %d",
                  in->remote_rank,
                  ai->dir_node->name,
                  cs->gate_id);
        err = nm_core_gate_accept(p_core, cs->gate_id, ds->drv_id, NULL);
        if (err != NM_ESUCCESS) {
          printf("nm_core_gate_accept returned err = %d\n", err);
          TBX_FAILURE("nmad error");
        }
        NMAD_EVENT_CNX_ACCEPT(in->remote_rank, cs->gate_id, ds->drv_id);
        TRACE("gate_accept: connection established");
#else
        if (cs->master_cnx) {
                NM_TRACEF("accept: cnx_id = %d, remote node = %s, gate_id = %d",
                          in->remote_rank,
                          ai->dir_node->name,
                          cs->gate_id);
                err = nm_core_gate_accept(p_core, cs->gate_id, ds->drv_id, NULL);
                if (err != NM_ESUCCESS) {
                        printf("nm_core_gate_accept returned err = %d\n", err);
                        TBX_FAILURE("nmad error");
                }
                NMAD_EVENT_CNX_ACCEPT(in->remote_rank, cs->gate_id, ds->drv_id);
                TRACE("gate_accept: connection established");
        } else {
                cs->master_cnx = tbx_darray_get(as->cnx_darray, in->remote_rank);
        }
#endif
        NM_LOG_OUT();

}

static
void
mad_nmad_connect(p_mad_connection_t   out,
                 p_mad_adapter_info_t ai) {
        p_mad_nmad_driver_specific_t		ds	= NULL;
        p_mad_nmad_adapter_specific_t		as	= NULL;
        p_mad_nmad_connection_specific_t	cs	= NULL;
        p_mad_dir_node_t			r_n	= NULL;
        p_mad_dir_adapter_t			r_a	= NULL;
        int err;

        NM_LOG_IN();
        cs	= out->specific;
        as	= out->channel->adapter->specific;
        ds	= out->channel->adapter->driver->specific;

        r_a	= ai->dir_adapter;
        r_n	= ai->dir_node;

#ifdef CONFIG_MULTI_RAIL
        p_mad_madeleine_t madeleine = out->channel->adapter->driver->madeleine;

        char * url;
        size_t url_len;

        cs->master_cnx = tbx_darray_get(madeleine->cnx_darray, out->remote_rank);
        if (!strcmp(out->channel->adapter->driver->device_name, "tcp")) {
          /* remove the default hostname possibly provided by the tcp driver and add the correct one */
          char *port_str = strchr(r_a->parameter, ':');
          if (port_str) {
            port_str++;
          } else {
            port_str = r_a->parameter;
          }
          url_len = strlen(r_n->name) + 1 + strlen(port_str) + 1;
          url = TBX_MALLOC(url_len);
          sprintf(url, "%s:%s", r_n->name, port_str);
        } else {
          url = tbx_strdup(r_a->parameter);
        }

        NM_TRACEF("connect: cnx_id = %d, remote node = %s, gate_id = %d",
                  out->remote_rank,
                  ai->dir_node->name,
                  cs->gate_id);

        err = nm_core_gate_connect(p_core, cs->gate_id, ds->drv_id, url);
        if (err != NM_ESUCCESS) {
          printf("nm_core_gate_connect returned err = %d\n", err);
          TBX_FAILURE("nmad error");
        }
        TBX_FREE(url);

        NMAD_EVENT_CNX_CONNECT(out->remote_rank, cs->gate_id, ds->drv_id);
        TRACE("gate_connect: connection established");
#else
        if (cs->master_cnx) {
		char * url;
		size_t url_len;

		if (!strcmp(out->channel->adapter->driver->device_name, "tcp")) {
			/* remove the default hostname possibly provided by the tcp driver and add the correct one */
			char *port_str = strchr(r_a->parameter, ':');
			if (port_str) {
				port_str++;
			} else {
				port_str = r_a->parameter;
			}
			url_len = strlen(r_n->name) + 1 + strlen(port_str) + 1;
			url = TBX_MALLOC(url_len);
			sprintf(url, "%s:%s", r_n->name, port_str);
		} else {
			url = tbx_strdup(r_a->parameter);
		}

                NM_TRACEF("connect: cnx_id = %d, remote node = %s, gate_id = %d",
                          out->remote_rank,
                          ai->dir_node->name,
                          cs->gate_id);

                err = nm_core_gate_connect(p_core, cs->gate_id, ds->drv_id, url);
                if (err != NM_ESUCCESS) {
                        printf("nm_core_gate_connect returned err = %d\n", err);
                        TBX_FAILURE("nmad error");
                }
		TBX_FREE(url);

                NMAD_EVENT_CNX_CONNECT(out->remote_rank, cs->gate_id, ds->drv_id);
                TRACE("gate_connect: connection established");
        } else {
                cs->master_cnx = tbx_darray_get(as->cnx_darray, out->remote_rank);
        }
#endif
        NM_LOG_OUT();
}

static
void
mad_nmad_after_open_channel(p_mad_channel_t channel TBX_UNUSED) {
        /* nothing */
}

void
mad_nmad_disconnect(p_mad_connection_t cnx TBX_UNUSED) {

        NM_LOG_IN();
        /* TODO */
        NM_LOG_OUT();
}
#ifdef CONFIG_SCHED_MINI_ALT
#  ifdef MAD_MESSAGE_POLLING
static
p_mad_connection_t
mad_nmad_poll_message(p_mad_channel_t ch) {

        p_mad_nmad_channel_specific_t	chs		= NULL;
        p_tbx_darray_t			in_darray	= NULL;
        p_mad_connection_t		in		= NULL;

        NM_LOG_IN();
        chs		= ch->specific;
        in_darray	= ch->in_connection_darray;

        /* TODO */

        NM_LOG_OUT();

        return in;
}
#  endif // MAD_MESSAGE_POLLING

static
void
mad_nmad_new_message(p_mad_connection_t out) {
        p_mad_channel_t				 ch	= NULL;
        p_mad_nmad_channel_specific_t		 chs	= NULL;
        p_mad_nmad_connection_specific_t	 cs	= NULL;
        struct nm_basic_rq			*p_rq	= NULL;
        void					*buf	= NULL;
        uint64_t				 len;
        int err;

        NM_LOG_IN();
        ch	= out->channel;
        chs	= ch->specific;
        cs	= out->specific;
        buf	= &ch->process_lrank;
        len	= sizeof(ch->process_lrank);

        err	= nm_basic_isend(p_proto, cs->gate_id, chs->tag_id, buf, len, &p_rq);
        if (err != NM_ESUCCESS) {
                printf("nm_basic_isend returned err = %d\n", err);
                TBX_FAILURE("nmad error");
        }

        err = nm_basic_wait(p_rq);
        if (err != NM_ESUCCESS) {
                printf("nm_schedule returned err = %d\n", err);
                TBX_FAILURE("nmad error");
        }
        NM_LOG_OUT();
}

static
p_mad_connection_t
mad_nmad_receive_message(p_mad_channel_t ch) {
        p_mad_nmad_channel_specific_t		 chs		= NULL;
        p_tbx_darray_t				 in_darray	= NULL;
        p_mad_connection_t			 in		= NULL;
        struct nm_basic_rq			*p_rq		= NULL;
        void					*buf		= NULL;
        uint64_t				 len;
        int remote_process_lrank	= -1;
        int err;

        NM_LOG_IN();
        chs		= ch->specific;
        in_darray	= ch->in_connection_darray;
        buf		= &remote_process_lrank;
        len		= sizeof(remote_process_lrank);

        /* TODO */
        err	= nm_basic_irecv_any(p_proto, chs->tag_id, buf, len, &p_rq);
        if (err != NM_ESUCCESS) {
                printf("nm_basic_irecv returned err = %d\n", err);
                TBX_FAILURE("nmad error");
        }

        err = nm_basic_wait(p_rq);
        if (err != NM_ESUCCESS) {
                printf("nm_basic_wait returned err = %d\n", err);
                TBX_FAILURE("nmad error");
        }

        in 	= tbx_darray_get(in_darray, remote_process_lrank);
        NM_LOG_OUT();

        return in;
}

static
void
mad_nmad_send_buffer(p_mad_link_t     lnk,
                     p_mad_buffer_t   b) {
        p_mad_nmad_channel_specific_t		 chs	= NULL;
        p_mad_nmad_connection_specific_t	 cs	= NULL;
        struct nm_basic_rq			*p_rq	= NULL;
        char					*buf	= NULL;
        uint64_t				 len;
        int err;

        NM_LOG_IN();
        chs	= lnk->connection->channel->specific;
        cs	= lnk->connection->specific;
        buf	= b->buffer + b->bytes_read;
        len	= b->bytes_written - b->bytes_read;

        err	= nm_basic_isend(p_proto, cs->gate_id, chs->tag_id, buf, len, &p_rq);
        if (err != NM_ESUCCESS) {
                printf("nm_basic_isend returned err = %d\n", err);
                TBX_FAILURE("nmad error");
        }

        err = nm_basic_wait(p_rq);
        if (err != NM_ESUCCESS) {
                printf("nm_basic_wait returned err = %d\n", err);
                TBX_FAILURE("nmad error");
        }

        b->bytes_read	+= len;
        NM_LOG_OUT();
}

static
void
mad_nmad_receive_buffer(p_mad_link_t     lnk,
                        p_mad_buffer_t *_b) {
        p_mad_buffer_t				 b	= *_b;
        p_mad_nmad_channel_specific_t		 chs	= NULL;
        p_mad_nmad_connection_specific_t	 cs	= NULL;
        struct nm_basic_rq			*p_rq	= NULL;
        char					*buf	= NULL;
        uint64_t				 len;
        int err;

        NM_LOG_IN();
        chs	= lnk->connection->channel->specific;
        cs	= lnk->connection->specific;
        buf	= b->buffer + b->bytes_written;
        len	= b->length - b->bytes_written;
        err	= nm_basic_irecv(p_proto, cs->gate_id, chs->tag_id, buf, len, &p_rq);
        if (err != NM_ESUCCESS) {
                printf("nm_basic_isend returned err = %d\n", err);
                TBX_FAILURE("nmad error");
        }

        err = nm_basic_wait(p_rq);
        if (err != NM_ESUCCESS) {
                printf("nm_basic_wait returned err = %d\n", err);
                TBX_FAILURE("nmad error");
        }

        b->bytes_written	+= len;
        NM_LOG_OUT();
}

static
void
mad_nmad_send_buffer_group_1(p_mad_link_t         lnk,
                             p_mad_buffer_group_t bg) {

        p_mad_nmad_connection_specific_t	cs	= NULL;
        tbx_list_reference_t			ref;

        NM_LOG_IN();
        if (tbx_empty_list(&(bg->buffer_list)))
                goto out;

        cs	= lnk->connection->specific;
        tbx_list_reference_init(&ref, &(bg->buffer_list));
        do {
                p_mad_buffer_t *b;
                b	= tbx_get_list_reference_object(&ref);
                /* TODO */
        } while(tbx_forward_list_reference(&ref));

 out:
        NM_LOG_OUT();
}

static
void
mad_nmad_receive_sub_buffer_group_1(p_mad_link_t         lnk,
                                    p_mad_buffer_group_t bg) {
        p_mad_nmad_connection_specific_t	cs	= NULL;
        tbx_list_reference_t			ref;

        NM_LOG_IN();
        if (tbx_empty_list(&(bg->buffer_list)))
                goto out;

        cs	= lnk->connection->specific;
        tbx_list_reference_init(&ref, &(bg->buffer_list));
        do {
                p_mad_buffer_t *b;
                b	= tbx_get_list_reference_object(&ref);
                /* TODO */
        } while(tbx_forward_list_reference(&ref));

 out:
        NM_LOG_OUT();
}

static
void
mad_nmad_send_buffer_group_2(p_mad_link_t         lnk,
                             p_mad_buffer_group_t bg) {
        p_mad_nmad_connection_specific_t	cs	= NULL;
        tbx_list_length_t               	len;
        tbx_list_reference_t            	ref;

        NM_LOG_IN();
        if (tbx_empty_list(&(bg->buffer_list)))
                goto out;

        cs	= lnk->connection->specific;
        len	= bg->buffer_list.length;
        tbx_list_reference_init(&ref, &(bg->buffer_list));

        if (len < MAD_NMAD_GROUP_MALLOC_THRESHOLD) {
                struct iovec a[len];
                int          i	= 0;

                do {
                        p_mad_buffer_t b	= NULL;

                        if (i >= len)
                                TBX_FAILURE("index out of bounds");

                        b	= tbx_get_list_reference_object(&ref);
                        a[i].iov_base	= b->buffer;
                        a[i].iov_len	= b->bytes_written - b->bytes_read;
                        b->bytes_read	= b->bytes_written;
                        i++;
                } while(tbx_forward_list_reference(&ref));

                /* TODO: send iov */
        } else {
                struct iovec *a	= NULL;
                int           i	= 0;

                a	= TBX_MALLOC(len * sizeof(struct iovec));

                do {
                        p_mad_buffer_t b	= NULL;

                        if (i >= len)
                                TBX_FAILURE("index out of bounds");

                        b	= tbx_get_list_reference_object(&ref);
                        a[i].iov_base	= b->buffer;
                        a[i].iov_len	= b->bytes_written - b->bytes_read;
                        b->bytes_read	= b->bytes_written;
                        i++;
                } while(tbx_forward_list_reference(&ref));

                /* TODO: send iov */
                TBX_FREE(a);
        }

 out:
        NM_LOG_OUT();
}

static
void
mad_nmad_receive_sub_buffer_group_2(p_mad_link_t         lnk,
                                    p_mad_buffer_group_t bg) {
        p_mad_nmad_connection_specific_t	cs	= NULL;
        tbx_list_length_t               	len;
        tbx_list_reference_t            	ref;

        NM_LOG_IN();
        if (tbx_empty_list(&(bg->buffer_list)))
                goto out;

        cs	= lnk->connection->specific;
        len	= bg->buffer_list.length;
        tbx_list_reference_init(&ref, &(bg->buffer_list));

        if (len < MAD_NMAD_GROUP_MALLOC_THRESHOLD) {
                struct iovec a[len];
                int          i	= 0;

                do {
                        p_mad_buffer_t b	= NULL;

                        if (i >= len)
                                TBX_FAILURE("index out of bounds");

                        b	= tbx_get_list_reference_object(&ref);
                        a[i].iov_base	= b->buffer;
                        a[i].iov_len	= b->length - b->bytes_written;
                        b->bytes_written	= b->length;
                        i++;
                } while(tbx_forward_list_reference(&ref));

                /* TODO: recv iov */
        } else {
                struct iovec *a	= NULL;
                int           i	= 0;

                a	= TBX_MALLOC(len * sizeof(struct iovec));

                do {
                        p_mad_buffer_t b	= NULL;

                        if (i >= len)
                                TBX_FAILURE("index out of bounds");

                        b	= tbx_get_list_reference_object(&ref);
                        a[i].iov_base		= b->buffer;
                        a[i].iov_len		= b->length - b->bytes_written;
                        b->bytes_written	= b->length;
                        i++;
                } while(tbx_forward_list_reference(&ref));

                /* TODO: recv iov */
                TBX_FREE(a);
        }

 out:
        NM_LOG_OUT();
}




static
void
mad_nmad_send_buffer_group(p_mad_link_t         lnk,
                           p_mad_buffer_group_t bg) {
        NM_LOG_IN();
        mad_nmad_send_buffer_group_1(lnk, bg);
        NM_LOG_OUT();
}

static
void
mad_nmad_receive_sub_buffer_group(p_mad_link_t         lnk,
                                  tbx_bool_t           first_sub_group
                                  __attribute__ ((unused)),
                                  p_mad_buffer_group_t bg) {
        NM_LOG_IN();
        mad_nmad_receive_sub_buffer_group_1(lnk, bg);
        NM_LOG_OUT();
}
#elif defined(CONFIG_SCHED_OPT)

/* Direct mapping of the high level interface */

#  ifdef MAD_NMAD_SO_DEBUG
static
void
mad_nmad_deferred_dump(p_tbx_slist_t l) {
        while (!tbx_slist_is_nil(l)) {
                p_mad_nmad_deferred_t	def	= NULL;

                def	= tbx_slist_extract(l);
                DISP("completed deferred unpack: seq = %d, tag_id = %d, len = %zx",
                     def->seq, def->tag_id, def->len);
                tbx_dump(def->ptr, def->len);
                TBX_FREE(def);
        }
}
#  endif /* MAD_NMAD_SO_DEBUG */

p_mad_connection_t
mad_nmad_begin_packing(p_mad_channel_t      ch,
                       ntbx_process_lrank_t remote_rank) {
  p_mad_connection_t	out = NULL;

  LOG_IN();
  TRACE("New emission request");
  if (!ch->not_private)
    return NULL;

  out = tbx_darray_get(ch->out_connection_darray, remote_rank);

  if (!out)
    {
      LOG_OUT();

      return NULL;
    }

#  ifdef MARCEL
  marcel_mutex_lock(&(out->lock_mutex));
#  else /* MARCEL */
  if (out->lock == tbx_true)
    TBX_FAILURE("mad_begin_packing: connection dead lock");
  out->lock = tbx_true;
#  endif /* MARCEL */

  /* Nothing */

  TRACE("Emission request initiated");
  LOG_OUT();

  return out;
}

p_mad_connection_t
mad_nmad_begin_unpacking_from(p_mad_channel_t      ch,
                              ntbx_process_lrank_t remote_rank) {
  p_mad_connection_t	in	= NULL;

  LOG_IN();
  TRACE("New selective reception request");
  if (!ch->not_private)
    return NULL;

  in = tbx_darray_get(ch->in_connection_darray, remote_rank);

  if (!in)
    {
      LOG_OUT();

      return NULL;
    }

#  ifdef MARCEL
  marcel_mutex_lock(&(in->lock_mutex));
#  else /* MARCEL */
  if (in->lock == tbx_true)
    TBX_FAILURE("mad_begin_unpacking_from: connection dead lock");
  in->lock = tbx_true;
#  endif /* MARCEL */

  /* Nothing */

  TRACE("Selective reception request initiated");
  LOG_OUT();

  return in;
}

void
mad_nmad_end_packing(p_mad_connection_t out) {
  p_mad_nmad_connection_specific_t	 cs	= NULL;
  p_mad_nmad_channel_specific_t	 	 chs	= NULL;

  LOG_IN();
  cs	= out->specific;
  chs	= out->channel->specific;

  if (cs->out_wait_seq != cs->out_next_seq) {
          uint8_t i;
          for (i = cs->out_wait_seq; i != cs->out_next_seq; i++) {
#  ifdef MAD_NMAD_SO_DEBUG
                  DISP("end wait send request[%d]: %d", i, cs->out_reqs[i]);
#  endif /* MAD_NMAD_SO_DEBUG */
                  nm_so_sr_swait(p_so_if, cs->out_reqs[i]);
          }
          cs->out_wait_seq	= cs->out_next_seq;
          cs->out_flow_ctrl	= 0;
  }

#ifdef MARCEL
  marcel_mutex_unlock(&(out->lock_mutex));
#else // MARCEL
  out->lock = tbx_false;
#endif // MARCEL
  TRACE("Emission request completed");
  LOG_OUT();
}


void
mad_nmad_end_unpacking(p_mad_connection_t in) {
  p_mad_nmad_connection_specific_t	 cs	= NULL;
  p_mad_nmad_channel_specific_t	 	 chs	= NULL;

  LOG_IN();
  cs	= in->specific;
  chs	= in->channel->specific;

  if (cs->in_wait_seq != cs->in_next_seq) {
          uint8_t i;

          for (i = cs->in_wait_seq; i != cs->in_next_seq; i++) {
#  ifdef MAD_NMAD_SO_DEBUG
                  DISP("end wait recv request[%d]: %d", i, cs->in_reqs[i]);
#  endif /* MAD_NMAD_SO_DEBUG */
                  nm_so_sr_rwait(p_so_if, cs->in_reqs[i]);
          }
          cs->in_wait_seq	= cs->in_next_seq;
          cs->in_flow_ctrl	= 0;
#  ifdef MAD_NMAD_SO_DEBUG
          mad_nmad_deferred_dump(cs->in_deferred_slist);
#  endif /* MAD_NMAD_SO_DEBUG */
  }

#ifdef MARCEL
  marcel_mutex_unlock(&(in->lock_mutex));
#else // MARCEL
  in->lock = tbx_false;
#endif // MARCEL
  TRACE("Reception request completed");
  LOG_OUT();
}

void
mad_nmad_pack(p_mad_connection_t   out,
              void                *ptr,
              size_t               len,
              mad_send_mode_t      send_mode,
              mad_receive_mode_t   receive_mode TBX_UNUSED) {
  p_mad_nmad_connection_specific_t	cs	= NULL;
  p_mad_nmad_channel_specific_t		chs	= NULL;

  LOG_IN();
  if (send_mode == mad_send_LATER)
          TBX_FAILURE("mad_send_LATER unimplemented");

  cs	= out->specific;
  chs	= out->channel->specific;

#  ifdef MAD_NMAD_SO_DEBUG
  DISP("pack: seq = %d, tag = %d, len = %zx",
       cs->out_next_seq, chs->tag_id, len);
  tbx_dump(ptr, len);
#  endif /* MAD_NMAD_SO_DEBUG */
  nm_so_sr_isend(p_so_if, cs->gate_id, chs->tag_id, ptr, len, &cs->out_reqs[cs->out_next_seq]);

#  ifdef MAD_NMAD_SO_DEBUG
  DISP("send request[%d]: %d", cs->out_next_seq, cs->out_reqs[cs->out_next_seq]);
#  endif /* MAD_NMAD_SO_DEBUG */

  cs->out_next_seq++;
  cs->out_flow_ctrl++;

  if (cs->out_flow_ctrl == 255) {
          uint8_t i;
          for (i = cs->out_wait_seq; i != cs->out_next_seq; i++) {
                  nm_so_sr_swait(p_so_if, cs->out_reqs[i]);
          }

          cs->out_wait_seq	= cs->out_next_seq;
          cs->out_flow_ctrl	= 0;
  } else if (send_mode == mad_send_SAFER) {
          nm_so_sr_swait(p_so_if, cs->out_reqs[cs->out_next_seq-1]);
          if (cs->out_flow_ctrl == 1) {
                            cs->out_wait_seq	= cs->out_next_seq;
                            cs->out_flow_ctrl	= 0;
          }
  }
  LOG_OUT();
}

void
mad_nmad_unpack(p_mad_connection_t   in,
                void                *ptr,
                size_t               len,
                mad_send_mode_t      send_mode,
                mad_receive_mode_t   receive_mode) {
  p_mad_nmad_connection_specific_t	 cs	= NULL;
  p_mad_nmad_channel_specific_t		 chs	= NULL;

  LOG_IN();
  if (send_mode == mad_send_LATER)
          TBX_FAILURE("mad_send_LATER unimplemented");

  cs	= in->specific;
  chs	= in->channel->specific;

  nm_so_sr_irecv(p_so_if, cs->gate_id, chs->tag_id, ptr, len, &cs->in_reqs[cs->in_next_seq]);

#  ifdef MAD_NMAD_SO_DEBUG
  DISP("recv request[%d]: %d", cs->in_next_seq, cs->in_reqs[cs->in_next_seq]);
#  endif /* MAD_NMAD_SO_DEBUG */

  cs->in_next_seq++;
  cs->in_flow_ctrl++;

  if (cs->in_flow_ctrl == 255) {
           uint8_t i;
          for (i = cs->in_wait_seq; i != cs->in_next_seq; i++) {
#  ifdef MAD_NMAD_SO_DEBUG
                  DISP("fc wait recv request[%d]: %d", i, cs->in_reqs[i]);
#  endif /* MAD_NMAD_SO_DEBUG */
                  nm_so_sr_rwait(p_so_if, cs->in_reqs[i]);
          }
          cs->in_wait_seq	= cs->in_next_seq;
          cs->in_flow_ctrl	= 0;
#  ifdef MAD_NMAD_SO_DEBUG
          mad_nmad_deferred_dump(cs->in_deferred_slist);
          DISP("unpack: seq = %d, tag_id = %d, len = %zx",
               cs->in_next_seq-1, chs->tag_id, len);
          tbx_dump(ptr, len);
#  endif /* MAD_NMAD_SO_DEBUG */
  } else if (receive_mode == mad_receive_EXPRESS) {
#  ifdef MAD_NMAD_SO_DEBUG
          DISP("express wait recv request[%d]: %d", cs->in_next_seq-1, cs->in_reqs[cs->in_next_seq-1]);
#  endif /* MAD_NMAD_SO_DEBUG */
          nm_so_sr_rwait(p_so_if, cs->in_reqs[cs->in_next_seq-1]);
          if (cs->in_flow_ctrl == 1) {
                            cs->in_wait_seq	= cs->in_next_seq;
                            cs->in_flow_ctrl	= 0;
          }
#  ifdef MAD_NMAD_SO_DEBUG
          DISP("unpack: seq = %d, tag_id = %d, len = %zx",
               cs->in_next_seq-1, chs->tag_id, len);
          tbx_dump(ptr, len);
#  endif /* MAD_NMAD_SO_DEBUG */
  } else {
#  ifdef MAD_NMAD_SO_DEBUG
          p_mad_nmad_deferred_t	def	= NULL;

          DISP("mad_receive_CHEAPER: potentially deferred reception");
          def	= TBX_MALLOC(sizeof(*def));
          def->ptr	= ptr;
          def->len	= len;
          def->seq	= cs->in_next_seq-1;
          def->tag_id	= chs->tag_id;
          tbx_slist_append(cs->in_deferred_slist, def);
#  endif  /* MAD_NMAD_SO_DEBUG */
  }
  LOG_OUT();
}

#endif
