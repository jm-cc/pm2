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
 * mad_fault.c
 * ==========
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "pm2_common.h"

//#define NEW

int
main(int    argc,
     char **argv)
{
   p_mad_madeleine_t          madeleine       = NULL;
   p_mad_session_t            session         = NULL;
   p_mad_channel_t            channel         = NULL;
   p_ntbx_process_container_t pc              = NULL;
   ntbx_process_grank_t       process_rank    = -1;
   ntbx_process_grank_t       next_rank       = -1;
   char                       *name           = NULL;
   int                        session_size;
   char                       *buffer;
   char                       *token;
   char                       *my_token       = "TOKEN1";
   char                       *my_token2      = "TOKEN2";
   int                        error           = 0; 
   int                        count           = 15;  
   
#ifdef NEW
   /* New interface */
   p_mad_service_t            mad3_service    = NULL;
   p_mad_service_interface_t  mad3            = NULL;
#endif
   
   common_init(&argc, argv, NULL); 
   
   madeleine    = mad_get_madeleine();
   session      = madeleine->session;
   tbx_slist_ref_to_head(madeleine->public_channel_slist);
   name         = tbx_slist_ref_get(madeleine->public_channel_slist);
   channel      = tbx_htable_get(madeleine->channel_htable, name);
   process_rank = session->process_rank;
   session_size = tbx_slist_get_length( madeleine->dir->process_slist );
   
   next_rank = 1;  
   token     = my_token;
   
#ifdef NEW   
   /* New interface */ 
   mad3_service = mad_find_service(madeleine, "mad3_service");
   if( mad3_service )
     {	
	mad3 = mad3_service->interface ;
	if(mad3)
	  {
	     (mad3->restart)(NULL);
	  }
     }
#endif
  while(count--)
    { 
#ifdef NEW
       mad_service_args_t  args;
#endif
       p_mad_connection_t  in_connection  = NULL;
       p_mad_connection_t  out_connection = NULL;
       

       if ( process_rank ==  0 )
	{
#ifdef NEW
	   mad_mad3_args_t  out_args = {mad_SEND,channel,next_rank,out_connection,token,
		strlen(token),mad_send_CHEAPER,mad_receive_CHEAPER};	   	
	   args.private_args = &out_args ;	   
#endif
	   DISP("Hello from process %i, next = %i",process_rank, next_rank);
	   //marcel_delay(1000);
	   
	   DISP("BEGIN PACKING");
#ifdef NEW
	   (mad3->start_service)( &args );
#else
	   out_connection = mad_begin_packing( channel, next_rank);
	   if(! out_connection)
	     {
		DISP("Error in begin packing");
	     }
	   
#endif
	   DISP("PACKING");
#ifdef NEW
	   (mad3->submit)( &args );
#else
	   mad_pack (out_connection,
		     token,
		     strlen(token),
		     mad_send_CHEAPER,
		     mad_receive_CHEAPER);
#endif	   
	   DISP("END PACKING");
#ifdef NEW
	   (mad3->exit_service)( &args );
#else
	   mad_end_packing ( out_connection );
#endif
	}
      else
	{
#ifdef NEW
	   mad_mad3_args_t  in_args = {mad_RECV,channel,next_rank,in_connection,NULL,
		strlen(token),mad_send_CHEAPER,mad_receive_CHEAPER};	   	
	   args.private_args = &in_args ;	   
#endif
	   buffer = (char *)malloc(strlen(token)*sizeof(char));
#ifdef NEW
	   in_args.user_buffer = buffer; 
#endif
	   DISP("BEGIN unPACKING");
#ifdef NEW
	   in_connection = (p_mad_connection_t)(mad3->start_service)( &args );
#else
	   in_connection = mad_begin_unpacking( channel );
#endif
	   if(! in_connection)
	     {
		DISP("Error in begin UNpacking");
	     }
	   
	   DISP("unPACKING");
#ifdef NEW
	   (mad3->submit)( &args );
#else
	   mad_unpack (in_connection,
		       buffer,
		       strlen(token),
		       mad_send_CHEAPER,
		       mad_receive_CHEAPER);
#endif
	   
	   DISP("END unPACKING");
#ifdef NEW
	   (mad3->exit_service)( &args );
#else
	   mad_end_unpacking ( in_connection );
#endif
	   DISP("Got Token : %s", buffer);
	   free(buffer);
	   //marcel_delay(1000);
	}
    }
  common_exit(NULL);
  return 0;
}


