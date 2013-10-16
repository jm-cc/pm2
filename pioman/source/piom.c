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
	.busy_wait_usec     = 5,
	.busy_wait_granularity = 100,
	.enable_progression = 1,
	.idle_granularity   = 5,
	.timer_period       = 4000,
	.spare_lwp          = 0
    };

static int piom_init_done = 0;

#ifdef PIOMAN_TRACE
static void pioman_trace_init(void) __attribute__((constructor,used));
static void pioman_trace_exit(void) __attribute__((destructor,used));
static void pioman_trace_init(void)
{
    char trace_name[256]; 
    char hostname[256];
    gethostname(hostname, 256);
    sprintf(trace_name, "piom_%s", hostname);
    fprintf(stderr, "# pioman trace init: %s\n", trace_name);
    setTraceType(PAJE);
    initTrace(trace_name, 0, GTG_FLAG_NONE);

    /* container types */
    addContType("Container_Machine", "0",                "Machine");
    addContType("Container_Node",   "Container_Machine", "Node");
    addContType("Container_Socket", "Container_Node",    "Socket");
    addContType("Container_Core",   "Container_Socket",  "Core");
    
    addStateType ("State_Machine", "Container_Machine", "Machine state");
    addStateType ("State_Node",    "Container_Node",    "Node state");
    addStateType ("State_Socket",  "Container_Socket",  "Socket state");
    addStateType ("State_Core",    "Container_Core",    "Core state");

    addEntityValue ("State_Core_none", "State_Core", "State_Core_none", GTG_BLACK);
    addEntityValue ("State_Core_init", "State_Core", "State_Core_init", GTG_BLUE);
    addEntityValue ("State_Core_poll", "State_Core", "State_Core_poll", GTG_PINK);

    addEntityValue ("State_Socket_none", "State_Socket", "State_Socket_none", GTG_BLACK);
    addEntityValue ("State_Socket_init", "State_Socket", "State_Socket_init", GTG_BLUE);
    addEntityValue ("State_Socket_poll", "State_Socket", "State_Socket_poll", GTG_PINK);

    addEntityValue ("State_Node_none", "State_Node", "State_Node_none", GTG_BLACK);
    addEntityValue ("State_Node_init", "State_Node", "State_Node_init", GTG_BLUE);
    addEntityValue ("State_Node_poll", "State_Node", "State_Node_poll", GTG_PINK);

    addEntityValue ("State_Machine_none", "State_Machine", "State_Machine_none", GTG_BLACK);
    addEntityValue ("State_Machine_init", "State_Machine", "State_Machine_init", GTG_BLUE);
    addEntityValue ("State_Machine_poll", "State_Machine", "State_Machine_poll", GTG_PINK);
 
    addEventType ("Event_Machine", "Container_Machine", "Event_Machine_submit");
    addEventType ("Event_Node",    "Container_Node",    "Event_Node_submit");
    addEventType ("Event_Socket",  "Container_Socket",  "Event_Socket_submit");
    addEventType ("Event_Core",    "Container_Core",    "Event_Core_submit");
}
static void pioman_trace_exit(void)
{
    fprintf(stderr, "# pioman trace exit.\n");
    endTrace();
}
#endif /* PIOMAN_TRACE */

void pioman_init(int*argc, char**argv)
{
    piom_init_done++;
    if(piom_init_done > 1)
	return;

#ifdef __MIC__
    piom_parameters.timer_period = -1;
    piom_parameters.idle_granularity = -1;
    piom_parameters.busy_wait_usec = 1000 * 1000 * 1000;
#endif
    const char*s_busy_wait_usec        = getenv("PIOM_BUSY_WAIT_USEC");
    const char*s_busy_wait_granularity = getenv("PIOM_BUSY_WAIT_GRANULARITY");
    const char*s_enable_progression    = getenv("PIOM_ENABLE_PROGRESSION");
    const char*s_idle_granularity      = getenv("PIOM_IDLE_GRANULARITY");
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
    if(s_timer_period)
	{
	    piom_parameters.timer_period = atoi(s_timer_period);
	    fprintf(stderr, "# pioman: custom PIOM_TIMER_PERIOD = %d\n", piom_parameters.timer_period);
	}
    if(piom_parameters.enable_progression == 0)
	{
	    piom_parameters.timer_period = -1;
	    piom_parameters.idle_granularity = -1;
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
    int ret = 0;
    void*task = piom_ltask_schedule();
    ret = (task != NULL);
    return ret;
}
