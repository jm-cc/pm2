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
 * Mad_mx.c
 * =========
 */


#include "madeleine.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <myriexpress.h>


/*
 * local structures
 * ----------------
 */
typedef struct s_mad_mx_driver_specific
{
        int dummy;
} mad_mx_driver_specific_t, *p_mad_mx_driver_specific_t;

typedef struct s_mad_mx_adapter_specific
{
        int dummy;
} mad_mx_adapter_specific_t, *p_mad_mx_adapter_specific_t;

typedef struct s_mad_mx_channel_specific
{
        int dummy;
} mad_mx_channel_specific_t, *p_mad_mx_channel_specific_t;

typedef struct s_mad_mx_connection_specific
{
        int dummy;
} mad_mx_connection_specific_t, *p_mad_mx_connection_specific_t;

typedef struct s_mad_mx_link_specific
{
        int dummy;
} mad_mx_link_specific_t, *p_mad_mx_link_specific_t;


/*
 * static functions
 * ----------------
 */


/*
 * Registration function
 * ---------------------
 */

void
mad_mx_register(p_mad_driver_t driver)
{
        p_mad_driver_interface_t interface = NULL;

        LOG_IN();
        TRACE("Registering MX driver");
        interface = driver->interface;

        driver->connection_type  = mad_unidirectional_connection;
        driver->buffer_alignment = 32;
        driver->name             = tbx_strdup("mx");

        interface->driver_init                = mad_mx_driver_init;
        interface->adapter_init               = mad_mx_adapter_init;
        interface->channel_init               = mad_mx_channel_init;
        interface->before_open_channel        = NULL;
        interface->connection_init            = mad_mx_connection_init;
        interface->link_init                  = mad_mx_link_init;
        interface->accept                     = mad_mx_accept;
        interface->connect                    = mad_mx_connect;
        interface->after_open_channel         = NULL;
        interface->before_close_channel       = NULL;
        interface->disconnect                 = mad_mx_disconnect;
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
        interface->poll_message               = mad_mx_poll_message;
#endif // MAD_MESSAGE_POLLING
        interface->receive_message            = mad_mx_receive_message;
        interface->message_received           = NULL;
        interface->send_buffer                = mad_mx_send_buffer;
        interface->receive_buffer             = mad_mx_receive_buffer;
        interface->send_buffer_group          = mad_mx_send_buffer_group;
        interface->receive_sub_buffer_group   = mad_mx_receive_sub_buffer_group;
        LOG_OUT();
}


void
mad_mx_driver_init(p_mad_driver_t d)
{
        p_mad_mx_driver_specific_t ds = NULL;

        LOG_IN();
        TRACE("Initializing MX driver");
        ds  		= TBX_MALLOC(sizeof(mad_mx_driver_specific_t));
        d->specific	= ds;
        LOG_OUT();
}

void
mad_mx_adapter_init(p_mad_adapter_t a)
{
        p_mad_mx_adapter_specific_t as = NULL;

        LOG_IN();
        if (strcmp(a->dir_adapter->name, "default"))
                FAILURE("unsupported adapter");

        as		= TBX_MALLOC(sizeof(mad_mx_adapter_specific_t));
        a->specific	= as;
        a->parameter	= NULL;
        LOG_OUT();
}

void
mad_mx_channel_init(p_mad_channel_t ch)
{
        p_mad_mx_channel_specific_t chs = NULL;

        LOG_IN();
        chs		= TBX_MALLOC(sizeof(mad_mx_channel_specific_t));
        ch->specific	= chs;
        LOG_OUT();
}

void
mad_mx_connection_init(p_mad_connection_t in,
                       p_mad_connection_t out)
{
        LOG_IN();
        if (in) {
                p_mad_mx_connection_specific_t is = NULL;

                is		= TBX_MALLOC(sizeof(mad_mx_connection_specific_t));
                in->specific	= is;
                in->nb_link	= 1;
        }

        if (out) {
                p_mad_mx_connection_specific_t os = NULL;
                os		= TBX_MALLOC(sizeof(mad_mx_connection_specific_t));
                out->specific	= os;
                out->nb_link	= 1;
        }
        LOG_OUT();
}

