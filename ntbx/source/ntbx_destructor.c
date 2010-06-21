
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
 * Ntbx_destructor.c
 * ==================
 */

#include "tbx.h"
#include "ntbx.h"

void
ntbx_client_dest_ext(p_ntbx_client_t            object,
		     p_tbx_specific_dest_func_t dest_func)
{
  PM2_LOG_IN();
  if (object->local_host)
    {
      TBX_FREE(object->local_host);
      object->local_host = NULL;
    }

  if (object->local_alias)
    {
      tbx_slist_free(object->local_alias);
      object->local_alias = NULL;
    }

  if (object->remote_host)
    {
      TBX_FREE(object->remote_host);
      object->remote_host = NULL;
    }

  if (object->remote_alias)
    {
      tbx_slist_free(object->remote_alias);
      object->remote_alias = NULL;
    }

  if (object->specific)
    {
      if (dest_func)
	{
	  dest_func(object->specific);
	}
      else
	TBX_FAILURE("don't know how to destroy specific field");

      object->specific = NULL;
    }

  TBX_FREE(object);
  PM2_LOG_OUT();
}

void
ntbx_client_dest(p_ntbx_client_t object)
{
  PM2_LOG_IN();
  ntbx_client_dest_ext(object, NULL);
  PM2_LOG_OUT();
}

void
ntbx_server_dest_ext(p_ntbx_server_t            object,
		     p_tbx_specific_dest_func_t dest_func)
{
  PM2_LOG_IN();
  if (object->local_host)
    {
      TBX_FREE(object->local_host);
      object->local_host = NULL;
    }

  if (object->local_alias)
    {
      tbx_slist_free(object->local_alias);
      object->local_alias = NULL;
    }

  object->connection_data.data[0] = '\0';

  if (object->specific)
    {
      if (dest_func)
	{
	  dest_func(object->specific);
	}
      else
	TBX_FAILURE("don't know how to destroy specific field");

      object->specific = NULL;
    }

  TBX_FREE(object);
  PM2_LOG_OUT();
}

void
ntbx_server_dest(p_ntbx_server_t object)
{
  PM2_LOG_IN();
  ntbx_server_dest_ext(object, NULL);
  PM2_LOG_OUT();
}


void
ntbx_process_info_dest(p_ntbx_process_info_t      object,
		       p_tbx_specific_dest_func_t dest_func)
{
  PM2_LOG_IN();
  object->local_rank =   -1;
  object->process    = NULL;

  if (object->specific)
    {
      if (dest_func)
	{
	  dest_func(object->specific);
	}
      else
	TBX_FAILURE("don't know how to destroy specific field");

      object->specific = NULL;
    }

  TBX_FREE(object);
  PM2_LOG_OUT();
}

void
ntbx_pc_dest(p_ntbx_process_container_t object,
	     p_tbx_specific_dest_func_t dest_func)
{
  PM2_LOG_IN();
  while (object->local_array_size--)
    {
      if (object->local_index[object->local_array_size])
	{
	  ntbx_process_info_dest(object->
				 local_index[object->local_array_size],
				 dest_func);
	  object->local_index[object->local_array_size] = NULL;
	}
    }

  if (object->local_index)
    {
      TBX_FREE(object->local_index);
      object->local_index = NULL;
    }

  if (object->global_index)
    {
      memset(object->global_index, 0, (size_t)object->global_array_size);

      TBX_FREE(object->global_index);
      object->global_index = NULL;
    }

  object->global_array_size = 0;
  object->count             = 0;

  TBX_FREE(object);
  PM2_LOG_OUT();
}

void
ntbx_process_dest(p_ntbx_process_t           object,
		  p_tbx_specific_dest_func_t dest_func)
{
  PM2_LOG_IN();
  object->global_rank = -1;

  object->ref         = NULL;

  if (object->specific)
    {
      if (dest_func)
	{
	  dest_func(object->specific);
	}
      else
	TBX_FAILURE("don't know how to destroy specific field");

      object->specific = NULL;
    }

  TBX_FREE(object);
  PM2_LOG_OUT();
}

void
ntbx_topology_element_dest(p_ntbx_topology_element_t  object,
			   p_tbx_specific_dest_func_t dest_func)
{
  PM2_LOG_IN();
  if (object->specific)
    {
      if (dest_func)
	{
	  dest_func(object->specific);
	}
      else
	TBX_FAILURE("don't know how to destroy specific field");

      object->specific = NULL;
    }

  TBX_FREE(object);
  PM2_LOG_OUT();
}

void
ntbx_topology_table_dest(p_ntbx_topology_table_t    object,
			 p_tbx_specific_dest_func_t dest_func TBX_UNUSED)
{
  PM2_LOG_IN();
  if (object->table)
    {
      TBX_FREE(object->table);
      object->table = NULL;
    }

  object->size = 0;

  TBX_FREE(object);
  PM2_LOG_OUT();
}
