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
 * mad_basic_random.c
 * ==================
 */


/*
 * Note: This program interleaves packets construction and extraction
 * solely for the pupose of testing Madeleine's internals. Such an
 * interleaving MUST NOT be used in user applications as it may lead
 * to distributed dead-locks.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include "pm2_common.h"

#define BUF_LEN	1024

struct drand48_data	  main_rnd_buf;
p_ntbx_process_grank_t	  g;
int			  g_len;
char			**bs;
char			 *br;


static
long int
rand48(struct drand48_data *p_rand_buffer,
       long int             max_value) {
	double  rand_value	= 0.0;

	drand48_r(p_rand_buffer, &rand_value);
	return (long int)(rand_value * max_value);
}

static
int
brand48(struct drand48_data *p_rand_buffer) {
	double  rand_value	= 0.0;

	drand48_r(p_rand_buffer, &rand_value);

	return (int)(rand_value < 0.5);
}

void
shuffle(int *v, int len) {
        int v2[len];
        long int n;

        n = rand48(&main_rnd_buf, 1024);
        do {
                int j;

                for (j = 0; j < (len + 1) /2; j++) {
                        int j2 = len - j - 1;

                        if (j == j2) {
                                v2[2*j] = v[j];
                                continue;
                        }

                        if (brand48(&main_rnd_buf)) {
                                v2[2*j]     = v[j];
                                v2[2*j + 1] = v[j2];
                        } else {
                                v2[2*j]     = v[j2];
                                v2[2*j + 1] = v[j];
                        }
                }

                memcpy(v, v2, len*sizeof(int));
        } while (--n);
}


void
f(p_mad_channel_t 	channel,
  ntbx_process_grank_t  my_g) {
        p_ntbx_process_container_t	pc	= NULL;
        p_mad_connection_t		out[g_len];
        p_mad_connection_t		in;
        int				i;

        LOG_IN();
        pc	= channel->pc;

        DISP("send: first pass");
        for (i = 0; i < g_len; i++) {
                ntbx_process_lrank_t l;

                DISP("");
                DISP("==> %d", g[i]);
                l	= ntbx_pc_global_to_local(pc, g[i]);
                out[i]	= mad_begin_packing(channel, l);
                DISP("pack -->");
                mad_pack(out[i], bs[i], BUF_LEN,
                         mad_send_CHEAPER, mad_receive_CHEAPER);
                DISP("pack <--");
        }

        DISP("receive");
        for (i = 0; i < g_len; i++) {
                ntbx_process_grank_t _g;
                int err;
                int j;

                DISP("");
                DISP("begin_unpacking -->");
                in	= mad_begin_unpacking(channel);
                DISP("begin_unpacking <--");

                DISP("unpack -->");
                mad_unpack(in, br, BUF_LEN,
                           mad_send_CHEAPER, mad_receive_CHEAPER);
                DISP("unpack <--");

                DISP("end_unpacking -->");
                mad_end_unpacking(in);
                DISP("end_unpacking <--");

                _g	= ntbx_pc_local_to_global(pc, in->remote_rank);

                err = 0;
                for (j = 0; j < BUF_LEN; j++) {
                        char a = br[j];
                        char b = ((my_g + _g + j) % 255 + 1);

                        err = err || (a != b);
                }

                if (!err)
                        continue;

                DISP("message from process %d corrupted", _g);
                DISP("byte\t got\t expected");

                for (j = 0; j < BUF_LEN; j++) {
                        char a = br[j];
                        char b = ((my_g + _g + j) % 255 + 1);

                        if (a != b)
                                DISP("%x\t %x\t %x", j, a, b);
                }
        }

        DISP("send: second pass");
        for (i = 0; i < g_len; i++) {
                DISP("");
                mad_end_packing(out[i]);
        }
        LOG_OUT();
}



int
main(int argc, char **argv) {
        p_mad_madeleine_t         	 madeleine      = NULL;
        char                       	*name		= NULL;
        p_mad_channel_t           	 channel        = NULL;
        ntbx_process_grank_t      	 my_g		=   -1;

        LOG_IN();
        common_pre_init (&argc, argv, NULL);
        common_post_init(&argc, argv, NULL);

        madeleine	= mad_get_madeleine();
        my_g		= madeleine->session->process_rank;
        DISP("global_rank: %d", my_g);

        srand48_r(my_g, &main_rnd_buf);

        {
                p_tbx_slist_t ps_slist;
                int len;
                int i;

                ps_slist	= madeleine->dir->process_slist;
                len = tbx_slist_get_length(ps_slist) - 1;

                g	= TBX_MALLOC(len * sizeof(int));
                g_len	= len;

                bs	= TBX_MALLOC(len * sizeof(char *));

                i	= 0;
                tbx_slist_ref_to_head(ps_slist);
                do {
                        p_ntbx_process_t ps = tbx_slist_ref_get(ps_slist);

                        if (ps->global_rank == my_g)
                                continue;

                        assert(i < len);
                        g[i]	= ps->global_rank;
                        i++;
                } while (tbx_slist_ref_forward(ps_slist));

                shuffle(g, len);

                for (i = 0; i < len; i++) {
                        int j;

                        bs[i]	= TBX_MALLOC(BUF_LEN);
                        for (j = 0; j < BUF_LEN; j++)
                                bs[i][j] = (my_g + g[i] + j) % 255 + 1;
                }

                br	= TBX_MALLOC(BUF_LEN);
        }

        name	= tbx_slist_index_get(madeleine->public_channel_slist, 0);
        channel	= tbx_htable_get(madeleine->channel_htable, name);

        f(channel, my_g);

        common_exit(NULL);
        LOG_OUT();

        return 0;
}
