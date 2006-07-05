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

/*
 * Mad_nmad.c
 * =========
 */


#include "madeleine.h"

#include <stdlib.h>
#include <sys/uio.h>
#include <errno.h>


#define MAD_NMAD_GROUP_MALLOC_THRESHOLD 256

/*
 * local structures
 * ----------------
 */
typedef struct s_mad_nmad_driver_specific {
        struct nm_core		*p_core;
        struct nm_proto		*p_proto;
} mad_nmad_driver_specific_t, *p_mad_nmad_driver_specific_t;

typedef struct s_mad_nmad_adapter_specific {
        uint8_t			 drv_id;
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
        p_mad_nmad_driver_specific_t ds	= NULL;

        LOG_IN();
        TRACE("Initializing NMAD driver");
        d->connection_type	= mad_bidirectional_connection;
        d->buffer_alignment	= 32;
        ds			= TBX_MALLOC(sizeof(mad_nmad_driver_specific_t));
        d->specific	= ds;
        LOG_OUT();
}

static
void
mad_nmad_adapter_init(p_mad_adapter_t	a) {
        p_mad_nmad_adapter_specific_t	as	= NULL;
        p_tbx_string_t		param	= NULL;

        LOG_IN();
        if (strcmp(a->dir_adapter->name, "default"))
                TBX_FAILURE("unsupported adapter");

        as	= TBX_MALLOC(sizeof(mad_nmad_adapter_specific_t));
        a->specific		= as;

        param		= tbx_string_init_to_int(1234);	/* dummy param */
        a->parameter	= tbx_string_to_cstring(param);
        tbx_string_free(param);
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

