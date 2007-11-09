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
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2006 "the PM2 team" (see AUTHORS file)
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
 * basic_ping.c
 * ============
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include <nm_public.h>

/* Choose a scheduler
 */
#include <nm_public.h>
#if defined CONFIG_SCHED_MINI_ALT
#  include <nm_mini_alt_public.h>
#elif defined CONFIG_SCHED_OPT
#  include <nm_so_public.h>
#else
#  include <nm_mini_public.h>
#endif

/* Choose an API
 */
#include <nm_basic_public.h>

/* Choose a driver
 */
#include <nm_drivers.h>

#include <tbx.h>

//#define CHRONO


// Types
//......................
typedef struct s_ping_result
{
    int                  size;
    double               latency;
    double               bandwidth_mbit;
    double               bandwidth_mbyte;
} ping_result_t, *p_ping_result_t;

// Static variables
//......................

static const int param_control_receive   = 0;
static const int param_warmup            = 0;
static const int param_nb_samples        = 1000;
static const int param_min_size          = 4; //32768;
static const int param_max_size          = 65536; //32768; //1024*1024*2; //16384;
static const int param_step              = 0; /* 0 = progression log. */
static const int param_nb_tests          = 1; //5;
static const int param_no_zero           = 1;
static const int param_fill_buffer       = 1;
static const int param_fill_buffer_value = 1;
static const int param_one_way           = 0;
static const int param_dynamic_allocation	= 0;

static unsigned char		*main_buffer	= NULL;

static struct nm_core		*p_core		= NULL;
static struct nm_proto		*p_proto	= NULL;
static struct nm_basic_rq	*p_rq0		= NULL;
static struct nm_basic_rq	*p_rq1		= NULL;
static uint8_t			 drv_id		=    0;
static nm_gate_id_t              gate_id	=    0;

// Functions
//......................

static void
mark_buffer(int len);

static void
mark_buffer(int len) {
    unsigned int n = 0;
    int          i = 0;

    for (i = 0; i < len; i++)
        {
            unsigned char c = 0;

            n += 7;
            c = (unsigned char)(n % 256);

            main_buffer[i] = c;
        }
}

static void
clear_buffer(void);

static void
clear_buffer(void) {
    memset(main_buffer, 0, param_max_size);
}

static void
fill_buffer(void) {
    memset(main_buffer, param_fill_buffer_value, param_max_size);
}

static void
control_buffer(int len);

static void
control_buffer(int len) {
    int		ok = 1;
    unsigned int	n  = 0;
    int		i  = 0;

    for (i = 0; i < len; i++) {
        unsigned char c = 0;

        n += 7;
        c = (unsigned char)(n % 256);

        if (main_buffer[i] != c) {
            int v1 = 0;
            int v2 = 0;

            v1 = c;
            v2 = main_buffer[i];
            printf("Bad data at byte %X: expected %X, received %X\n", i, v1, v2);
            ok = 0;
        }
    }

    if (!ok) {
        printf("%d bytes reception failed", len);
    } else {
        printf("############### DATA OK #############\n");
    }
}

static
void
cond_alloc_main_buffer(size_t size) {
    if (param_dynamic_allocation) {
        main_buffer = tbx_aligned_malloc(size, 8);
    }
}

static
void
cond_free_main_buffer(void) {
    if (param_dynamic_allocation) {
        tbx_aligned_free(main_buffer, 8);
    }
}

static __inline__
void
b_isend(void *buf, uint64_t len,
        uint8_t tag_id) {
    int err;

    if(tag_id == 0){
        err = nm_basic_isend(p_proto, 0, 0, buf, len, &p_rq0);
    } else {
        err = nm_basic_isend(p_proto, 0, 1, buf, len, &p_rq1);
    }

    if (err != NM_ESUCCESS) {
        printf("nm_basic_isend returned err = %d\n", err);
        goto out;
    }
    return;

 out:
    exit(EXIT_FAILURE);
}

