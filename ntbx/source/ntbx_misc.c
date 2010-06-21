
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
 * ntbx_misc.c
 * -----------
 */
#include "ntbx.h"
#include <stdlib.h>
#include <string.h>
#include <netdb.h>

/*
 * Macro and constant definition
 * ___________________________________________________________________________
 */
#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 256
#endif /* MAXHOSTNAMELEN */

/*
 *  Functions
 * ___________________________________________________________________________
 */

/*
 * Initialization
 * --------------
 */
void
ntbx_misc_init(void)
{
  PM2_LOG_IN();
  /* Nothing to do */
  PM2_LOG_OUT();
}

//
//  Process container
///////////////////////////////////////////////////////////////////////////
//
void
ntbx_pc_add(p_ntbx_process_container_t  pc,
	    p_ntbx_process_t            process,
	    ntbx_process_lrank_t        local_rank,
	    void                       *object,
	    const char                 *name,
	    void                       *specific)
{
  p_ntbx_process_info_t process_info = NULL;

  PM2_LOG_IN();
  if (local_rank < 0)
    {
      local_rank = pc->count;
    }

  if (!pc->local_array_size)
    {
      pc->local_array_size  = 1 + 2 * local_rank;
      pc->global_array_size = 1 + 2 * process->global_rank;

      pc->local_index  =
	TBX_CALLOC((size_t)pc->local_array_size,
		   sizeof(p_ntbx_process_info_t));
      CTRL_ALLOC(pc->local_index);

      pc->global_index =
	TBX_CALLOC((size_t)pc->global_array_size,
		   sizeof(p_ntbx_process_info_t));
      CTRL_ALLOC(pc->global_index);
    }
  else
    {
      if (local_rank >= pc->local_array_size)
	{
	  ntbx_process_lrank_t old_size = pc->local_array_size;

	  pc->local_array_size =
	    tbx_max(2 * pc->local_array_size, local_rank + 1);

	  pc->local_index =
	    TBX_REALLOC(pc->local_index,
			pc->local_array_size
			* sizeof(p_ntbx_process_info_t));
	  CTRL_ALLOC(pc->local_index);

	  memset(pc->local_index + old_size,
		 0,
		 sizeof(p_ntbx_process_info_t)
		 * (pc->local_array_size - old_size));
	}

      if (process->global_rank >= pc->global_array_size)
	{
	  ntbx_process_grank_t old_size = pc->global_array_size;

	  pc->global_array_size =
	    tbx_max(2 * pc->global_array_size, process->global_rank + 1);

	  pc->global_index =
	    TBX_REALLOC(pc->global_index,
			pc->global_array_size
			* sizeof(p_ntbx_process_info_t));
	  CTRL_ALLOC(pc->global_index);

	  memset(pc->global_index + old_size,
		 0,
		 sizeof(p_ntbx_process_info_t)
		 * (pc->global_array_size - old_size));
	}
    }

  process_info = TBX_MALLOC(sizeof(ntbx_process_info_t));
  CTRL_ALLOC(process_info);

  process_info->local_rank = local_rank;
  process_info->process    = process;
  process_info->specific   = specific;

  if (   pc->local_index[local_rank]
	 || pc->global_index[process->global_rank])
    TBX_FAILURE("process_container slot already in use");

  pc->local_index[local_rank]            = process_info;
  pc->global_index[process->global_rank] = process_info;
  pc->count++;

  tbx_htable_add(process->ref, name, object);
  PM2_LOG_OUT();
}

ntbx_process_lrank_t
ntbx_pc_local_max(p_ntbx_process_container_t pc)
{
  ntbx_process_lrank_t lrank = -1;

  PM2_LOG_IN();
  if (pc->local_array_size)
    {
      lrank = pc->local_array_size;

      do
	{
	  lrank--;
	}
      while ((lrank >= 0) && !pc->local_index[lrank]);
    }
  PM2_LOG_OUT();

  return lrank;
}

ntbx_process_grank_t
ntbx_pc_local_to_global(p_ntbx_process_container_t pc,
			ntbx_process_lrank_t       local_rank)
{
  ntbx_process_grank_t global_rank = -1;

  PM2_LOG_IN();
  if ((local_rank >= 0) && (local_rank < pc->local_array_size))
    {
      p_ntbx_process_info_t process_info = NULL;

      process_info = pc->local_index[local_rank];
      if (process_info)
	{
	  global_rank = process_info->process->global_rank;
	}
    }
  PM2_LOG_OUT();

  return global_rank;
}

