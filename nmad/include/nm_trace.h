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

#ifndef NM_TRACE_H
#define NM_TRACE_H


typedef enum nm_trace_scope_e
  {
    _NM_TRACE_TOPO_NONE = 0,
    NM_TRACE_TOPO_GLOBAL,
    NM_TRACE_TOPO_CONNECTIONS,
    NM_TRACE_TOPO_CONNECTION
  }
  nm_trace_topo_t;

typedef int nm_trace_event_t;


#define NM_TRACE_EVENT_NEW_CONNECTION  ((nm_trace_event_t)0)
#define NM_TRACE_EVENT_TRY_COMMIT      ((nm_trace_event_t)1)
#define NM_TRACE_EVENT_VAR_CO_NB_GDRV  ((nm_trace_event_t)2)

#define NM_TRACE_EVENT_Pw_Outlist ((nm_trace_event_t)3)
#define NM_TRACE_EVENT_VAR_CO_Outlist_Pw_Size ((nm_trace_event_t)4)
#define NM_TRACE_EVENT_VAR_CO_Outlist_Nb_Pw ((nm_trace_event_t)5)
#define NM_TRACE_EVENT_VAR_CO_Next_Pw_Size ((nm_trace_event_t)6)
#define NM_TRACE_EVENT_VAR_CO_Next_Pw_Remaining_Data_Area ((nm_trace_event_t)7)
#define NM_TRACE_EVENT_VAR_CO_Next_Pw_Remaining_Header_Area ((nm_trace_event_t)8)



#define NM_TRACE_EVENT_Pw_Submited ((nm_trace_event_t)9)
#define NM_TRACE_EVENT_VAR_CO_Pw_Submitted_Seq ((nm_trace_event_t)10)
#define NM_TRACE_EVENT_VAR_CO_Gdrv_Profile_Latency ((nm_trace_event_t)11)
#define NM_TRACE_EVENT_VAR_CO_Gdrv_Profile_Bandwidth ((nm_trace_event_t)12)
#define NM_TRACE_EVENT_VAR_CO_Pw_Submitted_Size ((nm_trace_event_t)13)

void nm_trace_flush(void);
void nm_trace_container(nm_trace_topo_t _topo, nm_trace_event_t _event, int _cpt_connections);
void nm_trace_var(nm_trace_topo_t _topo, nm_trace_event_t  _event, int _value, int _cpt_connections);
void nm_trace_state(nm_trace_topo_t _topo, nm_trace_event_t _event, int _cpt_connections);
void nm_trace_event(nm_trace_topo_t _topo, nm_trace_event_t _event, void* _value, int _cpt_connections);

#endif /* NM_TRACE_H */
