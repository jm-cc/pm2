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
 * Mad_communication.c
 * ===================
 */
#include "madeleine.h"

p_mad_connection_t
mad_begin_packing(p_mad_channel_t      channel,
		  ntbx_process_lrank_t remote_rank)
{
   p_mad_madeleine_t          madeleine       = NULL;
   p_mad_service_t            mad3_service    = NULL;
   p_mad_service_interface_t  mad3            = NULL;
   p_mad_connection_t         connection      = NULL;
   
   LOG_IN();
   madeleine    = mad_get_madeleine();
   mad3_service = mad_find_service(madeleine, "mad3_service");
   if( mad3_service )
     {
	mad3 = mad3_service->interface ;
	if(mad3)
	  {  
	     mad_service_args_t service_args;
	     mad_mad3_args_t    mad3_args = { mad_SEND,channel,remote_rank,
		  NULL,NULL,-1,-1,-1};
	     
	     service_args.private_args = &mad3_args ;
	     connection = (mad3->start_service)( &service_args );
	     LOG_OUT();
	     return connection;
	  }	
     }
   LOG_OUT();
   return NULL;
}

#ifdef MAD_MESSAGE_POLLING
p_mad_connection_t
mad_message_ready(p_mad_channel_t channel)
{
   p_mad_madeleine_t          madeleine       = NULL;
   p_mad_service_t            mad3_service    = NULL;
   p_mad_service_interface_t  mad3            = NULL;
   p_mad_connection_t         connection      = NULL;
    
   LOG_IN();
   madeleine    = mad_get_madeleine();
   mad3_service = mad_find_service(madeleine, "mad3_service");
   if( mad3_service )
     {
	mad3 = mad3_service->interface ;
	if(mad3)
	  {  
	     mad_service_args_t service_args;
	     mad_mad3_args_t    mad3_args = { NO_WAY,channel,
		  -1, NULL,NULL,-1,-1,-1};
	     
	     service_args.private_args = &mad3_args ;
	     connection = (mad3->progress)( &service_args );
	     LOG_OUT();
	     return connection;
	  }	
     }
   LOG_OUT();
   return NULL;
}
#endif /* MAD_MESSAGE_POLLING */

p_mad_connection_t
  mad_begin_unpacking(p_mad_channel_t channel)
{
   p_mad_madeleine_t          madeleine       = NULL;
   p_mad_service_t            mad3_service    = NULL;
   p_mad_service_interface_t  mad3            = NULL;
   p_mad_connection_t         connection      = NULL;
      
   LOG_IN();
   madeleine    = mad_get_madeleine();
   mad3_service = mad_find_service(madeleine, "mad3_service");
   if( mad3_service )
     {
	mad3 = mad3_service->interface ;
	if(mad3)
	  {  
	     mad_service_args_t service_args;
	     mad_mad3_args_t    mad3_args = { mad_RECV,channel,
		  -1,NULL,NULL,-1,-1,-1};
	     service_args.private_args = &mad3_args ;
	     connection = (mad3->start_service)( &service_args );
	     LOG_OUT();
	     return connection;
	  }	
     }
   LOG_OUT();
   return connection;
}

void
mad_end_packing(p_mad_connection_t connection)
{
   p_mad_madeleine_t          madeleine       = NULL;
   p_mad_service_t            mad3_service    = NULL;
   p_mad_service_interface_t  mad3            = NULL;
   
   LOG_IN();
   madeleine    = mad_get_madeleine();
   mad3_service = mad_find_service(madeleine, "mad3_service");
   if( mad3_service )
     {
	mad3 = mad3_service->interface ;
	if(mad3)
	  {  
	     mad_service_args_t service_args;
	     mad_mad3_args_t    mad3_args = { mad_SEND,NULL,-1, connection,
		  NULL,-1,-1,-1};
	     
	     service_args.private_args = &mad3_args ;
	     (mad3->exit_service)( &service_args );
	  }	
     }
   LOG_OUT();
}

void
mad_end_unpacking(p_mad_connection_t connection)
{
   p_mad_madeleine_t          madeleine       = NULL;
   p_mad_service_t            mad3_service    = NULL;
   p_mad_service_interface_t  mad3            = NULL;
   
   LOG_IN();
   madeleine    = mad_get_madeleine();
   mad3_service = mad_find_service(madeleine, "mad3_service");
   if( mad3_service )
     {
	mad3 = mad3_service->interface ;
	if(mad3)
	  {  
	     mad_service_args_t service_args;
	     mad_mad3_args_t    mad3_args = { mad_RECV,NULL,-1, connection,
		  NULL,-1,-1,-1};
	     
	     service_args.private_args = &mad3_args ;
	     (mad3->exit_service)( &service_args );
	  }	
     }
   LOG_OUT();
}

/*
 * mad_pack
 * --------
 */
void
mad_pack(p_mad_connection_t   connection,
	 void                *user_buffer,
	 size_t               user_buffer_length,
	 mad_send_mode_t      send_mode,
	 mad_receive_mode_t   receive_mode)
{
   p_mad_madeleine_t          madeleine       = NULL;
   p_mad_service_t            mad3_service    = NULL;
   p_mad_service_interface_t  mad3            = NULL;
   
   LOG_IN();
   madeleine    = mad_get_madeleine();
   mad3_service = mad_find_service(madeleine, "mad3_service");
   if( mad3_service )
     {
	mad3 = mad3_service->interface ;
	if(mad3)
	  {  
	     mad_service_args_t service_args;
	     mad_mad3_args_t    mad3_args = { mad_SEND,NULL,-1, connection,
		  user_buffer,user_buffer_length,send_mode,receive_mode};
	     
	     service_args.private_args = &mad3_args ;
	     (mad3->submit)( &service_args );
	  }	
     }
   LOG_OUT();
}

/*
 * mad_unpack
 * ----------
 */
void
mad_unpack(p_mad_connection_t   connection,
	   void                *user_buffer,
	   size_t               user_buffer_length,
	   mad_send_mode_t      send_mode,
	   mad_receive_mode_t   receive_mode)
{
   p_mad_madeleine_t          madeleine       = NULL;
   p_mad_service_t            mad3_service    = NULL;
   p_mad_service_interface_t  mad3            = NULL;
   
   LOG_IN();
   madeleine    = mad_get_madeleine();
   mad3_service = mad_find_service(madeleine, "mad3_service");
   if( mad3_service )
     {
	mad3 = mad3_service->interface ;
	if(mad3)
	  {  
	     mad_service_args_t service_args;
	     mad_mad3_args_t    mad3_args = { mad_RECV,NULL,-1, connection,
		  user_buffer,user_buffer_length,send_mode,receive_mode};
	     
	     service_args.private_args = &mad3_args ;
	     (mad3->submit)( &service_args );
	  }	
     }
   LOG_OUT();
}
