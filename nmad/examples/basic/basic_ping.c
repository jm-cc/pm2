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
 * basic_ping.c
 * ============
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

/* Choose a scheduler
 */
#include <nm_public.h>

#include "basic_common.h"

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
#if defined CONFIG_IBVERBS
#  include <nm_ibverbs_public.h>
#elif defined CONFIG_MX
#  include <nm_mx_public.h>
#elif defined CONFIG_GM
#  include <nm_gm_public.h>
#elif defined CONFIG_QSNET
#  include <nm_qsnet_public.h>
#elif defined CONFIG_SISCI
#  include <nm_sisci_public.h>
#else
#  include <nm_tcp_public.h>
#endif

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
static const int param_test_warmup       = 1;
static const int param_nb_samples        = 2;
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
static gate_id_t                 gate_id	=    0;

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

        err	= nm_basic_irecv(p_proto, 0, 0, buf, len, &p_rq);
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
                        int                _nb_tests  = param_nb_samples;

                        if (param_test_warmup) {
                                while (_nb_tests--) {
                                        cond_alloc_main_buffer(_size);
                                        b_send(main_buffer, _size);
                                        cond_free_main_buffer();
                                }
                        }

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
                        int nb_tests	= param_nb_tests+param_test_warmup;
                        int flag	= !param_test_warmup;

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

                                if (flag) {
                                        sum += TBX_TIMING_DELAY(t1, t2);
                                } else {
                                        flag = 1;
                                }

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
                                (param_nb_tests+param_test_warmup) * param_nb_samples;
                        int                dummy      = 0;

                        while (nb_tests--) {
                                cond_alloc_main_buffer(_size);
                                b_recv(main_buffer, _size);
                                cond_free_main_buffer();
                        }

                        b_send(&dummy, sizeof(dummy));
                } else {
                        int nb_tests = param_nb_tests+param_test_warmup;

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
main(int    argc,
     char **argv) {
        int is_master;
        is_master	= !bc_core_init(&argc, argv, &p_core, &p_proto, &gate_id);

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
