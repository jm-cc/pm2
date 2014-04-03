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
#ifdef NMAD_TRACE

#ifndef NM_TRACE_H
#define NM_TRACE_H





typedef int nm_trace_event_t;

typedef nm_trace_event_t nm_trace_topo_t;

#define TOPO_GLOBAL         ((nm_trace_topo_t)1)
#define TOPO_CORE           ((nm_trace_topo_t)2)
#define TOPO_CONNECTIONS    ((nm_trace_topo_t)3)
#define TOPO_CONNECTION     ((nm_trace_topo_t)4)

#define NMAD_TRACE_EVENT_NEW_CONNECTION     ((nm_trace_event_t)1)
#define NMAD_TRACE_EVENT_CONNECTION_CONNECT     ((nm_trace_event_t)2)
#define NMAD_TRACE_EVENT_CONNECTION_CLOSED     ((nm_trace_event_t)3)
#define NMAD_TRACE_EVENT_TRY_COMMIT    ((nm_trace_event_t)4)
#define NMAD_TRACE_EVENT_VAR_CORE_NB_PENDING_SEND ((nm_trace_event_t)5)
#define NMAD_TRACE_EVENT_VAR_CORE_NB_PENDING_PACK ((nm_trace_event_t)6)
#define NMAD_TRACE_EVENT_VAR_CO_Outlist_Pw_Seq ((nm_trace_event_t)7)
#define NMAD_TRACE_EVENT_VAR_CO_Outlist_Pw_Size ((nm_trace_event_t)8)
#define NMAD_TRACE_EVENT_VAR_CO_NB_GDRV ((nm_trace_event_t)9)
#define NMAD_TRACE_EVENT_VAR_COMMIT_NB_PENDING_LARGE_SEND ((nm_trace_event_t)10)
#define NMAD_TRACE_EVENT_VAR_CO_Outlist_Pw_Tag ((nm_trace_event_t)11)
#define NMAD_TRACE_EVENT_VAR_CO_Pw_Submitted_Size ((nm_trace_event_t)12)
#define NMAD_TRACE_EVENT_VAR_CO_Gdrv_Profile_Latency ((nm_trace_event_t)13)
#define NMAD_TRACE_EVENT_VAR_CO_Gdrv_Profile_Bandwidth ((nm_trace_event_t)14)
#define NMAD_TRACE_EVENT_VAR_CO_Pw_Submitted_Seq ((nm_trace_event_t)15)


#define NMAD_TRACE_MAX (1024*1024*8)


static int nm_trace_connections_cpt = 0;


#define CHECK_RETURN(val) {if (val!=TRACE_SUCCESS){fprintf (stderr, "Function failed line %d. Leaving \n", __LINE__);exit (-1);}}



void nm_trace_exit();
void nm_trace_init();
static void nmad_trace_flush();
void nmad_trace_container(nm_trace_topo_t _topo, nm_trace_event_t _event, int _cpt_connections);
void nmad_trace_var(nm_trace_topo_t _topo, nm_trace_event_t  _event, int _value, int _cpt_connections);
void nmad_trace_state(nm_trace_topo_t _topo, nm_trace_event_t _event, int _cpt_connections);
void nmad_trace_event(nm_trace_topo_t _topo, nm_trace_event_t _event, void* _value, int _cpt_connections);

#endif /* NM_TRACE_H */
#endif /* NMAD_TRACE */
