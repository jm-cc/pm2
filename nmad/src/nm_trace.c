/*
 * NewMadeleine
 * Copyright (C) 2014-2015 (see AUTHORS file)
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

#include <nm_private.h>

#ifdef NMAD_TRACE

#include <stdio.h>
#include <GTG.h>
#include <tbx.h>

/** maximum number of events */
#define NM_TRACE_MAX (1024*1024*8)

#define CHECK_RETURN(val) { if (val!=TRACE_SUCCESS){NM_FATAL("trace: function failed line %d.\n", __LINE__);} }

static void nm_trace_exit(void) __attribute__((destructor));
static void nm_trace_init(void) __attribute__((constructor));

/** a trace event */
struct nm_trace_entry_s
{
  tbx_tick_t tick;         /**< absolute time of event */
  nm_trace_scope_t scope;  /**< scope of trace event */
  nm_trace_event_t event;
  void*value;
  nm_gate_t p_gate;
};

static struct
{
  tbx_tick_t orig;
  int last;
  struct nm_trace_entry_s entries[NM_TRACE_MAX];
} __nm_trace = { .last = 0 };

static struct nm_trace_entry_s*nm_trace_get_entry(void)
{
  const int i = __sync_fetch_and_add(&__nm_trace.last, 1);
  if(i >= NM_TRACE_MAX)
    return NULL;
  return &__nm_trace.entries[i];
}


void nm_trace_event(nm_trace_scope_t scope, nm_trace_event_t event, void*value, nm_gate_t p_gate)
{
  struct nm_trace_entry_s*p_entry = nm_trace_get_entry();
  if(p_entry)
    {
      TBX_GET_TICK(p_entry->tick);
      p_entry->event  = event;
      p_entry->scope  = scope;
      p_entry->value  = value;
      p_entry->p_gate = p_gate;
    }
}

void nm_trace_state(nm_trace_scope_t scope, nm_trace_event_t event, nm_gate_t p_gate)
{
  nm_trace_event(scope, event, NULL, p_gate);
}

void nm_trace_var(nm_trace_scope_t scope, nm_trace_event_t event, int _value, nm_gate_t p_gate)
{
  void*value = (void*)((uintptr_t)_value);
  nm_trace_event(scope, event, value, p_gate);
}

