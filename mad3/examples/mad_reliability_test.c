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
 * mad_micro.c
 * ===========
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "pm2_common.h"

static const int param_nb_msgs   = 10000;
static const int param_loss_rate =    50;
static       int nb_lost_msgs    =     0;
static

void
process_status_slist(p_tbx_slist_t status_slist)
{
        if (!tbx_slist_is_nil(status_slist)) {

                tbx_slist_ref_to_head(status_slist);
                do {
                        p_mad_buffer_slice_parameter_t status = NULL;

                        status = tbx_slist_ref_get(status_slist);

                        if (status->opcode == mad_os_lost_block) {
                                nb_lost_msgs++;
                        } else
                                TBX_FAILURE("invalid or unknown status opcode");

                } while (tbx_slist_ref_forward(status_slist));
        }
}

void
ping(p_mad_channel_t channel)
{
        p_mad_connection_t  out    = NULL;

        int                 len    =    0;
        char               *chaine = "Hello, world";

        out = mad_begin_packing(channel, 1);
        len = strlen(chaine)+1;
        mad_pack(out, &len, sizeof(len), mad_send_CHEAPER, mad_receive_EXPRESS);

        {
                p_mad_buffer_slice_parameter_t param = NULL;

                param = mad_alloc_slice_parameter();
                param->opcode = mad_op_optional_block;
                param->value  = param_loss_rate;
                param->offset = 0;
                param->length = 0;
                mad_pack_ext(out,
                             chaine, len,
                             mad_send_CHEAPER, mad_receive_CHEAPER,
                             param, NULL);
        }

        mad_end_packing(out);
}

void
pong(p_mad_channel_t channel)
{
        p_mad_connection_t  in           = NULL;

        int                 len          =    0;
        char               *msg          = NULL;
        p_tbx_slist_t       status_slist = NULL;

        in = mad_begin_unpacking(channel);
        mad_unpack(in, &len, sizeof(len), mad_send_CHEAPER, mad_receive_EXPRESS);
        msg = TBX_CALLOC(1, len);
        mad_unpack(in, msg, len, mad_send_CHEAPER, mad_receive_CHEAPER);
        status_slist = mad_end_unpacking_ext(in);

        if (status_slist) {
                process_status_slist(status_slist);
                mad_free_parameter_slist(status_slist);
        }

        TBX_FREE(msg);
}

int
main(int argc, char **argv) {
        p_mad_madeleine_t          madeleine      = NULL;
        p_mad_channel_t            channel        = NULL;
        p_ntbx_process_container_t pc             = NULL;
        ntbx_process_grank_t       my_global_rank =   -1;
        ntbx_process_lrank_t       my_local_rank  =   -1;
        int i = 0;

        common_pre_init (&argc, argv, NULL);
        common_post_init(&argc, argv, NULL);
        madeleine      = mad_get_madeleine();
        my_global_rank = madeleine->session->process_rank;
        channel = tbx_htable_get(madeleine->channel_htable, "test");
        pc = channel->pc;
        my_local_rank = ntbx_pc_global_to_local(pc, my_global_rank);

        for (i = 0; i < param_nb_msgs; i++) {
                DISP_VAL("i", i);
                if (my_local_rank == 0) {
                        ping(channel);
                } else if (my_local_rank == 1) {
                        pong(channel);
                }
        }

        if (my_local_rank == 1) {
                LDISP("allowed loss rate: %d %%", param_loss_rate);
                LDISP("measured loss rate: %d %%", nb_lost_msgs * 100 / param_nb_msgs);
        }

        common_exit(NULL);

        return 0;
}
