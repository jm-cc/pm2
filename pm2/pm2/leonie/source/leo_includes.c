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
 * leo_includes.c
 * ==============
 */ 

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "leonie.h"

void
process_network_include_file(p_leo_networks_t networks,
			     p_tbx_htable_t   include_htable)
{
  p_tbx_slist_t network_slist = NULL;

  LOG_IN();
  network_slist = leoparse_read_as_slist(include_htable, "networks");
  if (network_slist && !tbx_slist_is_nil(network_slist))
    {
      tbx_slist_merge_after(networks->slist, network_slist);

      do
	{
	  p_leoparse_object_t  object        = NULL;
	  p_tbx_htable_t       network_entry = NULL;
	  char                *name          = NULL;
	  
	  object = tbx_slist_remove_from_head(network_slist);
	  network_entry = leoparse_get_htable(object);
	  
	  name = leoparse_read_id(network_entry, "name");
	  tbx_htable_add(networks->htable, name, network_entry);
	}
      while(!tbx_slist_is_nil(network_slist));      
    }
  LOG_OUT();
}

void
include_network_files(p_leo_networks_t networks,
		      p_tbx_slist_t    include_files_slist)
{
  LOG_IN();
  tbx_slist_ref_to_head(include_files_slist);
      
  do
    {
      p_leoparse_object_t  object         = NULL;
      p_tbx_htable_t       include_htable = NULL;
      char                *filename       = NULL;
      
      object = tbx_slist_ref_get(include_files_slist);
      filename = leoparse_get_id(object);
      
      TRACE_STR("Including", filename);
      include_htable = leoparse_parse_local_file(filename);
      process_network_include_file(networks, include_htable);

      // *** include_htable should be freed here *** //
      include_htable = NULL;
    }
  while (tbx_slist_ref_forward(include_files_slist));
  LOG_OUT();
}

