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
 * mad_ping.c
 * ==========
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include "pm2_common.h"


// Macros
//......................


// Types
//......................
typedef struct s_mad_ping_result
{
        ntbx_process_lrank_t lrank_src;
        ntbx_process_lrank_t lrank_dst;
        int                  size;
        double               latency;
        double               bandwidth_mbit;
        double               bandwidth_mbyte;
} mad_ping_result_t, *p_mad_ping_result_t;

// Static variables
//......................

static const int param_control_receive   = 0;
static const int param_warmup            = 0;
static const int param_send_mode         = mad_send_CHEAPER;
static const int param_receive_mode      = mad_receive_CHEAPER;
static const int param_nb_samples        = 100;
static const int param_min_size          = MAD_LENGTH_ALIGNMENT;
static const int param_max_size          = 1024*1024*16;
static const int param_step              = 0; /* 0 = progression log. */
static const int param_nb_tests          = 5;
static const int param_fill_buffer       = 1;
static const int param_fill_buffer_value = 1;
static const int param_unreliable        = 0;

static ntbx_process_grank_t process_grank = -1;
static ntbx_process_lrank_t process_lrank = -1;
static ntbx_process_lrank_t master_lrank  = -1;

static unsigned char *main_buffer = NULL;

static p_tbx_slist_t  size_slist		= NULL;
static p_tbx_memory_t size_wrapper_memory	= NULL;

static char *filename = NULL;

// Functions
//......................

static
void *
wrap_int(int val) {
        int *ptr = tbx_malloc(size_wrapper_memory);
        *ptr = val;
        return ptr;
}

static
int
unwrap_int(int *ptr) {
        return *ptr;
}


static
void
default_size_func(void) {
        int size = 0;

        for (size  = param_min_size;
             size <= param_max_size;
             size  = param_step?size + param_step:size * 2) {
                tbx_slist_append(size_slist, wrap_int(size));
        }
}

static
void
file_size_func(void) {
        int          fd = 0;
        p_tbx_string_t s  = NULL;

        fd = open(filename, O_RDONLY);
        if (fd < 0) {
                perror("open");
                exit(EXIT_FAILURE);
        }

        while (1) {
                int  status = 0;
                char c      = 0;

                status = read(fd, &c, 1);
                if (status < 0) {
                        perror("read");
                        exit(EXIT_FAILURE);
                }

                if (!status) {
                        if (s) {
                                char *cs	= NULL;
                                int   value	= 0;

                                cs	= tbx_string_to_cstring_and_free(s);
                                s	= NULL;
                                value	= strtol(cs, NULL, 10);
                                free(cs);
                                tbx_slist_append(size_slist, wrap_int(value));
                        }

                        break;
                }

                if (isdigit(c)) {
                        if (!s) {
                                s = tbx_string_init_to_char(c);
                        } else {
                                tbx_string_append_char(s, c);
                        }
                } else if (s) {
                        char *cs	= NULL;
                        int   value	= 0;

                        cs	= tbx_string_to_cstring_and_free(s);
                        s	= NULL;
                        value	= strtol(cs, NULL, 10);
                        free(cs);
                        tbx_slist_append(size_slist, wrap_int(value));
                }
        }
}

static
void
init_size_list() {
        tbx_malloc_init(&size_wrapper_memory, sizeof(int), 16);
        size_slist = tbx_slist_nil();
}

static
void
mlp_pack(p_mad_connection_t out, int size) {
        if (param_unreliable) {
                p_mad_buffer_slice_parameter_t param = NULL;

                param = mad_alloc_slice_parameter();
                param->length = 0;
                param->opcode = mad_op_optional_block;
                param->value  = 100;
                mad_pack_ext(out, main_buffer, size,
                             param_send_mode, param_receive_mode, param, NULL);
        } else {
                mad_pack(out, main_buffer, size,
                         param_send_mode, param_receive_mode);
        }
}

static
void
mlp_unpack(p_mad_connection_t in, int size) {
        mad_unpack(in, main_buffer, size,
                   param_send_mode, param_receive_mode);
}

static
void
mlp_send_int(p_mad_connection_t out, int val) {
        ntbx_pack_buffer_t b;
        ntbx_pack_int(val, &b);
        mad_pack(out, &b, sizeof(b), mad_send_SAFER, mad_receive_EXPRESS);
}

static
int
mlp_receive_int(p_mad_connection_t in) {
        ntbx_pack_buffer_t b;
        mad_unpack(in, &b, sizeof(b), mad_send_SAFER, mad_receive_EXPRESS);
        return ntbx_unpack_int(&b);
}


