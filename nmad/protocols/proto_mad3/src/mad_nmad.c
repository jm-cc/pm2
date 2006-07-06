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


#include <stdint.h>
#include <sys/uio.h>
#include <assert.h>

#include <tbx.h>

#include <nm_public.h>
#if defined CONFIG_SCHED_MINI_ALT
#  include <nm_mini_alt_public.h>
#else
#  error mad_nmad.c requires the mini alt scheduler for now
#endif

#if defined CONFIG_MX
#  include <nm_mx_public.h>
#elif defined CONFIG_GM
#  include <nm_gm_public.h>
#elif defined CONFIG_QSNET
#  include <nm_qsnet_public.h>
#else
#  include <nm_tcp_public.h>
#endif

#include "nm_mad3_private.h"

#include "madeleine.h"

#include <errno.h>


#define MAD_NMAD_GROUP_MALLOC_THRESHOLD 256

/*
 * local structures
 * ----------------
 */
typedef struct s_mad_nmad_driver_specific {
        uint8_t			 drv_id;
        char			*l_url;
} mad_nmad_driver_specific_t, *p_mad_nmad_driver_specific_t;

typedef struct s_mad_nmad_adapter_specific {
        int dummy;
} mad_nmad_adapter_specific_t, *p_mad_nmad_adapter_specific_t;

typedef struct s_mad_nmad_channel_specific {
        uint8_t			 proto_id;
} mad_nmad_channel_specific_t, *p_mad_nmad_channel_specific_t;

typedef struct s_mad_nmad_connection_specific {
  	uint8_t			 gate_id;
} mad_nmad_connection_specific_t, *p_mad_nmad_connection_specific_t;

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
mad_nmad_adapter_init(p_mad_adapter_t);

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

#ifdef MAD_MESSAGE_POLLING
static p_mad_connection_t
mad_nmad_poll_message(p_mad_channel_t);
#endif /* MAD_MESSAGE_POLLING */

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

/* Macros
 */
#define INITIAL_RQ_NUM		4


/* static vars
 */
/* fast allocator structs */
static p_tbx_memory_t	 nm_mad3_rq_mem	= NULL;

/* core and proto objects */
static struct nm_core	*p_core		= NULL;
static struct nm_proto	*p_proto	= NULL;

/* main madeleine object
 */
static p_mad_madeleine_t	p_madeleine	= NULL;



/*
 * Driver private functions
 * ------------------------
 */
/* successful outgoing request
 */
static
int
nm_mad3_out_success	(struct nm_proto 	* const p_proto,
                         struct nm_pkt_wrap	*p_pw) {
        struct nm_mad3_rq	 *p_rq	= NULL;
	int err;

        p_rq	= p_pw->proto_priv;
        p_rq->lock	= 0;
        p_rq->status	= NM_ESUCCESS;
	err = NM_ESUCCESS;

	return err;
}

/* failed outgoing request
 */
static
int
nm_mad3_out_failed	(struct nm_proto 	* const p_proto,
                         struct nm_pkt_wrap	*p_pw,
                         int			_err) {
        struct nm_mad3_rq	 *p_rq	= NULL;
	int err;

        p_rq	= p_pw->proto_priv;
        p_rq->lock	= 0;
        p_rq->status	= _err;
	err = NM_ESUCCESS;

	return err;
}

/* successful incoming request
 */
static
int
nm_mad3_in_success	(struct nm_proto 	* const p_proto,
                         struct nm_pkt_wrap	*p_pw) {
        struct nm_mad3_rq	 *p_rq	= NULL;
	int err;

        p_rq	= p_pw->proto_priv;
        p_rq->lock	= 0;
        p_rq->status	= NM_ESUCCESS;
	err = NM_ESUCCESS;

	return err;
}

/* failed incoming request
 */
