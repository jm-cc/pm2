/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2015-2016 "the PM2 team" (see AUTHORS file)
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

#include "piom_private.h"

#ifdef PIOMAN_ABT

extern int __abt_app_main(int argc, char**argv);

static int __abt_argc = -1;
static char**__abt_argv = NULL;

static void __piom_abt_main(void*dummy)
{
    __abt_app_main(__abt_argc, __abt_argv);
}

extern int main(int argc, char**argv)
{
    fprintf(stderr, "# pioman: ABT main- starting.\n");
    if(!tbx_initialized())
	{
	    fprintf(stderr, "# pioman: ABT main- init tbx.\n");
	    tbx_init(&argc, &argv);
	}
    fprintf(stderr, "# pioman: ABT main- init.\n");
    ABT_init(argc, argv);
    ABT_xstream main_xstream;
    ABT_xstream_create(ABT_SCHED_NULL, &main_xstream);
    ABT_pool main_pool;
    ABT_xstream_get_main_pools(main_xstream, 1, &main_pool);
    ABT_thread main_thread;
    __abt_argc = argc;
    __abt_argv = argv;
    ABT_thread_create_on_xstream(main_xstream, &__piom_abt_main, NULL, ABT_THREAD_ATTR_NULL, &main_thread);
    ABT_thread_join(main_thread);
    fprintf(stderr, "# pioman: ABT main exited.\n");
    ABT_thread_free(&main_thread);
    ABT_xstream_join(main_xstream);
    ABT_xstream_free(main_xstream);
    ABT_finalize();
    return 0;
}

static void __piom_abt_worker(void*_dummy)
{
    piom_ltask_queue_t*queue = piom_topo_get_queue(piom_topo_full);
    int rank;
    ABT_xstream_self_rank(&rank);
    while(queue->state != PIOM_LTASK_QUEUE_STATE_STOPPED)
	{
	    usleep(1000);
	    piom_ltask_schedule(PIOM_POLL_POINT_IDLE);
	    ABT_thread_yield();
	}
}

void piom_abt_init_ltasks(void)
{
    /* ** ABT init */
    fprintf(stderr, "# piom_init: init ABT\n");
    if(ABT_initialized() != ABT_SUCCESS)
	{
	    abort();
	}
    ABT_xstream worker_xstream;
    ABT_xstream_create(ABT_SCHED_NULL, &worker_xstream);
    ABT_thread worker_thread;
    ABT_thread_create_on_xstream(worker_xstream, &__piom_abt_worker, NULL, ABT_THREAD_ATTR_NULL, &worker_thread);
    int rank;
    ABT_xstream_self_rank(&rank);
    fprintf(stderr, "# pioman ABT: init done; rank = %d.\n", rank);
}

#endif /* PIOMAN_ABT */
