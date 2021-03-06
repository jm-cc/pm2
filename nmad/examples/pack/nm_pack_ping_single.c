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


#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "../common/nm_examples_helper.h"
#include <nm_pack_interface.h>

int main(int argc, char**argv)
{
  char*buf = NULL;
  nm_pack_cnx_t cnx;
  
  nm_examples_init(&argc, argv);
  
  buf = malloc(1024);
  memset(buf, 0, 1024);
  
  if(is_server)
    {
      /* server
       */
      nm_begin_unpacking(p_session, p_gate, 0, &cnx);
      nm_unpack(&cnx, buf, 4);
      nm_unpack(&cnx, buf + 4, 4);
      nm_end_unpacking(&cnx);
      
      nm_begin_packing(p_session, p_gate, 0, &cnx);
      nm_pack(&cnx, buf, 4);
      nm_pack(&cnx, buf + 4, 4);
      nm_end_packing(&cnx);
    }
  else
    {
      /* client
       */
      strcpy(buf, "abcdefgh");
      
      nm_begin_packing(p_session, p_gate, 0, &cnx);
      nm_pack(&cnx, buf, 4);
      nm_pack(&cnx, buf + 4, 4);
      nm_end_packing(&cnx);
      
      memset(buf, 0, 1024);
      
      nm_begin_unpacking(p_session, p_gate, 0, &cnx);
      nm_unpack(&cnx, buf, 4);
      nm_unpack(&cnx, buf + 4, 4);
      nm_end_unpacking(&cnx);
      
      printf("<%s>\n", buf);
    }
  
  free(buf);
  nm_examples_exit();
  return 0;
}
