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


#include <stdint.h>
#include <sys/uio.h>
#include <assert.h>

#include "nm_private.h"

/** Main function of the core scheduler loop.
 *
 * This is the heart of NewMadeleine...
 */
int nm_schedule(struct nm_core *p_core)
{
#ifdef NMAD_POLL
  nmad_lock();  

#ifdef NMAD_DEBUG
  static int scheduling_in_progress = 0;
  assert(!scheduling_in_progress);
  scheduling_in_progress = 1;  
#endif /* NMAD_DEBUG */

  nm_sched_out(p_core);

  nm_sched_in(p_core);
  
  nmad_unlock();
  
#ifdef NMAD_DEBUG
  scheduling_in_progress = 0;
#endif /* NMAD_DEBUG */

  return NM_ESUCCESS;
#else  /* NMAD_POLL */
  __piom_check_polling(PIOM_POLL_WHEN_FORCED);
  return 0;
#endif /* NMAD_POLL */
}
