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

  nm_session_t p_session = NULL;
  nm_launcher_session_open(&p_session, "launcher-mini");

  nm_launcher_session_close(p_session);

  nm_launcher_exit();

  return 0;
}
