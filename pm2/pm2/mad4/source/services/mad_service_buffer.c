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
 * Mad_service_buffer.c
 * ====================
 */


/*
 * Headers
 * -------
 */

#include "madeleine.h"

static p_mad_service_t buffer_service = NULL;

mad_func_array_t buffer_func_array = { start_buffer,
                                       exit_buffer,
                                       submit_buffer,
                                       progress_buffer,
                                       freeze_buffer,
                                       restart_buffer };

/*
 *  * Declarations
 *  * ------------
 *  */

mad_return_t
  start_buffer ( p_mad_service_args_t args )
{
   LOG_IN();
   if(args)
     {  
	buffer_service = args->service;
	buffer_service->active++;
	marcel_fprintf(stdout,"Buffer Service started.\n");
     }
   else { 
      FAILURE("No args specified !");
     }
   LOG_OUT();
   return NULL;
}


mad_return_t
  exit_buffer  ( p_mad_service_args_t args )
{   
   LOG_IN();
   buffer_service->active--;
   marcel_fprintf(stdout,"Buffer Service stopped.\n");
   LOG_OUT();
   return NULL;
}


mad_return_t
  submit_buffer ( p_mad_service_args_t args )
{
   LOG_IN();
   LOG_OUT();
   return NULL;
}


mad_return_t
  progress_buffer ( p_mad_service_args_t args )
{
   LOG_IN();
   LOG_OUT();
   return NULL;
}


mad_return_t
  freeze_buffer ( p_mad_service_args_t args )
{ 
   LOG_IN();
   buffer_service->active--;
   LOG_OUT();
   return NULL;
}


mad_return_t
  restart_buffer  ( p_mad_service_args_t args )
{
   LOG_IN();
   buffer_service->active++;
   marcel_fprintf(stdout,"Buffer service restarted !! \n");
   LOG_OUT();
   return NULL;
}

