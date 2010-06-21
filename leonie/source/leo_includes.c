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
 *
 * - helper routines used by leo_file_processing to handle the
 *   inclusion of the networks configuration file
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "leonie.h"

static
void
merge_network_entries(p_tbx_htable_t dst,
		      p_tbx_htable_t src)
{
  char          *dst_dev   = NULL;
  char          *src_dev   = NULL;
  p_tbx_slist_t  dst_slist = NULL;
  p_tbx_slist_t  src_slist = NULL;

  dst_dev = leoparse_read_id(src, "dev");
  src_dev = leoparse_read_id(src, "dev");

  if (!tbx_streq(src_dev, dst_dev))
    leo_terminate("duplicate network definition", NULL);

  leoparse_convert_to_slist(dst, "hosts");
  dst_slist = leoparse_read_as_slist(dst, "hosts");
  src_slist = leoparse_read_as_slist(src, "hosts");

  tbx_slist_merge_after(src_slist, dst_slist);
}

void
process_network_include_file(p_leo_networks_t networks,
			     p_tbx_htable_t   include_htable)
{
  p_tbx_slist_t network_slist = NULL;

  network_slist = leoparse_read_as_slist(include_htable, "networks");
  if (network_slist)
    {
      if (tbx_slist_is_nil(network_slist))
	goto empty_list;

      tbx_slist_merge_after(networks->slist, network_slist);

      do
	{
	  p_leoparse_object_t  object        = NULL;
	  p_tbx_htable_t       network_entry = NULL;
	  p_tbx_htable_t       old_entry     = NULL;
	  char                *name          = NULL;

	  object = tbx_slist_remove_from_head(network_slist);
	  network_entry = leoparse_get_htable(object);

	  name      = leoparse_read_id(network_entry, "name");
	  old_entry = tbx_htable_get(networks->htable, name);

	  if (!old_entry)
	    {
	      tbx_htable_add(networks->htable, name, network_entry);
	    }
	  else
	    {
	      merge_network_entries(old_entry, network_entry);
	    }
	}
      while (!tbx_slist_is_nil(network_slist));

    empty_list:
      tbx_slist_free(network_slist);
      network_slist = NULL;
    }
}

void
include_network_files(p_leo_networks_t networks,
		      p_tbx_slist_t    include_files_slist)
{
  p_tbx_htable_t filename_htable = NULL;

  filename_htable = tbx_htable_empty_table();

  tbx_slist_ref_to_head(include_files_slist);
  do
    {
      p_leoparse_object_t  object         = NULL;
      char                *filename       = NULL;

      object = tbx_slist_ref_get(include_files_slist);
      filename = leoparse_get_id(object);

      if (!tbx_htable_get(filename_htable, filename))
	{
	  p_tbx_htable_t include_htable = NULL;

	  tbx_htable_add(filename_htable, filename, filename);

	  include_htable = leoparse_parse_local_file(filename);
	  process_network_include_file(networks, include_htable);

	  tbx_htable_cleanup_and_free(include_htable);
	  include_htable = NULL;
	}
    }
  while (tbx_slist_ref_forward(include_files_slist));

  tbx_htable_cleanup_and_free(filename_htable);
  filename_htable = NULL;
}