p_ntbx_process_t
ntbx_pc_get_local_process(p_ntbx_process_container_t pc,
			  ntbx_process_lrank_t       local_rank)
{
  p_ntbx_process_t process = NULL;

  PM2_LOG_IN();
  if ((local_rank >= 0) && (local_rank < pc->local_array_size))
    {
      p_ntbx_process_info_t process_info = NULL;

      process_info = pc->local_index[local_rank];
      process = process_info->process;
    }
  PM2_LOG_OUT();

  return process;
}

p_ntbx_process_info_t
ntbx_pc_get_local(p_ntbx_process_container_t pc,
		  ntbx_process_lrank_t       local_rank)
{
  p_ntbx_process_info_t process_info = NULL;

  PM2_LOG_IN();
  if ((local_rank >= 0) && (local_rank < pc->local_array_size))
    {
      process_info = pc->local_index[local_rank];
    }
  PM2_LOG_OUT();

  return process_info;
}

void *
ntbx_pc_get_local_specific(p_ntbx_process_container_t pc,
			   ntbx_process_lrank_t       local_rank)
{
  void *result = NULL;

  PM2_LOG_IN();
  if ((local_rank >= 0) && (local_rank < pc->local_array_size))
    {
      p_ntbx_process_info_t process_info = NULL;

      process_info = pc->local_index[local_rank];

      if (process_info)
	{
	  result = process_info->specific;
	}
    }
  PM2_LOG_OUT();

  return result;
}

tbx_bool_t
ntbx_pc_first_local_rank(p_ntbx_process_container_t  pc,
			 p_ntbx_process_lrank_t      local_rank)
{
  tbx_bool_t result = tbx_false;

  PM2_LOG_IN();
  if (pc->count)
    {
      *local_rank = 0;

      while (!pc->local_index[*local_rank])
	{
	  (*local_rank)++;
	}

      result = tbx_true;
    }
  PM2_LOG_OUT();

  return result;
}

tbx_bool_t
ntbx_pc_next_local_rank(p_ntbx_process_container_t  pc,
			p_ntbx_process_lrank_t      local_rank)
{
  tbx_bool_t result = tbx_false;

  PM2_LOG_IN();
  if (pc->count)
    {
      (*local_rank)++;

      while (*local_rank < pc->local_array_size)
	{
	  if (pc->local_index[*local_rank])
	    {
	      result = tbx_true;
	      break;
	    }

	  (*local_rank)++;
	}
    }
  PM2_LOG_OUT();

  return result;
}

ntbx_process_grank_t
ntbx_pc_global_max(p_ntbx_process_container_t pc)
{
  ntbx_process_grank_t grank = -1;

  PM2_LOG_IN();
  if (pc->global_array_size)
    {
      grank = pc->global_array_size;

      do
	{
	  grank--;
	}
      while ((grank >= 0) && !pc->local_index[grank]);
    }
  PM2_LOG_OUT();

  return grank;
}

ntbx_process_lrank_t
ntbx_pc_global_to_local(p_ntbx_process_container_t pc,
			ntbx_process_grank_t       global_rank)
{
  ntbx_process_lrank_t local_rank = -1;

  PM2_LOG_IN();
  if ((global_rank >= 0) && (global_rank < pc->global_array_size))
    {
      p_ntbx_process_info_t process_info = NULL;

      process_info = pc->global_index[global_rank];

      if (process_info)
	{
	  local_rank = process_info->local_rank;
	}
    }
  PM2_LOG_OUT();

  return local_rank;
}

p_ntbx_process_t
ntbx_pc_get_global_process(p_ntbx_process_container_t pc,
			   ntbx_process_grank_t       global_rank)
{
  p_ntbx_process_t process = NULL;

  PM2_LOG_IN();
  if ((global_rank >= 0) && (global_rank < pc->global_array_size))
    {
      p_ntbx_process_info_t process_info = NULL;

      process_info = pc->global_index[global_rank];
      process = process_info->process;
    }
  PM2_LOG_OUT();

  return process;
}

p_ntbx_process_info_t
ntbx_pc_get_global(p_ntbx_process_container_t pc,
		   ntbx_process_grank_t       global_rank)
{
  p_ntbx_process_info_t process_info = NULL;

  PM2_LOG_IN();
  if ((global_rank >= 0)
      && (global_rank < pc->global_array_size))
    {
      process_info = pc->global_index[global_rank];
    }
  PM2_LOG_OUT();

  return process_info;
}

