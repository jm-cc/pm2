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

#include "helper.h"

#define MAX     (8 * 1024 * 1024)
#define LOOPS   2000

static __inline__
uint32_t _next(uint32_t len)
{
  if(!len)
    return 4;
  else if(len < 32)
    return len + 4;
  else if(len < 1024)
    return len + 32;
  else
    return len << 1;
}

int
main(int	  argc,
     char	**argv) {
  char			*buf		= NULL;
  uint32_t		 len;
  nm_pack_cnx_t         cnx;

  init(&argc, argv);

  buf = malloc(MAX);
  memset(buf, 0, MAX);

  if (is_server) {
    int k;
    /* server
     */
    for(len = 0; len <= MAX; len = _next(len)) {
      for(k = 0; k < LOOPS; k++) {
        nm_begin_unpacking(p_core, NM_ANY_GATE, 0, &cnx);
        nm_unpack(&cnx, buf, len);
        nm_end_unpacking(&cnx);

        nm_begin_packing(p_core, gate_id, 0, &cnx);
        nm_pack(&cnx, buf, len);
        nm_end_packing(&cnx);
      }
    }

  } else {
    tbx_tick_t t1, t2;
    int k;
    /* client
     */
    for(len = 0; len <= MAX; len = _next(len)) {

      TBX_GET_TICK(t1);

      for(k = 0; k < LOOPS; k++) {
        nm_begin_packing(p_core, gate_id, 0, &cnx);
        nm_pack(&cnx, buf, len);
        nm_end_packing(&cnx);

        nm_begin_unpacking(p_core, NM_ANY_GATE, 0, &cnx);
        nm_unpack(&cnx, buf, len);
        nm_end_unpacking(&cnx);
      }

      TBX_GET_TICK(t2);

      printf("%d\t%lf\n", len, TBX_TIMING_DELAY(t1, t2)/(2*LOOPS));
    }
  }

  free(buf);
  nmad_exit();
  exit(0);
}