static __inline__
void
b_irecv(void *buf, uint64_t len,
        uint8_t tag_id) {
    int err;

    if(tag_id == 0){
        err= nm_basic_irecv(p_proto, 0, 0, buf, len, &p_rq0);
    } else {
        err= nm_basic_irecv(p_proto, 0, 1, buf, len, &p_rq1);
    }

    if (err != NM_ESUCCESS) {
        printf("nm_basic_isend returned err = %d\n", err);
        goto out;
    }
    return;

 out:
    exit(EXIT_FAILURE);
}

int
b_test(struct nm_basic_rq *p_rq){
    int err;

    err = nm_basic_test(p_rq);
    return err;
}

void b_test_global(void){
    int err;
    while(1){
        err = b_test(p_rq0);
        if(err == NM_ESUCCESS)
            goto poll_rq1;
        err = b_test(p_rq1);
        if(err == NM_ESUCCESS)
            goto poll_rq0;
    }
    poll_rq0 :
        while(1){
            err = b_test(p_rq0);
            if(err == NM_ESUCCESS)
                return;
        }

    poll_rq1 :
        while(1){
            err = b_test(p_rq1);
            if(err == NM_ESUCCESS)
                return;
        }
}


static
void
ping(void) {
    int size = 0;

    if (!param_dynamic_allocation && param_fill_buffer) {
        fill_buffer();
    }

    for (size = param_min_size;
         size <= param_max_size;
         size = param_step?size + param_step:size * 2) {
        const int       _size   = (!size && param_no_zero)?1:size;
        //ping_result_t	results = { 0, 0.0, 0.0, 0.0 };
        double          sum     = 0.0;
        tbx_tick_t      t1;
        tbx_tick_t      t2;

#ifdef CHRONO
        tbx_tick_t      t3;
        tbx_tick_t      t4;
        tbx_tick_t      t5;
        double          temps_envoi     = 0.0;
        double          temps_reception = 0.0;
#endif

        sum = 0;


        int nb_tests = param_nb_tests;

        while (nb_tests--) {
            int nb_samples = param_nb_samples;

            TBX_GET_TICK(t1);
            while (nb_samples--) {
                cond_alloc_main_buffer(_size);

                if (param_control_receive) {
                    mark_buffer(_size);
                }

#ifdef CHRONO
                TBX_GET_TICK(t3);
#endif

                b_isend(main_buffer, _size, 0);
                b_isend(main_buffer, _size, 1);

                b_test_global();

#ifdef CHRONO
                TBX_GET_TICK(t4);
#endif
                if (param_control_receive) {
                    clear_buffer();
                }

                b_irecv(main_buffer, _size, 0);
                b_irecv(main_buffer, _size, 1);

                b_test_global();

#ifdef CHRONO
                TBX_GET_TICK(t5);
#endif

                if (param_control_receive) {
                    control_buffer(_size);
                }
                cond_free_main_buffer();

#ifdef CHRONO
                temps_envoi += TBX_TIMING_DELAY(t3, t4);
                temps_reception += TBX_TIMING_DELAY(t4, t5);
#endif
            }
            TBX_GET_TICK(t2);

            sum += TBX_TIMING_DELAY(t1, t2);
        }

        printf("##############taille = %d - sum = %f\n", _size, sum / (param_nb_tests * param_nb_samples));


        //results.size            = _size;
        //results.bandwidth_mbit  =
        //    (_size * (double)param_nb_tests * (double)param_nb_samples)
        //    / (sum / (2 - param_one_way));
        //
        //results.bandwidth_mbyte = results.bandwidth_mbit / 1.048576;
        //results.latency         =
        //    sum / param_nb_tests / param_nb_samples / (2 - param_one_way);
        //
        //printf("%12d %12.3f %8.3f %8.3f\n",
        //       results.size,
        //       results.latency,
        //       results.bandwidth_mbit,
        //       results.bandwidth_mbyte);

#ifdef CHRONO
        printf("nb_boucles = %d\n", param_nb_tests * param_nb_samples);
        printf("temps_envoi     = %f\n", temps_envoi / param_nb_tests / param_nb_samples);
        printf("temps_reception = %f\n", temps_reception / param_nb_tests / param_nb_samples);
        temps_envoi = 0;
        temps_reception  = 0;

        extern double take_pre_posted_chrono;
        extern int nb_take_pre_posted;

        extern double schedule_ack_chrono;
        extern int nb_schedule_ack;

        extern double treat_unexpected_chrono;
        extern int nb_treat_unexpected;

        extern double so_schedule_in_time;
        extern int nb_so_sched_in;

        extern double open_received_time;
        extern int nb_open_received;

        extern double in_process_success_chrono;
        extern int nb_in_process_success;

        printf("\n");
        printf("  nb_take_pre_posted = %d\n", nb_take_pre_posted);
        printf("  take_pre_posted total -> %f\n", take_pre_posted_chrono);
        printf("  take_pre_posted -> %f\n", take_pre_posted_chrono / nb_take_pre_posted);
        if(nb_schedule_ack){
            printf("\n");
            printf("  nb_schedule_ack = %d\n", nb_schedule_ack);
            printf("  schedule_ack_chrono total-> %f\n", schedule_ack_chrono);
            printf("  schedule_ack_chrono -> %f\n", schedule_ack_chrono / nb_schedule_ack);
        }
        if(nb_treat_unexpected){
            printf("\n");
            printf("  treat_unexpected_nb = %d\n", nb_treat_unexpected);
            printf("  treat_unexpected_chrono total-> %f\n", treat_unexpected_chrono);
            printf("  treat_unexpected_chrono -> %f\n", treat_unexpected_chrono / nb_treat_unexpected);
        }
        printf("\n");
        printf("  nb_so_sched_in = %d\n", nb_so_sched_in);
        printf("  so_schedule_in_time total-> %f\n", so_schedule_in_time);
        printf("  so_schedule_in_time -> %f\n", so_schedule_in_time / nb_so_sched_in);
        printf("\n");
        printf("  nb_open_received = %d\n", nb_open_received);
        printf("  open_received_time total -> %f\n", open_received_time);
        printf("  open_received_time -> %f\n", open_received_time / nb_open_received);
        printf("\n");
        printf("  nb_in_process_success = %d\n", nb_in_process_success);
        printf("  in_process_success_chrono total -> %f\n", in_process_success_chrono);
        printf("  in_process_success_chrono -> %f\n", in_process_success_chrono / nb_in_process_success);

        extern int nb_success;
        extern double success_3_4;
        extern double success_4_5;
        extern double success_5_6;

        printf("\n");
        printf("    nb_success = %d\n", nb_success);
        printf("    open_received total -> %f\n",  success_3_4);
        printf("    open_received -> %f\n", success_3_4 / nb_success);
        printf("\n");
        printf("    append_list total -> %f\n",  success_4_5);
        printf("    append_list -> %f\n", success_4_5 / nb_success);

        printf("\n");
        printf("    release pre posted total -> %f\n",  success_5_6);
        printf("    release -> %f\n", success_5_6 / nb_success);


        extern int nb_release_pre_posted;
        extern double release_1_2;
        extern double release_2_3;
        extern double release_3_4;

        printf("\n");
        printf("      nb_release = %d\n", nb_release_pre_posted);
        printf("      init total -> %f\n",  release_1_2);
        printf("      init -> %f\n", release_1_2 / nb_release_pre_posted);
        //printf("\n");
        //printf("      memset total -> %f\n",  release_2_3);
        //printf("      init -> %f\n", release_2_3 / nb_release_pre_posted);
        printf("\n");
        printf("      heap_push total -> %f\n",  release_3_4);
        printf("      heap_push -> %f\n", release_3_4 / nb_release_pre_posted);



        take_pre_posted_chrono = 0;
        nb_take_pre_posted = 0;

        schedule_ack_chrono = 0;
        nb_schedule_ack = 0;

        treat_unexpected_chrono = 0;
        nb_treat_unexpected = 0;

        so_schedule_in_time = 0;
        nb_so_sched_in = 0;

        open_received_time = 0;
        nb_open_received = 0;

        in_process_success_chrono = 0;
        nb_in_process_success = 0;

        nb_success = 0;
        success_3_4 = 0;
        success_4_5 = 0;
        success_5_6 = 0;

        nb_release_pre_posted = 0;
        release_1_2 = 0;
        release_2_3 = 0;
        release_3_4 = 0;
#endif
    }
}

