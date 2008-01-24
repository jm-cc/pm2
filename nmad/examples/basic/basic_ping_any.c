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
 * basic_ping_any.c
 * ================
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
#else
#  error default scheduler 'sched_mini' does not support application level gateless requests
#endif

/* Choose an API
 */
#include <nm_basic_public.h>

/* Choose a driver
 */
#include <nm_drivers.h>

#include <tbx.h>

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
static const int param_warmup            = 1;
static const int param_nb_samples        = 1000;
static const int param_min_size          = 4;
static const int param_max_size          = 1024*1024*2;
static const int param_step              = 0; /* 0 = progression log. */
static const int param_nb_tests          = 5;
static const int param_no_zero           = 1;
static const int param_fill_buffer       = 1;
static const int param_fill_buffer_value = 1;
static const int param_one_way           = 0;
static const int param_dynamic_allocation	= 0;

static unsigned char		*main_buffer	= NULL;

static struct nm_core		*p_core		= NULL;
static struct nm_proto		*p_proto	= NULL;
static struct nm_basic_rq	*p_rq		= NULL;
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
                printf("ok");
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
b_send(void		*buf,
        uint64_t	 len) {
        int err;

        err	= nm_basic_isend(p_proto, 0, 0, buf, len, &p_rq);
        if (err != NM_ESUCCESS) {
                printf("nm_basic_isend returned err = %d\n", err);
                goto out;
        }

        err = nm_basic_wait(p_rq);
        if (err != NM_ESUCCESS) {
                printf("nm_schedule returned err = %d\n", err);
                goto out;
        }

        return;

 out:
        exit(EXIT_FAILURE);
}

static __inline__
void
b_recv(void		*buf,
        uint64_t	 len) {
        int err;

        err	= nm_basic_irecv_any(p_proto, 0, buf, len, &p_rq);
        if (err != NM_ESUCCESS) {
                printf("nm_basic_isend returned err = %d\n", err);
                goto out;
        }

        err = nm_basic_wait(p_rq);
        if (err != NM_ESUCCESS) {
                printf("nm_schedule returned err = %d\n", err);
                goto out;
        }

        return;

 out:
        exit(EXIT_FAILURE);
}

static
void
ping(void) {
        int size = 0;

        if (!param_dynamic_allocation && param_fill_buffer) {
                fill_buffer();
        }

        if (param_warmup) {
                cond_alloc_main_buffer(param_max_size);
                b_send(main_buffer, param_max_size);
                cond_free_main_buffer();

                cond_alloc_main_buffer(param_max_size);
                b_recv(main_buffer, param_max_size);
                cond_free_main_buffer();
        }

        for (size = param_min_size;
             size <= param_max_size;
             size = param_step?size + param_step:size * 2) {
                const int       _size   = (!size && param_no_zero)?1:size;
                int             dummy   = 0;
                ping_result_t	results = { 0, 0.0, 0.0, 0.0 };
                double          sum     = 0.0;
                tbx_tick_t      t1;
                tbx_tick_t      t2;

                sum = 0;

                if (param_one_way) {
                        int                nb_tests   = param_nb_tests * param_nb_samples;

                        TBX_GET_TICK(t1);
                        while (nb_tests--) {
                                cond_alloc_main_buffer(_size);
                                b_send(main_buffer, _size);
                                cond_free_main_buffer();
                        }

                        b_recv(&dummy, sizeof(dummy));

                        TBX_GET_TICK(t2);

                        sum += TBX_TIMING_DELAY(t1, t2);
                } else {
                        int nb_tests = param_nb_tests;

                        while (nb_tests--) {
                                int nb_samples = param_nb_samples;

                                TBX_GET_TICK(t1);
                                while (nb_samples--) {
                                        cond_alloc_main_buffer(_size);

                                        if (param_control_receive) {
                                                mark_buffer(_size);
                                        }

                                        b_send(main_buffer, _size);

                                        if (param_control_receive) {
                                                clear_buffer();
                                        }

                                        b_recv(main_buffer, _size);

                                        if (param_control_receive) {
                                                control_buffer(_size);
                                        }
                                        cond_free_main_buffer();
                                }
                                TBX_GET_TICK(t2);

                                sum += TBX_TIMING_DELAY(t1, t2);
                        }
                }

                results.size            = _size;
                results.bandwidth_mbit  =
                        (_size * (double)param_nb_tests * (double)param_nb_samples)
                        / (sum / (2 - param_one_way));

                results.bandwidth_mbyte = results.bandwidth_mbit / 1.048576;
                results.latency         =
                        sum / param_nb_tests / param_nb_samples / (2 - param_one_way);

                printf("%12d %12.3f %8.3f %8.3f\n",
                      results.size,
                      results.latency,
                      results.bandwidth_mbit,
                      results.bandwidth_mbyte);
        }
}

static
void
pong(void) {
        int size = 0;

        if (!param_dynamic_allocation && param_fill_buffer) {
                fill_buffer();
        }

        if (param_warmup) {
                cond_alloc_main_buffer(param_max_size);
                b_recv(main_buffer, param_max_size);
                cond_free_main_buffer();

                cond_alloc_main_buffer(param_max_size);
                b_send(main_buffer, param_max_size);
                cond_free_main_buffer();
        }

        for (size = param_min_size;
             size <= param_max_size;
             size = param_step?size + param_step:size * 2) {
                const int _size = (!size && param_no_zero)?1:size;

                if (param_one_way) {
                        int                nb_tests   =
                                param_nb_tests * param_nb_samples;
                        int                dummy      = 0;

                        while (nb_tests--) {
                                cond_alloc_main_buffer(_size);
                                b_recv(main_buffer, _size);
                                cond_free_main_buffer();
                        }

                        b_send(&dummy, sizeof(dummy));
                } else {
                        int nb_tests = param_nb_tests;

                        while (nb_tests--) {
                                int nb_samples = param_nb_samples;

                                while (nb_samples--) {
                                        cond_alloc_main_buffer(_size);
                                        b_recv(main_buffer, _size);
                                        b_send(main_buffer, _size);
                                        cond_free_main_buffer();
                                }
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
        err = nm_core_driver_load_init(p_core, nm_tcp_load, &drv_id, &l_url);
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
