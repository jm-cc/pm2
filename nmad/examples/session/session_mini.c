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

#include <nm_session_interface.h>

#include <stdio.h>


int main(int argc, char**argv)
{
  nm_session_t p_session = NULL;
  int err = nm_session_create(&p_session, "mini");

  const char*local_url;
  nm_session_init(p_session, &argc, argv, &local_url);

  fprintf(stderr, "# session_mini: local url = %s\n", local_url);
  

  nm_session_destroy(p_session);

  return 0;
}