static
void
pong(void) {
    int size = 0;

    if (!param_dynamic_allocation && param_fill_buffer) {
        fill_buffer();
    }

    for (size = param_min_size;
         size <= param_max_size;
         size = param_step?size + param_step:size * 2) {
        const int _size = (!size && param_no_zero)?1:size;

        int nb_tests = param_nb_tests;

        while (nb_tests--) {
            int nb_samples = param_nb_samples;

            while (nb_samples--) {
                cond_alloc_main_buffer(_size);


                b_irecv(main_buffer, _size, 0);
                b_irecv(main_buffer, _size, 1);

                b_test_global();

                b_isend(main_buffer, _size, 0);
                b_isend(main_buffer, _size, 1);

                b_test_global();

                cond_free_main_buffer();

            }
        }
    }
}

int
session_init(int    argc,
             char **argv) {
    char	*r_url	= NULL;
    char	*l_url	= NULL;
    int err;

#if defined CONFIG_SCHED_MINI_ALT
    err = nm_core_init(&argc, argv, &p_core, nm_mini_alt_load);
#elif defined CONFIG_SCHED_OPT
    err = nm_core_init(&argc, argv, &p_core, nm_so_load);
#else
    err = nm_core_init(&argc, argv, &p_core, nm_mini_load);
#endif
    if (err != NM_ESUCCESS) {
        printf("nm_core_init returned err = %d\n", err);
        goto out;
    }

    argc--;
    argv++;

    if (argc) {
        r_url	= *argv;
        printf("running as client using remote url: [%s]\n", r_url);

        argc--;
        argv++;
    } else {
        printf("running as server\n");
    }

    err = nm_core_proto_init(p_core, nm_basic_load, &p_proto);
    if (err != NM_ESUCCESS) {
        printf("nm_core_proto_init returned err = %d\n", err);
        goto out;
    }

#if defined CONFIG_MX
    err = nm_core_driver_load_init(p_core, nm_mx_load, &drv_id, &l_url);
#elif defined CONFIG_GM
    err = nm_core_driver_load_init(p_core, nm_gm_load, &drv_id, &l_url);
#elif defined CONFIG_TCP
    err = nm_core_driver_load_init(p_core, nm_tcpdg_load, &drv_id, &l_url);
#endif
    if (err != NM_ESUCCESS) {
        printf("nm_core_driver_load_init returned err = %d\n", err);
        goto out;
    }

    err = nm_core_gate_init(p_core, &gate_id);
    if (err != NM_ESUCCESS) {
        printf("nm_core_gate_init returned err = %d\n", err);
        goto out;
    }

    if (!r_url) {
        /* server
         */
        printf("local url: [%s]\n", l_url);

        err = nm_core_gate_accept(p_core, gate_id, drv_id, NULL);
        if (err != NM_ESUCCESS) {
            printf("nm_core_gate_accept returned err = %d\n", err);
            goto out;
        }
    } else {
        /* client
         */
        err = nm_core_gate_connect(p_core, gate_id, drv_id, r_url);
        if (err != NM_ESUCCESS) {
            printf("nm_core_gate_connect returned err = %d\n", err);
            goto out;
        }
    }

    return !!r_url;

 out:
    exit(EXIT_FAILURE);
}

int
main(int    argc,
     char **argv) {
    int is_master;

    is_master	= session_init(argc, argv);
    if (!param_dynamic_allocation) {
        main_buffer = tbx_aligned_malloc(param_max_size, 8);
    }

    if (is_master) {
        ping();
    } else {
        pong();
    }

    if (!param_dynamic_allocation) {
        tbx_aligned_free(main_buffer, 8);
    }

    printf("Exiting\n");

    return 0;
}
