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
	.binding_level      = PIOM_TOPO_SOCKET,
	.idle_granularity   = 5,
	.idle_distrib       = PIOM_BIND_DISTRIB_ALL,
	.timer_period       = 4000,
	.spare_lwp          = 0,
	.mckernel           = 0
    };

static int piom_init_done = 0;

#ifdef PIOMAN_MARCEL
/** Polling point. May be called from the application to force polling,
 * from marcel hooks, from timer handler.
 * @return 0 if we didn't need to poll and 1 otherwise
 */
static int piom_polling_hook(unsigned _point)
{
    const int point =
	(_point == MARCEL_SCHEDULING_POINT_TIMER) ? PIOM_POLL_POINT_TIMER :
	(_point == MARCEL_SCHEDULING_POINT_IDLE)  ? PIOM_POLL_POINT_IDLE :
	PIOM_POLL_POINT_FORCED;
    piom_ltask_schedule(point);
    return 1;
}
#endif /* PIOMAN_MARCEL */

void piom_polling_force(void)
{
    piom_ltask_schedule(PIOM_POLL_POINT_FORCED);
}

void pioman_init(int*argc, char**argv)
{
    piom_init_done++;
    if(piom_init_done > 1)
	return;
    if(getenv("PADICO_QUIET") == NULL)
       setenv("PIOM_VERBOSE", "1", 0);

    struct stat proc_mckernel;
    int rc = stat("/proc/mckernel", &proc_mckernel);
    if(rc == 0)
	{
	    piom_parameters.mckernel = 1;
	    PIOM_DISP("running on mckernel\n");
	}

#ifdef MCKERNEL
    piom_parameters.timer_period = -1;
    piom_parameters.idle_granularity = -1;
    piom_parameters.busy_wait_usec = 1000 * 1000 * 1000;
#endif /* MCKERNEL */
    const char*s_busy_wait_usec        = getenv("PIOM_BUSY_WAIT_USEC");
    const char*s_busy_wait_granularity = getenv("PIOM_BUSY_WAIT_GRANULARITY");
    const char*s_enable_progression    = getenv("PIOM_ENABLE_PROGRESSION");
    const char*s_binding_level         = getenv("PIOM_BINDING_LEVEL");
    const char*s_idle_granularity      = getenv("PIOM_IDLE_GRANULARITY");
    const char*s_idle_distrib          = getenv("PIOM_IDLE_DISTRIB");
    const char*s_timer_period          = getenv("PIOM_TIMER_PERIOD");
    const char*s_spare_lwp             = getenv("PIOM_SPARE_LWP");
    if(s_busy_wait_usec)
	{
	    piom_parameters.busy_wait_usec = atoi(s_busy_wait_usec);
	    PIOM_DISP("custom PIOM_BUSY_WAIT_USEC = %d\n", piom_parameters.busy_wait_usec);
	}
    if(s_busy_wait_granularity)
	{
	    piom_parameters.busy_wait_granularity = atoi(s_busy_wait_granularity);
	    PIOM_DISP("custom PIOM_BUSY_WAIT_GRANULARITY = %d\n", piom_parameters.busy_wait_granularity);
	}
    if(s_enable_progression)
	{
	    piom_parameters.enable_progression = atoi(s_enable_progression);
	    PIOM_DISP("custom PIOM_ENABLE_PROGRESSION = %d\n", piom_parameters.enable_progression);
	}
    if(s_idle_granularity)
	{
	    piom_parameters.idle_granularity = atoi(s_idle_granularity);
	    PIOM_DISP("custom PIOM_IDLE_GRANULARITY = %d\n", piom_parameters.idle_granularity);
	}
    if(s_binding_level)
	{
	    enum piom_topo_level_e level = 
		(strcmp(s_binding_level, "machine") == 0) ? PIOM_TOPO_MACHINE :
		(strcmp(s_binding_level, "node")    == 0) ? PIOM_TOPO_NODE :
		(strcmp(s_binding_level, "socket")  == 0) ? PIOM_TOPO_SOCKET :
		(strcmp(s_binding_level, "core")    == 0) ? PIOM_TOPO_CORE :
		(strcmp(s_binding_level, "pu")      == 0) ? PIOM_TOPO_PU :
		PIOM_TOPO_NONE;
	    if(level != PIOM_TOPO_NONE)
		{
		    piom_parameters.binding_level = level;
		    PIOM_DISP("custom PIOM_BINDING_LEVEL = %s (%d)\n", s_binding_level, piom_parameters.binding_level);
		}
	}
    if(s_idle_distrib)
	{
	    enum piom_bind_distrib_e distrib =
		(strcmp(s_idle_distrib, "all")   == 0) ? PIOM_BIND_DISTRIB_ALL :
		(strcmp(s_idle_distrib, "odd")   == 0) ? PIOM_BIND_DISTRIB_ODD :
		(strcmp(s_idle_distrib, "even")  == 0) ? PIOM_BIND_DISTRIB_EVEN :
		(strcmp(s_idle_distrib, "first") == 0) ? PIOM_BIND_DISTRIB_FIRST :
		PIOM_BIND_DISTRIB_NONE;
	    if(distrib != PIOM_BIND_DISTRIB_NONE)
		{
		    piom_parameters.idle_distrib = distrib;
		    PIOM_DISP("custom PIOM_IDLE_DISTRIB = %s\n", s_idle_distrib);
		}
	}
    if(s_timer_period)
	{
	    piom_parameters.timer_period = atoi(s_timer_period);
	    PIOM_DISP("custom PIOM_TIMER_PERIOD = %d\n", piom_parameters.timer_period);
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
	    PIOM_DISP("custom PIOM_SPARE_LWP = %d\n", piom_parameters.spare_lwp);
	}

#ifdef PIOMAN_MARCEL
    marcel_init(argc, argv);
#endif /* PIOMAN_MARCEL */
    piom_init_ltasks();
    /*    piom_io_task_init(); */
#ifdef PIOMAN_MARCEL
    /* marcel: register hooks */
    marcel_register_scheduling_hook(piom_polling_hook, MARCEL_SCHEDULING_POINT_TIMER);
    marcel_register_scheduling_hook(piom_polling_hook, MARCEL_SCHEDULING_POINT_YIELD);
    marcel_register_scheduling_hook(piom_polling_hook, MARCEL_SCHEDULING_POINT_IDLE);
    marcel_register_scheduling_hook(piom_polling_hook, MARCEL_SCHEDULING_POINT_LIB_ENTRY);
    marcel_register_scheduling_hook(piom_polling_hook, MARCEL_SCHEDULING_POINT_CTX_SWITCH);
#endif /* PIOMAN_MARCEL */
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

