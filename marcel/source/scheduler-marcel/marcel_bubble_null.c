/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2006 "the PM2 team" (see AUTHORS file)
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

#include "marcel.h"

#ifdef MA__BUBBLES

static void __sched_submit(marcel_entity_t *e[], int ne, struct marcel_topo_level **l)
{
  int i;

  for (i = 0; i < ne; i++) 
    {	      
      int state = ma_get_entity(e[i]);
      ma_put_entity(e[i], &l[0]->sched.hold, state);
    }
}

int
null_sched_submit(marcel_entity_t *e)
{
  struct marcel_topo_level *l = marcel_topo_level(0,0);
  __sched_submit(&e, 1, &l);

  return 0;
}

int
null_sched_vp_is_idle(unsigned vp)
{
#ifdef MARCEL_BUBBLE_STEAL
  /* TODO: avoir un scheduler steal plutôt ? */
  return marcel_bubble_steal_work(vp);
#else
  return 0;
#endif
}

struct ma_bubble_sched_struct marcel_bubble_null_sched = {
  .submit = null_sched_submit,
};

#endif /* MA__BUBBLES */
