
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
 * Mad_service_mad3.c
 * =============
 */


/*
 * Headers
 * -------
 */

#include "madeleine.h"


/*
 * Declarations
 * ------------
 */

static p_mad_service_t mad3_service = NULL;

mad_func_array_t mad3_func_array    = { start_mad3,
                                        exit_mad3,
                                        submit_mad3,
                                        progress_mad3,
                                        freeze_mad3,
                                        restart_mad3 };

/*
 * Declarations
 * ------------
 */


/* mad_begin_packing/unpacking */
mad_return_t 
start_mad3 ( p_mad_service_args_t args )
{
   p_mad_connection_t   connection  = NULL;
   
   LOG_IN();
   
   if (args == NULL)
     {	
        FAILURE("mad3 start : No args specified !");
     }
   else
     {
	if (mad3_service == NULL)
	  {
	     mad3_service = args->service ;
	     mad3_service->active++;
	     DISP("Mad3 Service started.");
	  }
	else
	  {
	     if(mad3_service->active)
	       {  
		  p_mad_mad3_args_t    mad_args    = (p_mad_mad3_args_t)args->private_args;
		  mad_way_t            way         = mad_args->way;
		  p_mad_channel_t      channel     = mad_args->channel;
		  
		  if( way == mad_SEND )
		    {
		       ntbx_process_lrank_t remote_rank = mad_args->remote_rank;	
		       connection = mad3_begin_packing( channel, remote_rank);
		       mad_args->connection = connection ;
		    }
		  else if (way == mad_RECV )
		    {
		       connection = mad3_begin_unpacking( channel );
		       mad_args->connection = connection ;
		    }
		  else {
		     FAILURE("Invalid mode");
		  }  
	       }
	  }	
     }
   LOG_OUT();
   return connection;
}

/* mad_end_packing/unpacking */
mad_return_t 
exit_mad3  ( p_mad_service_args_t args )
{
   LOG_IN();
   if(mad3_service->active)
     {
	if (args == NULL)
	  {	
	     mad3_service->active--;
	     DISP("Mad3 service stopped.");
	  }
	else
	  {
	     p_mad_mad3_args_t  mad_args   = (p_mad_mad3_args_t)args->private_args;
	     mad_way_t          way        = mad_args->way;
	     p_mad_connection_t connection = mad_args->connection;
	     
	     if( way == mad_SEND )
	       {
		  mad3_end_packing ( connection);
	       }
	     else if ( way == mad_RECV )
	       {
		  mad3_end_unpacking ( connection);
	       }
	     else {
		  FAILURE("Invalid mode");
	       }  
	  }
     }
   LOG_OUT();
   return NULL;
}

/* mad_pack/unpack */
mad_return_t
submit_mad3 ( p_mad_service_args_t args )
{
   LOG_IN();
   if (args == NULL)
     {	
	FAILURE("NULL args pointer");
     }
   else
     {
	p_mad_mad3_args_t mad_args = (p_mad_mad3_args_t)args->private_args;
   	mad_way_t           way                = mad_args->way;
	p_mad_connection_t  connection         = mad_args->connection;
	void               *user_buffer        = mad_args->user_buffer;
	size_t              user_buffer_length = mad_args->user_buffer_length;
	mad_send_mode_t     send_mode          = mad_args->send_mode;
	mad_receive_mode_t  receive_mode       = mad_args->receive_mode;
       
	if( way == mad_SEND )
	  {
	     mad3_pack (connection,
		       user_buffer,
		       user_buffer_length,
		       send_mode,
		       receive_mode);
	  }
	else if ( way == mad_RECV )
	  {
	     mad3_unpack (connection,
			 user_buffer,
			 user_buffer_length,
			 send_mode,
			 receive_mode);
	  }
	else
	  {
	     FAILURE("Invalid mode");
	  }  
     }
   
   LOG_OUT();
   return NULL;
}

mad_return_t
progress_mad3 ( p_mad_service_args_t args )
{
   LOG_IN();
   p_mad_connection_t connection = NULL;
   
   if (args == NULL)
     {	
	FAILURE("NULL args pointer");
     }
   else
     {
#ifdef MAD_MESSAGE_POLLING      
	p_mad_mad3_args_t  mad_args  = (p_mad_mad3_args_t)args->private_args;
        p_mad_channel_t channel      = mad_args->channel;
	connection                   = mad3_message_ready( channel );
	mad_args->connection         = connection ;
#endif
     }
   LOG_OUT();
   return connection;
}

mad_return_t
  freeze_mad3 ( p_mad_service_args_t args )
{
   LOG_IN();
   LOG_OUT();
   return NULL;
}

mad_return_t
restart_mad3 ( p_mad_service_args_t args )
{
   LOG_IN();
   fprintf(stdout,"Restarting mad3 service ...\n");
   LOG_OUT();
   return NULL;
}

