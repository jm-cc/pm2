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
#endif

#if defined CONFIG_GM
#  include <nm_gm_public.h>
#endif

#if defined CONFIG_QSNET
#  include <nm_qsnet_public.h>
#endif

#include <nm_tcp_public.h>
#include <nm_basic_public.h>
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
        uint8_t			 tag_id;
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

/* static vars
 */

/* core and proto objects */
static struct nm_core	*p_core		= NULL;
static struct nm_proto	*p_proto	= NULL;


/*
 * Driver private functions
 * ------------------------
 */

static
void
nm_mad3_init_core(int	 *argc,
                  char	**argv) {
        int err;

        TRACE("Initializing NMAD driver");
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
        interface->new_message                = mad_nmad_new_message;
        interface->receive_message            = mad_nmad_receive_message;
        interface->message_received           = NULL;
        interface->send_buffer                = mad_nmad_send_buffer;
        interface->receive_buffer             = mad_nmad_receive_buffer;
        interface->send_buffer_group          = mad_nmad_send_buffer_group;
        interface->receive_sub_buffer_group   = mad_nmad_receive_sub_buffer_group;
        NM_LOG_OUT();

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

        NM_LOG_IN();
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
        a->specific	= as;
        a->parameter	= tbx_strdup(ds->l_url);
        NM_LOG_OUT();
}

static
void
mad_nmad_channel_init(p_mad_channel_t ch) {
        p_mad_nmad_channel_specific_t chs	= NULL;

        NM_LOG_IN();
        chs		= TBX_MALLOC(sizeof(mad_nmad_channel_specific_t));
        chs->tag_id	= ch->dir_channel->id;
        ch->specific	= chs;
        NM_LOG_OUT();
}

static
void
mad_nmad_connection_init(p_mad_connection_t in,
                         p_mad_connection_t out) {
        p_mad_nmad_connection_specific_t cs	= NULL;
        int err;

        NM_LOG_IN();
        cs	= TBX_MALLOC(sizeof(mad_nmad_connection_specific_t));
        err = nm_core_gate_init(p_core, &cs->gate_id);
        if (err != NM_ESUCCESS) {
                printf("nm_core_gate_init returned err = %d\n", err);
                TBX_FAILURE("nmad error");
        }
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
mad_nmad_before_open_channel(p_mad_channel_t ch) {
        /* nothing */
}

static
void
mad_nmad_accept(p_mad_connection_t   in,
                p_mad_adapter_info_t ai TBX_UNUSED) {
        p_mad_nmad_driver_specific_t		ds	= NULL;
        p_mad_nmad_connection_specific_t	cs	= NULL;
        int err;

        NM_LOG_IN();
        cs	= in->specific;
        ds	= in->channel->adapter->driver->specific;

        err = nm_core_gate_accept(p_core, cs->gate_id, ds->drv_id, NULL, NULL);
        if (err != NM_ESUCCESS) {
                printf("nm_core_gate_accept returned err = %d\n", err);
                TBX_FAILURE("nmad error");
        }
        DISP("gate_accept: connection established");
        NM_LOG_OUT();

}

static
void
mad_nmad_connect(p_mad_connection_t   out,
                 p_mad_adapter_info_t ai) {
        p_mad_nmad_driver_specific_t		ds	= NULL;
        p_mad_nmad_connection_specific_t	cs	= NULL;
        p_mad_dir_node_t			r_n	= NULL;
        p_mad_dir_adapter_t			r_a	= NULL;
        int err;

        NM_LOG_IN();
        cs	= out->specific;
        ds	= out->channel->adapter->driver->specific;

        r_a	= ai->dir_adapter;
        r_n	= ai->dir_node;
        err = nm_core_gate_connect(p_core, cs->gate_id, ds->drv_id,
                                   r_n->name, r_a->parameter);
        if (err != NM_ESUCCESS) {
                printf("nm_core_gate_connect returned err = %d\n", err);
                TBX_FAILURE("nmad error");
        }
        DISP("gate_connect: connection established");
        NM_LOG_OUT();
}

static
void
mad_nmad_after_open_channel(p_mad_channel_t channel) {
        /* nothing */
}

void
mad_nmad_disconnect(p_mad_connection_t cnx) {

        NM_LOG_IN();
        /* TODO */
        NM_LOG_OUT();
}

#ifdef MAD_MESSAGE_POLLING
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
#endif // MAD_MESSAGE_POLLING

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

