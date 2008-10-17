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

#ifdef CONFIG_PROTO_MAD3
#include <madeleine.h>

int
main(int argc, char **argv) {
        p_mad_madeleine_t          madeleine      = NULL;
        p_mad_channel_t            channel        = NULL;
        p_ntbx_process_container_t pc             = NULL;
        ntbx_process_grank_t       my_global_rank =   -1;
        ntbx_process_lrank_t       my_local_rank  =   -1;

        madeleine = mad_init(&argc, argv);
        my_global_rank = madeleine->session->process_rank;
        channel = tbx_htable_get(madeleine->channel_htable, tbx_slist_index_get(madeleine->public_channel_slist, 0));
        pc = channel->pc;
        my_local_rank = ntbx_pc_global_to_local(pc, my_global_rank);

        if (my_local_rank == 0) {
                p_mad_connection_t  out    = NULL;
                char               *chaine = "abcd";

                out  = mad_begin_packing(channel, 1);
                mad_nmad_pack(out, chaine, 5,         mad_send_CHEAPER, mad_receive_CHEAPER);
                mad_end_packing(out);
        } else if (my_local_rank == 1) {
                p_mad_connection_t  in  = NULL;
                char               *msg = NULL;

                in = mad_begin_unpacking(channel);
                msg = TBX_MALLOC(5);
                mad_nmad_unpack(in, msg,  5,         mad_send_CHEAPER, mad_receive_CHEAPER);
                mad_nmad_end_unpacking(in);
                printf("msg = [%s]\n", msg);
                TBX_FREE(msg);
        }

        /*
         * On termine l'exécution des bibliothèques.
         */
        mad_exit(madeleine);
        return 0;
}
#else	/* CONFIG_PROTO_MAD3 */
int
main(int argc, char **argv) {
        printf("This program requires the proto_mad3 module\n");
        return 0;
}
#endif	/* CONFIG_PROTO_MAD3 */
