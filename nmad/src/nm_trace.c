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

#include <nm_trace.h>
#include <stdio.h>
#include <GTG.h>
#include <tbx.h>

struct nmad_trace_entry_s
{
  tbx_tick_t tick;
  double time;
  nm_trace_topo_t topo;
  nm_trace_event_t event;
  void* value;
  int   cpt_connections;
};

static struct
{
  tbx_tick_t orig;
  int last;
  struct nmad_trace_entry_s entries[NMAD_TRACE_MAX];
} __nmad_trace = { .last = 0 };

struct nmad_trace_entry_s* nmad_trace_get_entry(void)
{
  const int i = __sync_fetch_and_add(&__nmad_trace.last, 1);
  if(i >= NMAD_TRACE_MAX)
    return NULL;
  return &__nmad_trace.entries[i];
}


void nmad_trace_event(nm_trace_topo_t _topo, nm_trace_event_t _event, void* _value, int _cpt_connections)
{
  struct nmad_trace_entry_s* entry = nmad_trace_get_entry();
  if(entry) 
    {
      TBX_GET_TICK(entry->tick);
      entry->event = _event;
      entry->topo  = _topo;
      entry->value = _value; 
      entry->cpt_connections = _cpt_connections;
    }
}

void nmad_trace_state(nm_trace_topo_t _topo, nm_trace_event_t _event, int _cpt_connections)
{
  nmad_trace_event(_topo, _event, NULL, _cpt_connections);
}

void nmad_trace_container(nm_trace_topo_t _topo, nm_trace_event_t _event, int _cpt_connections)
{
  nmad_trace_event(_topo, _event, NULL, _cpt_connections);
}


void nmad_trace_var(nm_trace_topo_t _topo, nm_trace_event_t  _event, int _value, int _cpt_connections)
{
  void* value = (void*)((uintptr_t)_value);
  nmad_trace_event(_topo, _event, value, _cpt_connections);
}

static void nmad_trace_flush()
{

  static int flushed = 0;
  if(flushed) return;
  flushed = 1;

  int i;
  for(i = 0; i < ((__nmad_trace.last > NMAD_TRACE_MAX) ? NMAD_TRACE_MAX : __nmad_trace.last) ; i++)
    {
      struct nmad_trace_entry_s* e = &__nmad_trace.entries[i];

      e->time = TBX_TIMING_DELAY(__nmad_trace.orig, e->tick) / 1000000.0;
      
      char level_label[128];
      switch(e->topo)
	{
	case TOPO_CORE:
	  sprintf(level_label, "CORE");
	  break;
	case TOPO_GLOBAL:
	  sprintf(level_label, "GLOBAL");
	  break;
	case TOPO_CONNECTIONS:
	  sprintf(level_label, "CONNECTIONS");
	  break;
	case TOPO_CONNECTION:
	  sprintf(level_label, "CONNECTION_%d", e->cpt_connections);
	  break;
	default:
	  sprintf(level_label, "(unkown)");
	  break;
	}
      
      char cont_name[64];
      char state_type[128];
      int _var;
      
      sprintf(cont_name, "C_%s", level_label);
      sprintf(state_type, "State_%s", level_label);
      
      switch(e->event)
	{
	case NMAD_TRACE_EVENT_NEW_CONNECTION:
	  CHECK_RETURN (addContainer(e->time, cont_name,"Container_Connection", "C_CONNECTIONS", cont_name, "0"));
	  break;
	case NMAD_TRACE_EVENT_TRY_COMMIT:
	  CHECK_RETURN (addEvent(e->time, "Event_Try_And_Commit", cont_name, NULL));
	  break;
	case NMAD_TRACE_EVENT_VAR_CO_NB_GDRV:
	  _var = (uintptr_t)e->value;
	  CHECK_RETURN(setVar(e->time, "Var_Nb_Gdrv", cont_name, _var));
	  break;
	case NMAD_TRACE_EVENT_VAR_CO_Pw_Submitted_Size:
	  _var = (uintptr_t)e->value;
	  CHECK_RETURN(setVar(e->time, "Var_Pw_Submited_Size", cont_name, _var));
	  break;
	case NMAD_TRACE_EVENT_VAR_CO_Pw_Submitted_Seq:
	  _var = (uintptr_t)e->value;
	  CHECK_RETURN(setVar(e->time, "Var_Pw_Submited_Seq", cont_name, _var));
	  break;
	case NMAD_TRACE_EVENT_VAR_CO_Gdrv_Profile_Latency:
	  _var = (uintptr_t)e->value;
	  CHECK_RETURN(setVar(e->time, "Var_Gdrv_Profile_Latency", cont_name, _var));
	  break;
	case NMAD_TRACE_EVENT_VAR_CO_Gdrv_Profile_Bandwidth:
	  _var = (uintptr_t)e->value;
	  CHECK_RETURN(setVar(e->time, "Var_Gdrv_Profile_bandwidth", cont_name, _var));
	  break;
	case NMAD_TRACE_EVENT_VAR_CO_Outlist_Pw_Seq:
	  _var = (uintptr_t)e->value;
	  CHECK_RETURN(setVar(e->time, "Var_Outlist_Pw_Seq", cont_name, _var));
	  break;
	case NMAD_TRACE_EVENT_VAR_CO_Outlist_Pw_Size:
	  _var = (uintptr_t)e->value;
	  CHECK_RETURN(setVar(e->time, "Var_Outlist_Pw_Size", cont_name, _var));
	  break;
	default:
	  break;
	}
    }
}