static
void
mlp_send_double(p_mad_connection_t out, double val) {
        ntbx_pack_buffer_t b;
        ntbx_pack_double(val, &b);
        mad_pack(out, &b, sizeof(b), mad_send_SAFER, mad_receive_EXPRESS);
}

static
int
mlp_receive_double(p_mad_connection_t in) {
        ntbx_pack_buffer_t b;
        mad_unpack(in, &b, sizeof(b), mad_send_SAFER, mad_receive_EXPRESS);
        return ntbx_unpack_double(&b);
}


static void
mark_buffer(int len);

static void
mark_buffer(int len)
{
        unsigned int n = 0;
        int          i = 0;

        for (i = 0; i < len; i++) {
                unsigned char c = 0;

                n += 7;
                c = (unsigned char)(n % 256);

                main_buffer[i] = c;
        }
}

static void
clear_buffer(void);

static void
clear_buffer(void)
{
        memset(main_buffer, 0, param_max_size);
}

static void
fill_buffer(void)
{
        memset(main_buffer, param_fill_buffer_value, param_max_size);
}

static void
control_buffer(int len);

static void
control_buffer(int len)
{
        tbx_bool_t   ok = tbx_true;
        unsigned int n  = 0;
        int          i  = 0;

        for (i = 0; i < len; i++) {
                unsigned char c = 0;

                n += 7;
                c = (unsigned char)(n % 256);

                if (main_buffer[i] != c)
                        {
                                int v1 = 0;
                                int v2 = 0;

                                v1 = c;
                                v2 = main_buffer[i];
                                DISP("Bad data at byte %X: expected %X, received %X", i, v1, v2);
                                ok = tbx_false;
                        }
        }

        if (!ok) {
                DISP("%d bytes reception failed", len);
                FAILURE("data corruption");
        } else {
                DISP("ok");
        }
}

static
void
send_results(p_mad_channel_t     channel,
            p_mad_ping_result_t results) {
        p_mad_connection_t out = NULL;

        out = mad_begin_packing(channel, master_lrank);
        mlp_send_int   (out, results->lrank_src);
        mlp_send_int   (out, results->lrank_dst);
        mlp_send_int   (out, results->size);
        mlp_send_double(out, results->latency);
        mlp_send_double(out, results->bandwidth_mbit);
        mlp_send_double(out, results->bandwidth_mbyte);
        mad_end_packing(out);
}

static
void
receive_results(p_mad_channel_t     channel,
               p_mad_ping_result_t results) {
        p_mad_connection_t in = NULL;

        in = mad_begin_unpacking(channel);
        results->lrank_src       = mlp_receive_int(in);
        results->lrank_dst       = mlp_receive_int   (in);
        results->size            = mlp_receive_int   (in);
        results->latency         = mlp_receive_double(in);
        results->bandwidth_mbit  = mlp_receive_double(in);
        results->bandwidth_mbyte = mlp_receive_double(in);
        mad_end_unpacking(in);
}

static
void
display_results(p_mad_ping_result_t results) {
        LDISP("%3d %3d %12d %12.3f %8.3f %8.3f",
              results->lrank_src,
              results->lrank_dst,
              results->size,
              results->latency,
              results->bandwidth_mbit,
              results->bandwidth_mbyte);
}

static
void
process_results(p_mad_channel_t     channel,
		p_mad_ping_result_t results)
{
        LOG_IN();
        if (process_lrank == master_lrank) {
                if (!results) {
                        mad_ping_result_t tmp_results = { 0, 0, 0, 0.0, 0.0, 0.0 };
                        results = &tmp_results;
                        receive_results(channel, results);
                }

                display_results(results);
        } else if (results) {
                send_results(channel, results);
        }

        LOG_OUT();
}

static
void
ping_warmup(p_mad_channel_t      channel,
            ntbx_process_lrank_t lrank_dst) {
        p_mad_connection_t out = NULL;
        p_mad_connection_t in  = NULL;

        out = mad_begin_packing(channel, lrank_dst);
        mlp_pack(out, param_max_size);
        mad_end_packing(out);

        in = mad_begin_unpacking(channel);
        mlp_pack(in, param_max_size);
        mad_end_unpacking(in);
}

static
void
pong_warmup(p_mad_channel_t      channel,
            ntbx_process_lrank_t lrank_dst) {
        p_mad_connection_t out = NULL;
        p_mad_connection_t in  = NULL;

        in = mad_begin_unpacking(channel);
        mlp_unpack(in, param_max_size);
        mad_end_unpacking(in);

        out = mad_begin_packing(channel, lrank_dst);
        mlp_pack(out, param_max_size);
        mad_end_packing(out);
}

