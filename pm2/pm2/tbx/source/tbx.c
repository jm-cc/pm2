
/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 "the PM2 team" (pm2-dev@listes.ens-lyon.fr)
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

/*
 * Tbx.c
 * =====
 */
#include "tbx.h"

static volatile tbx_bool_t initialized = tbx_false;

void
tbx_init(int    argc,
	 char **argv)
{
  if (!initialized)
    {
      initialized = tbx_true;

      /* Safe malloc */
      //#ifdef TBX_SAFE_MALLOC
      tbx_safe_malloc_init();
      //#endif /* TBX_SAFE_MALLOC */

      /* Timer */
      tbx_timing_init();

      /* List manager */
      tbx_list_manager_init();

      /* Slist manager */
      tbx_slist_manager_init();

      /* Hash table manager */
      tbx_htable_manager_init();
    }
  
}

void
tbx_purge_cmd_line(int   *argc,
		   char **argv)
{
  LOG_IN();
  /* --- */
  LOG_OUT();
}
