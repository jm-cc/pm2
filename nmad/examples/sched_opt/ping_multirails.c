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

//#define ISO_SPLIT

#define SPLIT_THRESHOLD (64 * 1024)
#define NB_PACKS      2
#define MIN           (4 * NB_PACKS)
#define MAX           (8 * 1024 * 1024)
#define LOOPS         2000

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

static __inline__
uint32_t _size(uint32_t len, unsigned n)
{
  unsigned chunk = len / NB_PACKS;

#ifndef ISO_SPLIT
  if (len >= 256 * 1024) {
    if(n & 1) { /* pack 1 */
      return chunk - chunk / 8;
    } else { /* pack 0 */
      return chunk + chunk / 8;
    }
  }
#endif

  return chunk;
}

int
main(int	  argc,
     char	**argv) {
  char			*buf		= NULL;
  uint32_t		 len;
  struct nm_so_cnx         cnx;

  init(&argc, argv);

  buf = malloc(MAX);
  memset(buf, 0, MAX);

  if (is_server) {
    int k;
    /* server
     */
    for(len = MIN; len <= MAX; len = _next(len)) {
      unsigned long n;
      void *_buf;

      for(k = 0; k < LOOPS; k++) {

        _buf = buf;
        nm_so_begin_unpacking(pack_if, gate_id, 0, &cnx);
        if(len < SPLIT_THRESHOLD)
          nm_so_unpack(&cnx, _buf, len);
        else
          for(n = 0; n < NB_PACKS; n++) {
            unsigned long size = _size(len, n);
            nm_so_unpack(&cnx, _buf, size);
            _buf += size;
          }
        nm_so_end_unpacking(&cnx);

        _buf = buf;
        nm_so_begin_packing(pack_if, gate_id, 0, &cnx);
        if(len < SPLIT_THRESHOLD)
          nm_so_pack(&cnx, _buf, len);
        else
          for(n = 0; n < NB_PACKS; n++) {
            unsigned long size = _size(len, n);
            nm_so_pack(&cnx, _buf, size);
            _buf += size;
          }
        nm_so_end_packing(&cnx);
      }
    }

  } else {
    tbx_tick_t t1, t2;
    int k;

    /* client
     */
    for(len = MIN; len <= MAX; len = _next(len)) {
      unsigned long n;
      void *_buf;
      double sum, lat, bw_million_byte, bw_mbyte;

      TBX_GET_TICK(t1);

      for(k = 0; k < LOOPS; k++) {

        _buf = buf;
        nm_so_begin_packing(pack_if, gate_id, 0, &cnx);
        if(len < SPLIT_THRESHOLD)
          nm_so_pack(&cnx, _buf, len);
        else
          for(n = 0; n < NB_PACKS; n++) {
            unsigned long size = _size(len, n);
            nm_so_pack(&cnx, _buf, size);
            _buf += size;
          }
        nm_so_end_packing(&cnx);

        _buf = buf;
        nm_so_begin_unpacking(pack_if, gate_id, 0, &cnx);
        if(len < SPLIT_THRESHOLD)
          nm_so_unpack(&cnx, _buf, len);
        else
          for(n = 0; n < NB_PACKS; n++) {
            unsigned long size = _size(len, n);
            nm_so_unpack(&cnx, _buf, size);
            _buf += size;
          }
        nm_so_end_unpacking(&cnx);

      }

      TBX_GET_TICK(t2);

      sum = TBX_TIMING_DELAY(t1, t2);
      lat = sum / (2 * LOOPS);
      bw_million_byte = len * (LOOPS / (sum / 2));
      bw_mbyte = bw_million_byte / 1.048576;

      printf("%d\t%lf\t%8.3f\t%8.3f\n",
             len, lat, bw_million_byte, bw_mbyte);
    }
  }

  return 0;
}