static
double
ping_loop(p_mad_channel_t      channel,
          ntbx_process_lrank_t lrank_dst,
          int                  size) {
        int        nb_samples = param_nb_samples;
        int        check      = !param_unreliable && param_control_receive;
        tbx_tick_t t1;
        tbx_tick_t t2;

        TBX_GET_TICK(t1);
        while (nb_samples--) {
                p_mad_connection_t out = NULL;
                p_mad_connection_t in  = NULL;

                if (check) {
                        mark_buffer(size);
                }

                out = mad_begin_packing(channel, lrank_dst);
                mlp_pack(out, size);
                mad_end_packing(out);

                if (check) {
                        clear_buffer();
                }

                in = mad_begin_unpacking(channel);
                mlp_unpack(in, size);
                mad_end_unpacking(in);

                if (check) {
                        control_buffer(size);
                }
        }
        TBX_GET_TICK(t2);

        return TBX_TIMING_DELAY(t1, t2);
}

static
void
pong_loop(p_mad_channel_t      channel,
          ntbx_process_lrank_t lrank_dst,
          int                  size) {
        int nb_samples = param_nb_samples;

        while (nb_samples--) {
                p_mad_connection_t in = NULL;
                p_mad_connection_t out = NULL;

                in = mad_begin_unpacking(channel);
                mlp_unpack(in, size);
                mad_end_unpacking(in);

                out = mad_begin_packing(channel, lrank_dst);
                mlp_pack(out, size);
                mad_end_packing(out);
        }
}

static
void
ping(p_mad_channel_t      channel,
     ntbx_process_lrank_t lrank_dst) {
        const double nb_messages =
                2.0 * (double)param_nb_tests * (double)param_nb_samples;

        LOG_IN();
        DISP_VAL("ping with", lrank_dst);

        if (param_fill_buffer) {
                fill_buffer();
        }

        if (param_warmup) {
                ping_warmup(channel, lrank_dst);
        }

        if (!tbx_slist_is_nil(size_slist)) {
                tbx_slist_ref_to_head(size_slist);
                do {
                        int size = unwrap_int(tbx_slist_ref_get(size_slist));
                        mad_ping_result_t results = { 0, 0, 0, 0.0, 0.0, 0.0 };
                        double            sum     = 0.0;

                        int nb_tests = param_nb_tests;

                        while (nb_tests--) {
                                sum += ping_loop(channel, lrank_dst, size);
                        }

                        results.lrank_src       = process_lrank;
                        results.lrank_dst       = lrank_dst;
                        results.size            = size;
                        results.bandwidth_mbit  = (nb_messages * size) / sum;
                        results.bandwidth_mbyte = results.bandwidth_mbit / 1.048576;
                        results.latency         = sum / nb_messages;

                        process_results(channel, &results);
                } while (tbx_slist_ref_forward(size_slist));
        }

        LOG_OUT();
}

static
void
pong(p_mad_channel_t      channel,
     ntbx_process_lrank_t lrank_dst)
{
        LOG_IN();
        DISP_VAL("pong with", lrank_dst);

        if (param_fill_buffer) {
                fill_buffer();
        }

        if (param_warmup) {
                pong_warmup(channel, lrank_dst);
        }

        if (!tbx_slist_is_nil(size_slist)) {
                tbx_slist_ref_to_head(size_slist);
                do {
                        int size = unwrap_int(tbx_slist_ref_get(size_slist));
                        int nb_tests = param_nb_tests;

                        while (nb_tests--) {
                                pong_loop(channel, lrank_dst, size);
                        }

                        process_results(channel, NULL);
                } while (tbx_slist_ref_forward(size_slist));
        }
        LOG_OUT();
}

static
void
master_loop(p_mad_channel_t channel)
{
        int size = 0;

        LOG_IN();
        if (!tbx_slist_is_nil(size_slist)) {
                tbx_slist_ref_to_head(size_slist);
                do {
                        size = unwrap_int(tbx_slist_ref_get(size_slist));
                        process_results(channel, NULL);
                } while (tbx_slist_ref_forward(size_slist));
        }
        LOG_OUT();
}

static
void
send_command(p_mad_channel_t      channel,
             ntbx_process_lrank_t lrank,
             ntbx_process_lrank_t peer_lrank,
             int                  command) {
        p_mad_connection_t out = NULL;
        p_mad_connection_t in  = NULL;

        out = mad_begin_packing(channel, lrank);
        mlp_send_int(out, peer_lrank);
        mlp_send_int(out, command);
        mad_end_packing(out);

        in = mad_begin_unpacking(channel);
        mlp_receive_int(in);
        mad_end_unpacking(in);
}