void *
ntbx_pc_get_global_specific(p_ntbx_process_container_t pc,
			    ntbx_process_grank_t       global_rank)
{
  void *result = NULL;

  PM2_LOG_IN();
  if ((global_rank >= 0) && (global_rank < pc->global_array_size))
    {
      p_ntbx_process_info_t process_info = NULL;

      process_info = pc->global_index[global_rank];
      result = process_info->specific;
    }
  PM2_LOG_OUT();

  return result;
}

tbx_bool_t
ntbx_pc_first_global_rank(p_ntbx_process_container_t pc,
			  p_ntbx_process_grank_t     global_rank)
{
  tbx_bool_t result = tbx_false;

  PM2_LOG_IN();
  if (pc->count)
    {
      *global_rank = 0;

      while (!pc->global_index[*global_rank])
	{
	  (*global_rank)++;
	}

      result = tbx_true;
    }
  PM2_LOG_OUT();

  return result;
}

tbx_bool_t
ntbx_pc_next_global_rank(p_ntbx_process_container_t pc,
			 p_ntbx_process_grank_t     global_rank)
{
  tbx_bool_t result = tbx_false;

  PM2_LOG_IN();
  if (pc->count)
    {
      (*global_rank)++;

      while (*global_rank < pc->global_array_size)
	{
	  if (pc->global_index[*global_rank])
	    {
	      result = tbx_true;
	      break;
	    }

	  (*global_rank)++;
	}
    }
  PM2_LOG_OUT();

  return result;
}


//
//  Topology tables
///////////////////////////////////////////////////////////////////////////
//
p_ntbx_topology_table_t
ntbx_topology_table_init(p_ntbx_process_container_t  pc,
			 ntbx_topology_t             topology,
			 void                       *parameter)
{
  p_ntbx_topology_table_t ttable = NULL;
  ntbx_process_lrank_t    size   =    0;

  PM2_LOG_IN();
  size = pc->local_array_size;

  while (size)
    {
      if (pc->local_index[size - 1])
	break;

      size--;
    }

  ttable = ntbx_topology_table_cons();
  ttable->size = size;

  if (size)
    {
      ttable->table =
	TBX_CALLOC(size * size, sizeof(p_ntbx_topology_element_t));

      switch (topology)
	{
	default:
	  TBX_FAILURE("unknown topology");
	  break;
	case ntbx_topology_empty:
	  break;
	case ntbx_topology_regular:
	  {
	    ntbx_process_lrank_t src = 0;

	    for (src = 0; src < size; src++)
	      {
		ntbx_process_lrank_t dst = 0;

		for (dst = 0; dst < size; dst++)
		  {
		    if (dst == src)
		      continue;

		    ttable->table[size * src + dst] =
		      ntbx_topology_element_cons();
		  }
	      }
	  }
	  break;
	case ntbx_topology_full:
	  {
	    ntbx_process_lrank_t src = 0;

	    for (src = 0; src < size; src++)
	      {
		ntbx_process_lrank_t dst = 0;

		for (dst = 0; dst < size; dst++)
		  {
		    ttable->table[size * src + dst] =
		      ntbx_topology_element_cons();
		  }
	      }
	  }
	  break;
	case ntbx_topology_star:
	  {
	    TBX_FAILURE("unimplemented");
	  }
	  break;
	case ntbx_topology_ring:
	  {
	    TBX_FAILURE("unimplemented");
	  }
	  break;
	case ntbx_topology_grid:
	  {
	    TBX_FAILURE("unimplemented");
	  }
	  break;
	case ntbx_topology_torus:
	  {
	    TBX_FAILURE("unimplemented");
	  }
	  break;
	case ntbx_topology_hypercube:
	  {
	    TBX_FAILURE("unimplemented");
	  }
	  break;
	case ntbx_topology_function:
	  {
            int (*f)(ntbx_process_lrank_t,  ntbx_process_lrank_t) = parameter;

	    ntbx_process_lrank_t src = 0;

	    for (src = 0; src < size; src++)
	      {
		ntbx_process_lrank_t dst = 0;

		for (dst = 0; dst < size; dst++)
		  {
                    if (f(src, dst))
                      {
                        ttable->table[size * src + dst] =
                          ntbx_topology_element_cons();
                      }
                  }
	      }
	  }
	  break;
	}
    }
  PM2_LOG_OUT();

  return ttable;
}