static
int
nm_mad3_in_failed	(struct nm_proto 	* const p_proto,
                         struct nm_pkt_wrap	*p_pw,
                         int			_err) {
        struct nm_mad3_rq	 *p_rq	= NULL;
	int err;

        p_rq	= p_pw->proto_priv;
        p_rq->lock	= 0;
        p_rq->status	= _err;
	err = NM_ESUCCESS;

	return err;
}

/* protocol initialisation
 */
static
int
nm_mad3_proto_init	(struct nm_proto 	* const p_proto) {
        struct nm_core	 *p_core	= NULL;
        struct nm_mad3	 *p_mad3	= NULL;
        int i;
	int err;

        p_core	= p_proto->p_core;

        /* check if protocol may be registered
         */
        for (i = 0; i < 127; i++) {
                if (p_core->p_proto_array[128+i]) {
                        NM_DISPF("nm_mad3_proto_init: protocol entry %d already used",
                             128+i);

                        err	= -NM_EALREADY;
                        goto out;
                }
        }

        /* allocate private protocol part
         */
        p_mad3	= TBX_MALLOC(sizeof(struct nm_mad3));
        if (!p_mad3) {
                err = -NM_ENOMEM;
                goto out;
        }

        /* TODO: allocate the madeleine object wrapper
         */
#warning TODO
        p_mad3->p_madeleine	= NULL;

        /* setup allocator for request structs
         */
        tbx_malloc_init(&nm_mad3_rq_mem,   sizeof(struct nm_mad3_rq),
                        INITIAL_RQ_NUM,   "nmad/mad3/rq");

        /* register protocol
         */
        p_proto->priv	= p_mad3;
        for (i = 0; i < 127; i++) {
                p_core->p_proto_array[128+i] = p_proto;
        }

	err = NM_ESUCCESS;

 out:
	return err;
}

static
int
nm_mad3_load		(struct nm_proto_ops	*p_ops) {
        p_ops->init		= nm_mad3_proto_init;

        p_ops->out_success	= nm_mad3_out_success;
        p_ops->out_failed	= nm_mad3_out_failed;

        p_ops->in_success	= nm_mad3_in_success;
        p_ops->in_failed	= nm_mad3_in_failed;

        return NM_ESUCCESS;
}

static
void
nm_mad3_init_core(int	 *argc,
                  char	**argv) {
        int err;

        TRACE("Initializing NMAD driver");
        err = nm_core_init(argc, argv, &p_core, nm_mini_alt_load);
        if (err != NM_ESUCCESS) {
                DISP("nm_core_init returned err = %d\n", err);
                TBX_FAILURE("newmad error");
        }
        err = nm_core_proto_init(p_core, nm_mad3_load, &p_proto);
        if (err != NM_ESUCCESS) {
                DISP("nm_core_proto_init returned err = %d\n", err);
                TBX_FAILURE("newmad error");
        }
}

/*
 * Registration function
 * ---------------------
 */

char *
mad_nmad_register(p_mad_driver_interface_t interface) {
        LOG_IN();
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
        interface->adapter_exit               = NULL;
        interface->driver_exit                = NULL;
        interface->choice                     = NULL;
        interface->get_static_buffer          = NULL;
        interface->return_static_buffer       = NULL;
        interface->new_message                = NULL;
        interface->finalize_message           = NULL;
#ifdef MAD_MESSAGE_POLLING
        interface->poll_message               = mad_nmad_poll_message;
#endif // MAD_MESSAGE_POLLING
        interface->receive_message            = mad_nmad_receive_message;
        interface->message_received           = NULL;
        interface->send_buffer                = mad_nmad_send_buffer;
        interface->receive_buffer             = mad_nmad_receive_buffer;
        interface->send_buffer_group          = mad_nmad_send_buffer_group;
        interface->receive_sub_buffer_group   = mad_nmad_receive_sub_buffer_group;
        LOG_OUT();

        return "nmad";
}