void nm_trace_init()
{
  /* Initialisation */
  char trace_name[256]; 
  char hostname[256];
  gethostname(hostname, 256);
  sprintf(trace_name, "nmad_%s", hostname);
  setTraceType(PAJE);
  CHECK_RETURN (initTrace (trace_name, 0, GTG_FLAG_NONE));

  TBX_GET_TICK(__nmad_trace.orig);

  /* Creating type */
  CHECK_RETURN (addContType ("Container_Core", "0", "Container_Core"));
  CHECK_RETURN (addContType ("Container_Global", "Container_Core", "Container_Global"));
  CHECK_RETURN (addContType ("Container_Connections", "Container_Core", "Container_Connections"));
  CHECK_RETURN (addContType ("Container_Connection", "Container_Connections", "Container_Connection"));

  CHECK_RETURN (addEventType ("Event_Try_And_Commit", "Container_Connection", "Event_Try_And_Commit"));

  CHECK_RETURN (addVarType ("Var_Nb_Gdrv", "Var_Nb_Gdrv", "Container_Connection"));

  CHECK_RETURN (addVarType ("Var_Pw_Submited_Size", "Var_Pw_Submited_Size", "Container_Connection"));
  CHECK_RETURN (addVarType ("Var_Pw_Submited_Seq", "Var_Pw_Submited_Seq", "Container_Connection"));
  CHECK_RETURN (addVarType ("Var_Gdrv_Profile_Latency", "Var_Gdrv_Profile_Latency", "Container_Connection"));
  CHECK_RETURN (addVarType ("Var_Gdrv_Profile_bandwidth", "Var_Gdrv_Profile_bandwidth", "Container_Connection"));

  CHECK_RETURN (addVarType ("Var_Outlist_Pw_Seq", "Var_Outlist_Pw_Seq", "Container_Connection"));
  CHECK_RETURN (addVarType ("Var_Outlist_Pw_Size", "Var_Outlist_Pw_Size", "Container_Connection"));

  /* Init Container */
  addContainer(0.0, "C_CORE", "Container_Core", "0", "C_CORE", "0"); 
  CHECK_RETURN (addContainer(0.0, "C_GLOBAL", "Container_Global", "C_CORE", "C_GLOBAL", "0")); 
  CHECK_RETURN (addContainer(0.0, "C_CONNECTIONS", "Container_Connections", "C_CORE", "C_CONNECTIONS", "0")); 
  
}

void nm_trace_exit()
{
  nmad_trace_flush();
  CHECK_RETURN ( endTrace());
}