static
void
play_with_channel(p_mad_madeleine_t  madeleine,
		  char              *name)
{
        p_mad_channel_t            channel = NULL;
        p_ntbx_process_container_t pc      = NULL;
        tbx_bool_t                 status  = tbx_false;

        DISP_STR("Channel", name);
        channel = tbx_htable_get(madeleine->channel_htable, name);
        if (!channel) {
                DISP("I don't belong to this channel");

                return;
        }

        pc = channel->pc;

        status = ntbx_pc_first_local_rank(pc, &master_lrank);
        if (!status)
                return;

        process_lrank = ntbx_pc_global_to_local(pc, process_grank);
        DISP_VAL("My local channel rank is", process_lrank);

        if (process_lrank == master_lrank) {
                // Master
                ntbx_process_lrank_t lrank_src = -1;

                LDISP_STR("Channel", name);
                LDISP("src|dst|size        |latency     |10^6 B/s|MB/s    |");

                ntbx_pc_first_local_rank(pc, &lrank_src);
                do {
                        ntbx_process_lrank_t lrank_dst = -1;

                        lrank_dst = lrank_src;

                        if (!ntbx_pc_next_local_rank(pc, &lrank_dst))
                                break;

                        do {
                                if (lrank_dst == lrank_src)
                                        continue;

                                if (lrank_src == master_lrank) {
                                        send_command(channel, lrank_dst, lrank_src, 0);

                                        ping(channel, lrank_dst);
                                } else if (lrank_dst == master_lrank) {
                                        send_command(channel, lrank_src, lrank_dst, 1);

                                        pong(channel, lrank_src);
                                } else {
                                        send_command(channel, lrank_dst, lrank_src, 0);
                                        send_command(channel, lrank_src, lrank_dst, 1);
                                        master_loop(channel);
                                }
                        } while (ntbx_pc_next_local_rank(pc, &lrank_dst));
                } while (ntbx_pc_next_local_rank(pc, &lrank_src));

                DISP("test series completed");
                ntbx_pc_first_local_rank(pc, &lrank_src);

                while (ntbx_pc_next_local_rank(pc, &lrank_src)) {
                        p_mad_connection_t out = NULL;

                        out = mad_begin_packing(channel, lrank_src);
                        mlp_send_int(out, lrank_src);
                        mlp_send_int(out, 0);
                        mad_end_packing(out);
                }
        } else {
                /* Slaves */
                while (1) {
                        p_mad_connection_t   out            = NULL;
                        p_mad_connection_t   in             = NULL;
                        ntbx_process_lrank_t its_local_rank =   -1;
                        int                  direction      =    0;

                        in = mad_begin_unpacking(channel);
                        its_local_rank = mlp_receive_int(in);
                        direction      = mlp_receive_int(in);
                        mad_end_unpacking(in);

                        if (its_local_rank == process_lrank)
                                return;

                        out = mad_begin_packing(channel, 0);
                        mlp_send_int(out, 1);
                        mad_end_packing(out);

                        if (direction) {
                                /* Ping */
                                ping(channel, its_local_rank);
                        } else {
                                /* Pong */
                                pong(channel, its_local_rank);
                        }
                }
        }
}

int
main(int    argc,
     char **argv)
{
        p_mad_madeleine_t madeleine = NULL;
        p_mad_session_t   session   = NULL;
        p_tbx_slist_t     slist     = NULL;

        common_pre_init(&argc, argv, NULL);
        common_post_init(&argc, argv, NULL);
        TRACE("Returned from common_init");

        argc--;
        argv++;

        if (argc) {
                filename = *argv;
                argc--;
                argv++;
        }

        init_size_list();

        if (filename) {
                file_size_func();
        } else {
                default_size_func();
        }

        madeleine = mad_get_madeleine();

        session       = madeleine->session;
        process_grank = session->process_rank;
        {
                char host_name[1024] = "";

                SYSCALL(gethostname(host_name, 1023));
                DISP("(%s): My global rank is %d", host_name, process_grank);
        }

        DISP_VAL("The configuration size is",
                 tbx_slist_get_length(madeleine->dir->process_slist));

        slist = madeleine->public_channel_slist;
        if (!tbx_slist_is_nil(slist)) {
                main_buffer = tbx_aligned_malloc(param_max_size, MAD_ALIGNMENT);

                tbx_slist_ref_to_head(slist);
                do {
                        char *name = NULL;

                        name = tbx_slist_ref_get(slist);

                        play_with_channel(madeleine, name);
                } while (tbx_slist_ref_forward(slist));

                tbx_aligned_free(main_buffer, MAD_ALIGNMENT);
        } else {
                DISP("No channels");
        }

        DISP("Exiting");

        common_exit(NULL);

        return 0;
}
