/*
 * NewMadeleine
 * Copyright (C) 2017 (see AUTHORS file)
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

#include <nm_launcher_interface.h>

#include <stdio.h>

#define SESSIONS_MAX 10000

int main(int argc, char**argv)
{
  int rc = nm_launcher_init(&argc, argv);
  if(rc != NM_ESUCCESS)
    {
      fprintf(stderr, "error in nm_launcher_init()\n");
    }
  int rank = -1, size = -1;
  nm_launcher_get_rank(&rank);
  nm_launcher_get_size(&size);

  printf("rank = %d; size = %d\n", rank, size);
  printf("opening %d sessions...\n", SESSIONS_MAX);

  nm_session_t*p_sessions = malloc(sizeof(nm_session_t) * SESSIONS_MAX);
  int i;
  for(i = 0; i < SESSIONS_MAX; i++)
    {
      char session_name[32];
      snprintf(session_name, 32, "launcher-mini-%d", i);
      rc = nm_launcher_session_open(&p_sessions[i], session_name);
      if(rc != NM_ESUCCESS)
	{
	  fprintf(stderr, "error in nm_launcher_session_open()\n");
	}
    }
  printf("closing %d sessions...\n", SESSIONS_MAX);
  for(i = 0; i < SESSIONS_MAX; i++)
    {
      nm_launcher_session_close(p_sessions[i]);
    }
  printf("done.\n");
  
  nm_launcher_exit();

  return 0;
}