void
ntbx_topology_table_set(p_ntbx_topology_table_t ttable,
			ntbx_process_lrank_t    src,
			ntbx_process_lrank_t    dst)
{
  ntbx_process_lrank_t size = 0;

  PM2_LOG_IN();
  size = ttable->size;

  if ((src < 0) || (dst < 0))
    TBX_FAILURE("invalid parameters");

  if ((src >= size) || (dst >= size))
    TBX_FAILURE("out-of-bound parameters");

  if (!ttable->table[size * src + dst])
    {
      ttable->table[size * src + dst] =
	ntbx_topology_element_cons();
    }
  PM2_LOG_OUT();
}

void
ntbx_topology_table_clear(p_ntbx_topology_table_t ttable,
			  ntbx_process_lrank_t    src,
			  ntbx_process_lrank_t    dst)
{
  p_ntbx_topology_element_t element = NULL;
  ntbx_process_lrank_t      size    =    0;

  PM2_LOG_IN();
  size = ttable->size;

  if ((src < 0) || (dst < 0))
    TBX_FAILURE("invalid parameters");

  if ((src >= size) || (dst >= size))
    TBX_FAILURE("out-of-bound parameters");

  element = ttable->table[size * src + dst];
  if (element)
    {
      ntbx_topology_element_dest(element, NULL);

      element                         = NULL;
      ttable->table[size * src + dst] = NULL;
    }
  PM2_LOG_OUT();
}

p_ntbx_topology_element_t
ntbx_topology_table_get(p_ntbx_topology_table_t ttable,
			ntbx_process_lrank_t    src,
			ntbx_process_lrank_t    dst)
{
  p_ntbx_topology_element_t element = NULL;
  ntbx_process_lrank_t      size    =    0;

  PM2_LOG_IN();
  size = ttable->size;

  if ((src < 0) || (dst < 0))
    TBX_FAILURE("invalid parameters");

  if ((src >= size) || (dst >= size))
    TBX_FAILURE("out-of-bound parameters");

  element = ttable->table[size * src + dst];
  PM2_LOG_OUT();

  return element;
}

void
ntbx_topology_table_exit(p_ntbx_topology_table_t ttable)
{
  ntbx_process_lrank_t size = 0;

  PM2_LOG_IN();
  size = ttable->size;

  if (size)
    {
      p_ntbx_topology_element_t *ptr = NULL;

      ptr   = ttable->table;
      size *= size;

      while (size)
	{
	  if (*ptr)
	    {
	      ntbx_topology_element_dest(*ptr, NULL);
	      *ptr = NULL;
	    }

	  ptr++; size--;
	}
    }

  ntbx_topology_table_dest(ttable, NULL);
  PM2_LOG_OUT();
}


//
//  Host main name
///////////////////////////////////////////////////////////////////////////
//
char *
ntbx_true_name(char *host_name)
{
  struct hostent *host_entry = NULL;
  char           *result     = NULL;

  PM2_LOG_IN();
  if (!strcmp(host_name, "localhost"))
    {
      host_name = TBX_MALLOC(MAXHOSTNAMELEN + 1);
      gethostname(host_name, MAXHOSTNAMELEN);
      host_entry = gethostbyname(host_name);

      if (!host_entry)
	{
          TBX_FAILUREF("%s: %s\n", "gethostbyname", hstrerror(h_errno));
	  __TBX_PRINT_TRACE();
	  _TBX_EXIT_FAILURE();
	}

      TBX_FREE(host_name);
    }
  else
    {
      host_entry = gethostbyname(host_name);

      if (!host_entry)
	{
          TBX_FAILUREF("%s: %s\n", "gethostbyname", hstrerror(h_errno));
	  __TBX_PRINT_TRACE();
	  _TBX_EXIT_FAILURE();
	}
    }

  result = host_entry->h_name;
  PM2_LOG("ntbx_true_name: %s --> %s", host_name, result);

  {
    char	**ptr;
    size_t	  len;

    ptr	= host_entry->h_aliases;
    len	= strlen(result);

    while (*ptr) {

      if (!strncmp(result, *ptr, len) && ((*ptr)[len] == '.')) {
        PM2_LOG("  prefering %s over %s", *ptr, result);
        result	= *ptr;
        len	= strlen(result);
      }

      ptr++;
    }
  }

  result	= tbx_strdup(result);
  PM2_LOG_OUT();

  return result;
}