void
mad_mx_link_init(p_mad_link_t lnk)
{
        LOG_IN();
        /* lnk->link_mode   = mad_link_mode_buffer_group; */
        lnk->link_mode   = mad_link_mode_buffer;
        lnk->buffer_mode = mad_buffer_mode_dynamic;
        lnk->group_mode  = mad_group_mode_split;
        LOG_OUT();
}

void
mad_mx_accept(p_mad_connection_t   in,
              p_mad_adapter_info_t ai TBX_UNUSED)
{
        p_mad_mx_connection_specific_t is = NULL;

        LOG_IN();
        is = in->specific;
        LOG_OUT();

}

void
mad_mx_connect(p_mad_connection_t   out,
               p_mad_adapter_info_t ai)
{
        p_mad_mx_connection_specific_t os = NULL;

        LOG_IN();
        os = out->specific;
        LOG_OUT();
}

void
mad_mx_disconnect(p_mad_connection_t c)
{
        p_mad_mx_connection_specific_t cs = NULL;

        LOG_IN();
        cs = c->specific;
        LOG_OUT();
}

#ifdef MAD_MESSAGE_POLLING
p_mad_connection_t
mad_mx_poll_message(p_mad_channel_t channel)
{
        FAILURE("unsupported");
}
#endif // MAD_MESSAGE_POLLING

p_mad_connection_t
mad_mx_receive_message(p_mad_channel_t ch)
{
        p_mad_mx_channel_specific_t	chs		= NULL;
        p_tbx_darray_t			in_darray	= NULL;
        p_mad_connection_t		in		= NULL;

        LOG_IN();
        chs		= ch->specific;
        in_darray	= ch->in_connection_darray;

        LOG_OUT();

        return in;
}

void
mad_mx_send_buffer(p_mad_link_t     lnk,
                   p_mad_buffer_t   buffer)
{
        p_mad_connection_t		out	= NULL;
        p_mad_mx_connection_specific_t	os	= NULL;

        LOG_IN();
        out	= lnk->connection;
        os	= out->specific;
        LOG_OUT();
}

void
mad_mx_receive_buffer(p_mad_link_t    lnk,
                      p_mad_buffer_t *buffer)
{
        p_mad_connection_t		in	= NULL;
        p_mad_mx_connection_specific_t	is	= NULL;

        LOG_IN();
        in	= lnk->connection;
        is	= in->specific;
        LOG_OUT();
}

void
mad_mx_send_buffer_group_1(p_mad_link_t         lnk,
                           p_mad_buffer_group_t buffer_group)
{
        LOG_IN();
        if (!tbx_empty_list(&(buffer_group->buffer_list))) {
                tbx_list_reference_t ref;

                tbx_list_reference_init(&ref, &(buffer_group->buffer_list));
                do {
                        p_mad_buffer_t b = NULL;

                        b = tbx_get_list_reference_object(&ref);
                        mad_mx_send_buffer(lnk, b);
                } while(tbx_forward_list_reference(&ref));
        }
        LOG_OUT();
}

void
mad_mx_receive_sub_buffer_group_1(p_mad_link_t         lnk,
                                  p_mad_buffer_group_t buffer_group)
{
        LOG_IN();
        if (!tbx_empty_list(&(buffer_group->buffer_list))) {
                tbx_list_reference_t ref;

                tbx_list_reference_init(&ref, &(buffer_group->buffer_list));
                do {
                        p_mad_buffer_t b = NULL;

                        b = tbx_get_list_reference_object(&ref);
                        mad_mx_receive_buffer(lnk, &b);
                } while(tbx_forward_list_reference(&ref));
        }
        LOG_OUT();
}

void
mad_mx_send_buffer_group(p_mad_link_t         lnk,
                         p_mad_buffer_group_t buffer_group)
{
        LOG_IN();
        mad_mx_send_buffer_group_1(lnk, buffer_group);
        LOG_OUT();
}

void
mad_mx_receive_sub_buffer_group(p_mad_link_t         lnk,
                                tbx_bool_t           first_sub_group
                                __attribute__ ((unused)),
                                p_mad_buffer_group_t buffer_group)
{
        LOG_IN();
        mad_mx_receive_sub_buffer_group_1(lnk, buffer_group);
        LOG_OUT();
}

