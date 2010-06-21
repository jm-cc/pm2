/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 "the PM2 team" (see AUTHORS file)
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
 * Ntbx.c
 * ======
 */

#include "tbx.h"
#include "ntbx.h"

static volatile tbx_bool_t initialized = tbx_false;

void
ntbx_init(int   *argc TBX_UNUSED,
	  char **argv TBX_UNUSED)
{
  PM2_LOG_IN();
  if (!initialized)
    {
      initialized = tbx_true;
    }

  ntbx_misc_init();
  PM2_LOG_OUT();

}

void
ntbx_purge_cmd_line(int   *argc TBX_UNUSED,
		    char **argv TBX_UNUSED)
{
  PM2_LOG_IN();
  /* --- */
  PM2_LOG_OUT();
}

void
ntbx_exit(void)
{
  PM2_LOG_IN();
  /* --- */
  PM2_LOG_OUT();
}
