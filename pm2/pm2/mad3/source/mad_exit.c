
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
 * Mad_exit.c
 * ==========
 */
/* #define TIMING */
#include "madeleine.h"

void
mad_close_channel(p_mad_channel_t channel)
{
  p_mad_madeleine_t          madeleine   = NULL;
  p_mad_session_t            session     = NULL;
  p_mad_adapter_t            adapter     = NULL;
  p_mad_driver_t             driver      = NULL;
  p_mad_driver_interface_t   interface   = NULL;
  p_tbx_darray_t             in_darray   = NULL;
  p_tbx_darray_t             out_darray  = NULL;  
  ntbx_process_lrank_t       size        =    0;
  ntbx_process_grank_t       global_rank =   -1;
  ntbx_process_lrank_t       local_rank  =   -1;
  ntbx_process_lrank_t       rank        =    0;

  LOG_IN();
  TBX_LOCK_SHARED(adapter);
  TBX_LOCK_SHARED(channel);

  adapter     = channel->adapter;
  driver      = adapter->driver;
  madeleine   = driver->madeleine;
  interface   = driver->interface;
  session     = madeleine->session;
  global_rank = session->process_rank;
  in_darray   = channel->in_connection_darray;
  out_darray  = channel->out_connection_darray;

  FAILURE("unimplemented");

  local_rank = ntbx_pc_global_to_local(channel->dir_channel->pc, global_rank);

  if (interface->before_close_channel)
    interface->before_close_channel(channel);

  if (interface->disconnect)
    {      
      if (driver->connection_type == mad_bidirectional_connection)
	{
	  for (rank = 0; rank < local_rank; rank++)
	    {
	      p_mad_connection_t connection = NULL;

	      connection = tbx_darray_get(in_darray, rank);
	      interface->disconnect(connection);
	    }
	  
	  for (rank = local_rank + 1; rank < size; rank++)
	    {
	      p_mad_connection_t connection = NULL;

	      connection = tbx_darray_get(out_darray, rank);
	      interface->disconnect(connection);
	    }
	}
      else
	{
	  for (rank = 0; rank < size; rank++)
	    {
	      if (rank == local_rank)
		{
		  ntbx_process_lrank_t remote_rank = -1;

		  for (remote_rank = 0 ; remote_rank < size; remote_rank++)
		    {
		      if (remote_rank != local_rank)
			{
			  p_mad_connection_t connection = NULL;
			  
			  connection =
			    tbx_darray_get(in_darray, remote_rank);
			  interface->disconnect(connection);
			}
		    }
		}
	      else
		{
		  p_mad_connection_t connection = NULL;

		  connection = tbx_darray_get(out_darray, rank);
		  interface->disconnect(connection);
		}
	    }
	}
    }

  if (interface->after_close_channel)
    interface->after_close_channel(channel);

  for (rank = 0; rank < size; rank++)
    {
      p_mad_connection_t in      = NULL;
      p_mad_connection_t out     = NULL;
      mad_link_id_t      link_id =   -1;

      in  = tbx_darray_get(in_darray, rank);
      out = tbx_darray_get(out_darray, rank);

      if (interface->link_exit)
	{
	  for (link_id = 0; link_id < in->nb_link; link_id++)
	    {
	      p_mad_link_t in_link  = in->link_array[link_id];
	      p_mad_link_t out_link = out->link_array[link_id];
	      
	      interface->link_exit(out_link);
	      interface->link_exit(in_link);

	      TBX_FREE(in_link);
	      in->link_array[link_id] = NULL;
	      TBX_FREE(out_link);
	      out->link_array[link_id] = NULL;
	    }
	}
      else
	{
	  for (link_id = 0; link_id < in->nb_link; link_id++)
	    {
	      p_mad_link_t in_link  = in->link_array[link_id];
	      p_mad_link_t out_link = out->link_array[link_id];
	      
	      if (out_link->specific)
		{
		  TBX_FREE(out_link->specific);
		  out_link->specific = NULL;
		}

	      if (in_link->specific)
		{
		  TBX_FREE(in_link->specific);
		  in_link->specific = NULL;
		}

	      TBX_FREE(in_link);
	      in->link_array[link_id] = NULL;
	      TBX_FREE(out_link);
	      out->link_array[link_id] = NULL;
	    }
	}
      
      TBX_FREE(out->link_array);
      out->link_array = NULL;
      
      TBX_FREE(in->link_array);
      in->link_array = NULL;
      
      if (interface->connection_exit)
	{
	  interface->connection_exit(in, out);
	}
      else
	{
	  if (out->specific)
	    {	      
	      TBX_FREE(out->specific);

	      if (in->specific == out->specific)
		{
		  in->specific = NULL;  
		}

	      out->specific = NULL;
	    }

	  if (in->specific)
	    {
	      TBX_FREE(in->specific);
	      in->specific = NULL;
	    }	  
	}

      TBX_FREE(in);
      tbx_darray_set(in_darray, rank, NULL);
      TBX_FREE(out);
      tbx_darray_set(out_darray, rank, NULL);
    }

  if (interface->channel_exit)
    {
      interface->channel_exit(channel);
    }
  else
    {
      if (channel->specific)
	{
	  TBX_FREE(channel->specific);
	  channel->specific = NULL;
	}
    }
  
  TBX_FREE(channel);
  channel = NULL;
  
  /* Note: the channel is never unlocked since it is being destroyed */
  TBX_UNLOCK_SHARED(adapter);
  LOG_OUT();  
}

static
void
mad_leonie_sync(p_mad_madeleine_t madeleine)
{
  p_mad_session_t session = NULL;
  p_ntbx_client_t client  = NULL;
  int             data    =    0;

  LOG_IN();
  TRACE("Termination sync");  
  session = madeleine->session;
  client  = session->leonie_link;

  mad_ntbx_send_int(client, mad_leo_command_end);
  data = mad_ntbx_receive_int(client);
  if (data != 1)
    FAILURE("synchronization error");

  mad_dir_vchannel_exit(madeleine);
  LOG_OUT();
}

void
mad_exit(p_mad_madeleine_t madeleine)
{
  LOG_IN();
  mad_leonie_sync(madeleine);
  LOG_OUT();

#if 0
  p_mad_session_t    session        = NULL;
  p_ntbx_client_t    client         = NULL;
  p_tbx_htable_t     channel_htable = NULL;

  LOG_IN();
  channel_htable = madeleine->channel_htable;
  session        = madeleine->session;
  client         = session->leonie_link;
  
  mad_ntbx_send_int(client, session->process_rank);

  while (tbx_htable_get_size(channel_htable))
    {
      p_mad_channel_t  channel = NULL;
      char            *name    = NULL;
      int              rank    =   -1;

      name = mad_ntbx_receive_string(client);

      channel = tbx_htable_extract(channel_htable, name);
      if (!channel)
	FAILURE("channel not found");
      
      rank = session->process_rank;
      mad_close_channel(channel);

      mad_ntbx_send_int(client, rank);
    }

  ntbx_tcp_client_disconnect(client);
  LOG_OUT();
#endif // 0
}
