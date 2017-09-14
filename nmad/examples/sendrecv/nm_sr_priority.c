/*
 * NewMadeleine
 * Copyright (C) 2006-2017 (see AUTHORS file)
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

#define MAXMSG 10

static int values[MAXMSG];
static nm_sr_request_t requests[MAXMSG];
static const int tag = 0;

int main(int argc, char**argv)
{ 
  nm_examples_init(&argc, argv);

  int i;
  for(i = 0; i < MAXMSG; i++)
    {
      if(is_server)
	{
	  values[i] = -1;
	  nm_sr_irecv(p_session, p_gate /* NM_ANY_GATE */, tag, &values[i], sizeof(int), &requests[i]);
	}
      else
	{
	  values[i] = i;
	  nm_sr_send_init(p_session, &requests[i]);
	  nm_sr_send_pack_contiguous(p_session, &requests[i], &values[i], sizeof(int));
	  nm_sr_send_set_priority(p_session, &requests[i], i);
	  nm_sr_send_isend(p_session, &requests[i], p_gate, tag);
	}
    }
  for(i = 0; i < MAXMSG; i++)
    {
      if(is_server)
	{
	  nm_sr_rwait(p_session, &requests[i]);
	  printf("# %d: %d\n", i, values[i]);
	}
      else
	{
	  nm_sr_swait(p_session, &requests[i]);
	}
      }
  
  nm_examples_exit();
  exit(0);
}