static
void
mad_nmad_driver_init(p_mad_driver_t	   d,
                     int		  *argc,
                     char		***argv) {
        p_mad_nmad_driver_specific_t	 ds		= NULL;
        uint8_t			 	 drv_id		=    0;
        char				*l_url		= NULL;
        int err;

        LOG_IN();
        if (!p_core) {
                nm_mad3_init_core(argc, *argv);
        }

        if (tbx_streq(d->device_name, "tcp")) {
                err = nm_core_driver_init(p_core, nm_tcp_load, &drv_id, &l_url);
                goto found;
        }

#ifdef CONFIG_GM
        if (tbx_streq(d->device_name, "gm")) {
                err = nm_core_driver_init(p_core, nm_gm_load, &drv_id, &l_url);
                goto found;
        }
#endif

#ifdef CONFIG_MX
        if (tbx_streq(d->device_name, "mx")) {
                err = nm_core_driver_init(p_core, nm_mx_load, &drv_id, &l_url);
                goto found;
        }
#endif

#ifdef CONFIG_QSNET
        if (tbx_streq(d->device_name, "quadrics")) {
                err = nm_core_driver_init(p_core, nm_qsnet_load, &drv_id, &l_url);
                goto found;
        }
#endif

        TBX_FAILURE("driver unavailable");

found:
        d->connection_type	= mad_bidirectional_connection;
        d->buffer_alignment	= 32;
        ds			= TBX_MALLOC(sizeof(mad_nmad_driver_specific_t));
        ds->drv_id	= drv_id;
        ds->l_url	= l_url;
        d->specific	= ds;
        LOG_OUT();
}

static
void
mad_nmad_adapter_init(p_mad_adapter_t	a) {
        p_mad_nmad_driver_specific_t	ds	= NULL;
        p_mad_nmad_adapter_specific_t	as	= NULL;

        LOG_IN();
        if (strcmp(a->dir_adapter->name, "default"))
                TBX_FAILURE("unsupported adapter");

        ds	= a->driver->specific;

        as	= TBX_MALLOC(sizeof(mad_nmad_adapter_specific_t));
        a->specific	= as;
        a->parameter	= tbx_strdup(ds->l_url);
        LOG_OUT();
}

static
void
mad_nmad_channel_init(p_mad_channel_t ch) {
        p_mad_nmad_channel_specific_t chs	= NULL;

        LOG_IN();
        chs		= TBX_MALLOC(sizeof(mad_nmad_channel_specific_t));
        ch->specific	= chs;
        LOG_OUT();
}

static
void
mad_nmad_connection_init(p_mad_connection_t in,
                         p_mad_connection_t out) {
        p_mad_nmad_connection_specific_t cs	= NULL;

        LOG_IN();
        cs	= TBX_MALLOC(sizeof(mad_nmad_connection_specific_t));
        in->specific	= out->specific	= cs;
        in->nb_link	= 1;
        out->nb_link	= 1;
        LOG_OUT();
}

static
void
mad_nmad_link_init(p_mad_link_t lnk) {
        LOG_IN();
        lnk->link_mode	= mad_link_mode_buffer;
        lnk->buffer_mode	= mad_buffer_mode_dynamic;
        lnk->group_mode	= mad_group_mode_split;
        LOG_OUT();
}

static
void
mad_nmad_before_open_channel(p_mad_channel_t ch) {
        /* nothing */
}

static
void
mad_nmad_accept(p_mad_connection_t   in,
                p_mad_adapter_info_t ai TBX_UNUSED) {
        p_mad_nmad_adapter_specific_t    as	= NULL;
        p_mad_nmad_connection_specific_t cs	= NULL;

        LOG_IN();
        cs	= in->specific;
        as	= in->channel->adapter->specific;
        LOG_OUT();

}

