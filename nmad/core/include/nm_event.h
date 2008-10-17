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

static inline void nm_so_status_event(nm_core_t p_core, nm_so_status_t event, nm_gate_t gate,
				      nm_tag_t tag, uint8_t seq, tbx_bool_t any_src)
{
  nm_so_monitor_itor_t i;
  puk_vect_foreach(i, nm_so_monitor, &p_core->so_sched.monitors)
    {
      if((*i)->mask & event)
	{
	  ((*i)->notifier)(event, gate, tag, seq, any_src);
	}
    }
}

static inline void nm_so_monitor_add(nm_core_t p_core, const struct nm_so_monitor_s*m)
{
  nm_so_monitor_vect_push_back(&p_core->so_sched.monitors, m);
}

#endif /* NM_EVENT_H */

