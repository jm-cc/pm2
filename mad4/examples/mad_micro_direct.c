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
 * mad_micro_direct.c
 * ==================
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "pm2_common.h"

int
main(int argc, char **argv) {
        p_mad_madeleine_t         	 madeleine      = NULL;
        char                       	*name		= NULL;
        p_mad_channel_t           	 channel        = NULL;
        p_ntbx_process_container_t	 pc             = NULL;
        ntbx_process_grank_t      	 my_global_rank =   -1;
        ntbx_process_lrank_t      	 my_local_rank  =   -1;

        common_pre_init (&argc, argv, NULL);
        common_post_init(&argc, argv, NULL);
        madeleine      = mad_get_madeleine();
        my_global_rank = madeleine->session->process_rank;
        name	= tbx_slist_index_get(madeleine->public_channel_slist, 0);
        channel = tbx_htable_get(madeleine->channel_htable, name);
        pc = channel->pc;
        my_local_rank = ntbx_pc_global_to_local(pc, my_global_rank);

        if (my_local_rank == 0) {
                p_mad_connection_t		out		= NULL;
                p_mad_driver_interface_t	interface	= NULL;
                p_mad_buffer_t			buffer_1	= NULL;
                p_mad_buffer_t			buffer_2	= NULL;
                p_mad_link_t			lnk		= NULL;

                int                 len    =    0;
                char               *chaine = "Hello, world";

                out		= tbx_darray_get(channel->out_connection_darray, 1);
                interface	= out->channel->adapter->driver->interface;
                lnk		= out->link_array[0];

                if (lnk->buffer_mode != mad_buffer_mode_dynamic)
                        FAILURE("unsupported buffer mode");

                if (interface->new_message) {
                        interface->new_message(out);
                }

                len = strlen(chaine)+1;

                buffer_1 = mad_get_user_send_buffer(&len, sizeof(len));
                interface->send_buffer(lnk, buffer_1);

                buffer_2 = mad_get_user_send_buffer(chaine, len);
                interface->send_buffer(lnk, buffer_2);

                if (interface->finalize_message) {
                        interface->finalize_message(out);
                }

                mad_free_buffer_struct(buffer_1);
                mad_free_buffer_struct(buffer_2);
        } else if (my_local_rank == 1) {
                p_mad_connection_t  		in  		= NULL;
                p_mad_driver_interface_t	interface	= NULL;
                p_mad_buffer_t			buffer_1	= NULL;
                p_mad_buffer_t			buffer_2	= NULL;
                p_mad_link_t			lnk		= NULL;

                int                 len =    0;
                char               *msg = NULL;

                interface	= channel->adapter->driver->interface;
                in		= interface->receive_message(channel);
                lnk		= in->link_array[0];

                if (lnk->buffer_mode != mad_buffer_mode_dynamic)
                        FAILURE("unsupported buffer mode");

                buffer_1 = mad_get_user_receive_buffer(&len, sizeof(len));
                interface->receive_buffer(lnk, &buffer_1);

                msg = malloc(len);

                buffer_2 = mad_get_user_receive_buffer(msg, len);
                interface->receive_buffer(lnk, &buffer_2);

                if (interface->message_received) {
                        interface->message_received(in);
                }

                printf("msg = [%s]\n", msg);

                mad_free_buffer_struct(buffer_1);
                mad_free_buffer_struct(buffer_2);
                free(msg);
        }

        common_exit(NULL);

        return 0;
}
