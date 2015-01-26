/* -*- Mode: C; c-basic-offset:4 ; -*- */
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

#include "piom_private.h"

#include <errno.h>

TBX_INTERNAL struct piom_parameters_s piom_parameters =
    {
	.busy_wait_usec     = 10,
	.busy_wait_granularity = 100,
	.enable_progression = 1,
	.idle_granularity   = 20,
	.idle_level         = PIOM_TOPO_SOCKET,
	.timer_period       = 4000,
	.spare_lwp          = 0
    };

static int piom_init_done = 0;


void pioman_init(int*argc, char**argv)
{
    piom_init_done++;
    if(piom_init_done > 1)
	return;

#ifdef MCKERNEL
    piom_parameters.timer_period = -1;
    piom_parameters.idle_granularity = -1;
    piom_parameters.busy_wait_usec = 1000 * 1000 * 1000;
#endif /* MCKERNEL */
    const char*s_busy_wait_usec        = getenv("PIOM_BUSY_WAIT_USEC");
    const char*s_busy_wait_granularity = getenv("PIOM_BUSY_WAIT_GRANULARITY");
    const char*s_enable_progression    = getenv("PIOM_ENABLE_PROGRESSION");
    const char*s_idle_granularity      = getenv("PIOM_IDLE_GRANULARITY");
    const char*s_idle_level            = getenv("PIOM_IDLE_LEVEL");
    const char*s_timer_period          = getenv("PIOM_TIMER_PERIOD");
    const char*s_spare_lwp             = getenv("PIOM_SPARE_LWP");
    if(s_busy_wait_usec)
	{
	    piom_parameters.busy_wait_usec = atoi(s_busy_wait_usec);
	    fprintf(stderr, "# pioman: custom PIOM_BUSY_WAIT_USEC = %d\n", piom_parameters.busy_wait_usec);
	}
    if(s_busy_wait_granularity)
	{
	    piom_parameters.busy_wait_granularity = atoi(s_busy_wait_granularity);
	    fprintf(stderr, "# pioman: custom PIOM_BUSY_WAIT_GRANULARITY = %d\n", piom_parameters.busy_wait_granularity);
	}
    if(s_enable_progression)
	{
	    piom_parameters.enable_progression = atoi(s_enable_progression);
	    fprintf(stderr, "# pioman: custom PIOM_ENABLE_PROGRESSION = %d\n", piom_parameters.enable_progression);
	}
    if(s_idle_granularity)
	{
	    piom_parameters.idle_granularity = atoi(s_idle_granularity);
	    fprintf(stderr, "# pioman: custom PIOM_IDLE_GRANULARITY = %d\n", piom_parameters.idle_granularity);
	}
    if(s_idle_level)
	{
	    enum piom_topo_level_e level = 
		(strcmp(s_idle_level, "machine") == 0) ? PIOM_TOPO_MACHINE :
		(strcmp(s_idle_level, "node")    == 0) ? PIOM_TOPO_NODE :
		(strcmp(s_idle_level, "socket")  == 0) ? PIOM_TOPO_SOCKET :
		(strcmp(s_idle_level, "core")    == 0) ? PIOM_TOPO_CORE :
		(strcmp(s_idle_level, "pu")      == 0) ? PIOM_TOPO_PU :
		PIOM_TOPO_NONE;
	    if(level != PIOM_TOPO_NONE)
		{
		    piom_parameters.idle_level = level;
		    fprintf(stderr, "# pioman: custom PIOM_IDLE_LEVEL = %s (%d)\n", s_idle_level, piom_parameters.idle_level);
		}
	}
    if(s_timer_period)
	{
	    piom_parameters.timer_period = atoi(s_timer_period);
	    fprintf(stderr, "# pioman: custom PIOM_TIMER_PERIOD = %d\n", piom_parameters.timer_period);
	}
    if(piom_parameters.enable_progression == 0)
	{
	    piom_parameters.timer_period = -1;
	    piom_parameters.idle_granularity = -1;
	    piom_parameters.busy_wait_usec = -1;
	}
    if(s_spare_lwp)
	{
	    piom_parameters.spare_lwp = atoi(s_spare_lwp);
	    fprintf(stderr, "# pioman: custom PIOM_SPARE_LWP = %d\n", piom_parameters.spare_lwp);
	}

#ifdef PIOMAN_MARCEL
    marcel_init(argc, argv);
#endif /* PIOMAN_MARCEL */
    piom_init_ltasks();
    /*    piom_io_task_init(); */
}

void pioman_exit(void)
{
    assert(piom_init_done);
    piom_init_done--;
    if(piom_init_done == 0)
	{
	    /*
	      piom_io_task_stop();
	    */
	    piom_exit_ltasks();
	}
}

/** Polling point. May be called from the application to force polling,
 * from marcel hooks, from timer handler.
 * @return 0 if we didn't need to poll and 1 otherwise
 */
int piom_check_polling(unsigned polling_point)
{
    piom_ltask_schedule();
    return 1;
}
