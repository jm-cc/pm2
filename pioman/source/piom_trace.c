/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2013-2015 "the PM2 team" (see AUTHORS file)
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

#ifdef PIOMAN_TRACE

#define PIOMAN_TRACE_MAX (1024*1024*8)

struct piom_trace_entry_s
{
    tbx_tick_t tick;
    enum piom_trace_event_e event;
    const struct piom_trace_info_s*trace_info;
    void*value;
};

static struct piom_trace_s
{
    tbx_tick_t orig;  /**< time origin*/
    int last;         /**< index of last entry recorded */
    struct piom_trace_entry_s entries[PIOMAN_TRACE_MAX];
    struct piom_trace_info_s**containers;
    int ncontainers;
} __piom_trace = { .last = 0, .containers = NULL, .ncontainers = 0};

static void pioman_trace_init(void) __attribute__((constructor,used));
static void pioman_trace_exit(void) __attribute__((destructor,used));
static void pioman_trace_init(void)
{
    TBX_GET_TICK(__piom_trace.orig);
}
static inline struct piom_trace_entry_s*piom_trace_get_entry(void)
{
    const int i = __sync_fetch_and_add(&__piom_trace.last, 1);
    if(i >= PIOMAN_TRACE_MAX)
	return NULL;
    return &__piom_trace.entries[i];
}

void piom_trace_local_new(struct piom_trace_info_s*trace_info)
{
    const int i = __piom_trace.ncontainers;
    __piom_trace.ncontainers++;
    __piom_trace.containers = realloc(__piom_trace.containers, __piom_trace.ncontainers * sizeof(struct piom_trace_info_s*));
    __piom_trace.containers[i] = trace_info;
}

void piom_trace_remote_event(piom_topo_obj_t obj, enum piom_trace_event_e _event, void*_value)
{
    const struct piom_ltask_locality_s*local = obj->userdata;
    assert(local != NULL);
    struct piom_trace_entry_s*entry = piom_trace_get_entry();
    if(entry)
	{
	    TBX_GET_TICK(entry->tick);
	    entry->event = _event;
	    entry->trace_info = &local->trace_info;
	    entry->value = _value;
	}
}

void piom_trace_local_event(enum piom_trace_event_e _event, void*_value)
{
    piom_topo_obj_t obj = piom_topo_current_obj();
    piom_trace_remote_event(obj, _event, _value);
}

void piom_trace_flush(void)
{
    static int flushed = 0;
    if(flushed)
	return;
    fprintf(stderr, "# pioman: flush traces (%d entries)\n", __piom_trace.last);
    flushed = 1;

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
 
    addEventType ("Event_Machine_submit", "Container_Machine", "Event: Machine ltask submit");
    addEventType ("Event_Node_submit",    "Container_Node",    "Event: Node ltask submit");
    addEventType ("Event_Socket_submit",  "Container_Socket",  "Event: Socket ltask submit");
    addEventType ("Event_Core_submit",    "Container_Core",    "Event: Core ltask submit");

    addEventType ("Event_Machine_success", "Container_Machine", "Event: Machine ltask success");
    addEventType ("Event_Node_success",    "Container_Node",    "Event: Node ltask success");
    addEventType ("Event_Socket_success",  "Container_Socket",  "Event: Socket ltask success");
    addEventType ("Event_Core_success",    "Container_Core",    "Event: Core ltask success");

    addVarType("Tasks_Machine", "ltasks in Machine queue", "Container_Machine");
    addVarType("Tasks_Node",    "ltasks in Node queue",    "Container_Node");
    addVarType("Tasks_Socket",  "ltasks in Socket queue",  "Container_Socket");
    addVarType("Tasks_Core",    "ltasks in Core queue",    "Container_Core");

    addContType("Container_Global", "0", "Global");
    addContainer(0.00000, "Timer_Events", "Container_Global", "0", "Timer_Events", "0");
    addContainer(0.00000, "Idle_Events", "Container_Global", "0", "Idle_Events", "0");
    addEventType ("Event_Timer_Poll", "Container_Global", "Event: timer poll");
    addEventType ("Event_Idle_Poll", "Container_Global", "Event: idle poll");
    int i;
    for(i = 0; i < __piom_trace.ncontainers; i++)
	{
	    const struct piom_trace_info_s*p_trace_info = __piom_trace.containers[i];
	    addContainer(0.00000, p_trace_info->cont_name, p_trace_info->cont_type, 
			 p_trace_info->parent ? p_trace_info->parent->cont_name:"0", p_trace_info->cont_name, "0");
	}
    for(i = 0; i < ((__piom_trace.last > PIOMAN_TRACE_MAX) ? PIOMAN_TRACE_MAX : __piom_trace.last) ; i++)
	{
	    const struct piom_trace_entry_s*e = &__piom_trace.entries[i];
	    const double d = TBX_TIMING_DELAY(__piom_trace.orig, e->tick) / 1000000.0;
	    const char*level_label = NULL;
	    struct piom_trace_info_s trace_info = e->trace_info ? *e->trace_info : (struct piom_trace_info_s){ 0 };
	    switch(trace_info.level)
		{
		case PIOM_TOPO_MACHINE:
		    level_label = "Machine";
		    break;
		case PIOM_TOPO_NODE:
		    level_label = "Node";
		    break;
		case PIOM_TOPO_SOCKET:
		    level_label = "Socket";
		    break;
		case PIOM_TOPO_CORE:
		    level_label = "Core";
		    break;
		default:
		    level_label = "(unkown)";
		    break;
		}
	    char cont_name[64];
	    char state_type[128];
	    char event_type[128];
	    char var_type[128];
	    char value[256];
	    sprintf(cont_name, "%s_%d", level_label, trace_info.rank);
	    sprintf(state_type, "State_%s", level_label);
	    switch(e->event)
		{
		case PIOM_TRACE_EVENT_TIMER_POLL:
		    addEvent(d, "Event_Timer_Poll", "Timer_Events", NULL);
		    break;
		case PIOM_TRACE_EVENT_IDLE_POLL:
		    addEvent(d, "Event_Idle_Poll", "Idle_Events", NULL);
		    break;
		case PIOM_TRACE_EVENT_SUBMIT:
		    sprintf(event_type, "Event_%s_submit", level_label);
		    sprintf(value, "ltask = %p", e->value);
		    addEvent(d, event_type, cont_name, value);
		    break;
		case PIOM_TRACE_EVENT_SUCCESS:
		    sprintf(event_type, "Event_%s_success", level_label);
		    sprintf(value, "ltask = %p", e->value);
		    addEvent(d, event_type, cont_name, value);
		    break;
		case PIOM_TRACE_STATE_INIT:
		    sprintf(value, "%s_init", state_type);
		    setState(d, state_type, cont_name, value);
		    break;
		case PIOM_TRACE_STATE_POLL:
		    sprintf(value, "%s_poll", state_type);
		    setState(d, state_type, cont_name, value);
		    break;
		case PIOM_TRACE_STATE_NONE:
		    sprintf(value, "%s_none", state_type);
		    setState(d, state_type, cont_name, value);
		    break;
		case PIOM_TRACE_VAR_LTASKS:
		    {
			int _var = (uintptr_t)e->value;
			sprintf(var_type, "Tasks_%s", level_label);
			setVar(d, var_type, cont_name, _var);
		    }
		    break;
		default:
		    break;
		}
	}
    endTrace();
    fprintf(stderr, "# pioman: traces flushed.\n");
}
static void pioman_trace_exit(void)
{
    piom_trace_flush();
}
#endif /* PIOMAN_TRACE */