static
void
mad_nmad_connect(p_mad_connection_t   out,
                 p_mad_adapter_info_t ai) {
        p_mad_nmad_connection_specific_t	cs		= NULL;
        p_mad_dir_node_t			r_node		= NULL;
        p_mad_dir_adapter_t			r_adapter	= NULL;

        LOG_IN();
        cs		= out->specific;
        r_node		= ai->dir_node;
        r_adapter	= ai->dir_adapter;
        LOG_OUT();
}

static
void
mad_nmad_after_open_channel(p_mad_channel_t channel) {
        /* nothing */
}

void
mad_nmad_disconnect(p_mad_connection_t cnx) {

        LOG_IN();
        /* TODO */
        LOG_OUT();
}

#ifdef MAD_MESSAGE_POLLING
static
p_mad_connection_t
mad_nmad_poll_message(p_mad_channel_t ch) {

        p_mad_nmad_channel_specific_t	chs		= NULL;
        p_tbx_darray_t			in_darray	= NULL;
        p_mad_connection_t		in		= NULL;

        LOG_IN();
        chs		= ch->specific;
        in_darray	= ch->in_connection_darray;

        /* TODO */

        LOG_OUT();

        return in;
}
#endif // MAD_MESSAGE_POLLING

static
p_mad_connection_t
mad_nmad_receive_message(p_mad_channel_t ch) {
        p_mad_nmad_channel_specific_t	chs		= NULL;
        p_tbx_darray_t			in_darray	= NULL;
        p_mad_connection_t		in		= NULL;

        LOG_IN();
        chs		= ch->specific;
        in_darray	= ch->in_connection_darray;

        /* TODO */

        LOG_OUT();

        return in;
}

static
void
mad_nmad_send_buffer(p_mad_link_t     lnk,
                     p_mad_buffer_t   b) {

        LOG_IN();
        /* TODO */
        LOG_OUT();
}

static
void
mad_nmad_receive_buffer(p_mad_link_t    lnk,
                        p_mad_buffer_t *b) {

        LOG_IN();
        /* TODO */
        LOG_OUT();
}

static
void
mad_nmad_send_buffer_group_1(p_mad_link_t         lnk,
                             p_mad_buffer_group_t bg) {

        p_mad_nmad_connection_specific_t	cs	= NULL;
        tbx_list_reference_t			ref;

        LOG_IN();
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
        LOG_OUT();
}

static
void
mad_nmad_receive_sub_buffer_group_1(p_mad_link_t         lnk,
                                    p_mad_buffer_group_t bg) {
        p_mad_nmad_connection_specific_t	cs	= NULL;
        tbx_list_reference_t			ref;

        LOG_IN();
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
        LOG_OUT();
}

static
void
mad_nmad_send_buffer_group_2(p_mad_link_t         lnk,
                             p_mad_buffer_group_t bg) {
        p_mad_nmad_connection_specific_t	cs	= NULL;
        tbx_list_length_t               	len;
        tbx_list_reference_t            	ref;

        LOG_IN();
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
        LOG_OUT();
}

static
void
mad_nmad_receive_sub_buffer_group_2(p_mad_link_t         lnk,
                                    p_mad_buffer_group_t bg) {
        p_mad_nmad_connection_specific_t	cs	= NULL;
        tbx_list_length_t               	len;
        tbx_list_reference_t            	ref;

        LOG_IN();
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
        LOG_OUT();
}




static
void
mad_nmad_send_buffer_group(p_mad_link_t         lnk,
                           p_mad_buffer_group_t bg) {
        LOG_IN();
        mad_nmad_send_buffer_group_2(lnk, bg);
        LOG_OUT();
}

static
void
mad_nmad_receive_sub_buffer_group(p_mad_link_t         lnk,
                                  tbx_bool_t           first_sub_group
                                  __attribute__ ((unused)),
                                  p_mad_buffer_group_t bg) {
        LOG_IN();
        mad_nmad_receive_sub_buffer_group_2(lnk, bg);
        LOG_OUT();
}