static void nm_trace_flush(void)
{
  static int flushed = 0;
  if(flushed)
    return;
  fprintf(stderr, "# nmad: flush traces (%d entries)\n", __nm_trace.last);
  flushed = 1;

  /* init GTG */
  char trace_name[256];
  char hostname[256];
  gethostname(hostname, 256);
  sprintf(trace_name, "nmad_%s", hostname);
  setTraceType(PAJE);
  CHECK_RETURN (initTrace (trace_name, 0, GTG_FLAG_NONE));

  /* create trace types */
  CHECK_RETURN (addContType ("Container_Core", "0", "Container_Core"));
  CHECK_RETURN (addContType ("Container_Global", "Container_Core", "Container_Global"));
  CHECK_RETURN (addContType ("Container_Connections", "Container_Core", "Container_Connections"));
  CHECK_RETURN (addContType ("Container_Connection", "Container_Connections", "Container_Connection"));

  CHECK_RETURN (addEventType ("Event_Try_And_Commit", "Container_Connection", "Event_Try_And_Commit"));

  CHECK_RETURN (addVarType ("Var_Outlist_Pw_Size", "Var_Outlist_Pw_Size", "Container_Connection"));
  CHECK_RETURN (addVarType ("Var_Outlist_Nb_Pw", "Var_Outlist_Nb_Pw", "Container_Connection"));
  CHECK_RETURN (addVarType ("Var_Next_Pw_Size", "Var_Next_Pw_Size", "Container_Connection"));
  CHECK_RETURN (addVarType ("Var_Next_Pw_Remaining_Data_Area", "Var_Next_Pw_Remaining_Data_Area", "Container_Connection"));

  CHECK_RETURN (addEventType ("Event_Pw_Submited", "Container_Connection", "Event_Pw_Submited"));

  CHECK_RETURN (addVarType ("Var_Pw_Submited_Size", "Var_Pw_Submited_Size", "Container_Connection"));
  CHECK_RETURN (addVarType ("Var_Gdrv_Profile_Latency", "Var_Gdrv_Profile_Latency", "Container_Connection"));
  CHECK_RETURN (addVarType ("Var_Gdrv_Profile_bandwidth", "Var_Gdrv_Profile_bandwidth", "Container_Connection"));

  /* init GTG containers */
  addContainer(0.0, "C_CORE", "Container_Core", "0", "C_CORE", "0");
  CHECK_RETURN (addContainer(0.0, "C_GLOBAL", "Container_Global", "C_CORE", "C_GLOBAL", "0"));
  CHECK_RETURN (addContainer(0.0, "C_CONNECTIONS", "Container_Connections", "C_CORE", "C_CONNECTIONS", "0"));

  int cpt_try_and_commit = 0;
  int cpt_pw_submited = 0;

  int i;
  for(i = 0; i < ((__nm_trace.last > NM_TRACE_MAX) ? NM_TRACE_MAX : __nm_trace.last) ; i++)
    {
      const struct nm_trace_entry_s* e = &__nm_trace.entries[i];
      const double d = TBX_TIMING_DELAY(__nm_trace.orig, e->tick) / 1000000.0;

      char level_label[128];
      switch(e->scope)
	{
	case NM_TRACE_SCOPE_CORE:
	  sprintf(level_label, "CORE");
	  break;
	case NM_TRACE_SCOPE_PW:
	  sprintf(level_label, "PW");
	  break;
	case NM_TRACE_SCOPE_CONNECTION:
	  sprintf(level_label, "CONNECTION_%p", e->p_gate);
	  break;
	default:
	  sprintf(level_label, "(unkown)");
	  break;
	}

      char cont_name[64];
      char state_type[128];
      int _var;
      char value[256];

      sprintf(cont_name, "C_%s", level_label);
      sprintf(state_type, "State_%s", level_label);

      switch(e->event)
	{
	case NM_TRACE_EVENT_CONNECT:
	  CHECK_RETURN (addContainer(d, cont_name,"Container_Connection", "C_CONNECTIONS", cont_name, "0"));
	  break;
	case NM_TRACE_EVENT_TRY_COMMIT:
	  sprintf(value, "%d", cpt_try_and_commit);
	  cpt_try_and_commit++;
	  CHECK_RETURN (addEvent(d, "Event_Try_And_Commit", cont_name, value));
	  break;
	case NM_TRACE_EVENT_Pw_Submited:
	  sprintf(value, "%d", cpt_pw_submited);
	  cpt_pw_submited++;
	  CHECK_RETURN (addEvent(d, "Event_Pw_Submited", cont_name, value));
	  break;
	case NM_TRACE_EVENT_VAR_CO_Pw_Submitted_Size:
	  _var = (uintptr_t)e->value;
	  CHECK_RETURN(setVar(d, "Var_Pw_Submited_Size", cont_name, _var));
	  CHECK_RETURN(subVar(d, "Var_Outlist_Pw_Size", cont_name, _var));
	  break;
	case NM_TRACE_EVENT_VAR_CO_Gdrv_Profile_Latency:
	  _var = (uintptr_t)e->value;
	  CHECK_RETURN(setVar(d, "Var_Gdrv_Profile_Latency", cont_name, _var));
	  break;
	case NM_TRACE_EVENT_VAR_CO_Gdrv_Profile_Bandwidth:
	  _var = (uintptr_t)e->value;
	  CHECK_RETURN(setVar(d, "Var_Gdrv_Profile_bandwidth", cont_name, _var));
	  break;
	case NM_TRACE_EVENT_Pw_Outlist:
	  _var = (uintptr_t)e->value;
	  CHECK_RETURN (addEvent(d, "Event_Pw_Outlist", cont_name, NULL));
	  break;
	case NM_TRACE_EVENT_VAR_CO_Outlist_Pw_Size:
	  _var = (uintptr_t)e->value;
	  CHECK_RETURN(addVar(d, "Var_Outlist_Pw_Size", cont_name, _var));
	  break;
	case NM_TRACE_EVENT_VAR_CO_Outlist_Nb_Pw:
	  _var = (uintptr_t)e->value;
	  CHECK_RETURN(setVar(d, "Var_Outlist_Nb_Pw", cont_name, _var));
	  break;
	case NM_TRACE_EVENT_VAR_CO_Next_Pw_Size:
	  _var = (uintptr_t)e->value;
	  CHECK_RETURN(setVar(d, "Var_Next_Pw_Size", cont_name, _var));
	  break;
	case NM_TRACE_EVENT_VAR_CO_Next_Pw_Remaining_Data_Area:
	  _var = (uintptr_t)e->value;
	  CHECK_RETURN(setVar(d, "Var_Next_Pw_Remaining_Data_Area", cont_name, _var));
	  break;
	default:
	  break;
	}
    }
  endTrace();
}

void nm_trace_init(void)
{
  fprintf(stderr, "# nmad trace enabled.\n");
  TBX_GET_TICK(__nm_trace.orig);
}

void nm_trace_exit(void)
{
  nm_trace_flush();
}


#endif /* NMAD_TRACE */
