
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
 * Mad_service_link.c
 * =============
 */


/*
 * Headers
 * -------
 */

#include "madeleine.h"




/*
 * Prototypes
 * ----------
 */


static p_mad_service_t link_service = NULL;

mad_func_array_t link_func_array = {start_link,
                                    exit_link,
                                    submit_link,
                                    progress_link,
                                    freeze_link,
                                    restart_link};

/*
 * Declarations
 * ------------
 */

mad_return_t 
start_link ( p_mad_service_args_t args )
{
   LOG_IN();
   link_service = args->service;
   link_service->active++;
   marcel_fprintf(stdout,"Link Service started. \n");
   LOG_OUT();
   return NULL;
}

mad_return_t 
exit_link  ( p_mad_service_args_t args )
{
   LOG_IN();
   link_service->active--;
   marcel_fprintf(stdout,"Link Service stopped. \n");
   LOG_OUT();
   return NULL;
}

mad_return_t
submit_link ( p_mad_service_args_t args )
{
   LOG_IN();
   LOG_OUT();
   return NULL;
}

mad_return_t
progress_link ( p_mad_service_args_t args )
{
   LOG_IN();
   LOG_OUT();
   return NULL;
}

mad_return_t
freeze_link ( p_mad_service_args_t args )
{
   LOG_IN();
   link_service->active--;
   LOG_OUT();
   return NULL;
}

mad_return_t
restart_link  ( p_mad_service_args_t args )
{
   LOG_IN();
   link_service->active++;
   marcel_fprintf(stdout,"Link service started !! \n");
   LOG_OUT();
   return NULL;
}

p_mad_link_t
mad_link_default_choice(p_mad_connection_t   connection,
			size_t               msg_size,
			p_mad_send_mode_t    send_mode    TBX_UNUSED,
			size_t               smode_size   TBX_UNUSED,
			p_mad_receive_mode_t receive_mode TBX_UNUSED,
			size_t               rmode_size   TBX_UNUSED) 
{ 
   p_mad_link_t lnk = NULL;
   
   LOG_IN();
   lnk = connection->link_array[0];
   LOG_OUT();
   
   return lnk;
}
