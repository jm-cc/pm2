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

#ifndef NM_EVENT_H
#define NM_EVENT_H

struct nm_so_event_s
{
  nm_so_status_t status;
  nm_gate_t p_gate;
  nm_tag_t tag;
  uint8_t seq;
  uint32_t len;
  tbx_bool_t any_src;
};

static inline void nm_so_status_event(nm_core_t p_core, const struct nm_so_event_s*const event)
{
  nm_so_monitor_itor_t i;
  if(event->any_src)
    {
      struct nm_so_any_src_s*any_src = nm_so_any_src_get(&p_core->so_sched.any_src, event->tag);
      any_src->status |= event->status;
    }
  else
    {
      struct nm_so_tag_s*p_so_tag = nm_so_tag_get(&event->p_gate->p_so_gate->tags, event->tag);
      p_so_tag->status[event->seq] |= event->status;
    }
  puk_vect_foreach(i, nm_so_monitor, &p_core->so_sched.monitors)
    {
      if((*i)->mask & event->status)
	{
	  ((*i)->notifier)(event);
	}
    }
}

static inline void nm_so_monitor_add(nm_core_t p_core, const struct nm_so_monitor_s*m)
{
  nm_so_monitor_vect_push_back(&p_core->so_sched.monitors, m);
}

#endif /* NM_EVENT_H */

