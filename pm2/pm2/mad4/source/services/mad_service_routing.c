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
 * Mad_service_routing.c
 * =====================
 */


/*
 * Headers
 * -------
 */

#include "madeleine.h"

static p_mad_service_t routing_service = NULL;

mad_func_array_t routing_func_array = { start_routing,
                                        exit_routing,
                                        submit_routing,
                                        progress_routing,
                                        freeze_routing,
                                        restart_routing };

/*
 *  * Declarations
 *  * ------------
 *  */

mad_return_t
  start_routing ( p_mad_service_args_t args )
{
   LOG_IN();
   routing_service = args->service;
   routing_service->active++;
   marcel_fprintf(stdout,"Routing Service started. \n");
   LOG_OUT();
   return NULL;
}


mad_return_t
  exit_routing  ( p_mad_service_args_t args )
{   
   LOG_IN();
   routing_service->active--;
   marcel_fprintf(stdout,"Routing Service stopped.\n");
   LOG_OUT();
   return NULL;
}


mad_return_t
  submit_routing ( p_mad_service_args_t args )
{
   LOG_IN();
   LOG_OUT();
   return NULL;
}


mad_return_t
  progress_routing ( p_mad_service_args_t args )
{
   LOG_IN();
   LOG_OUT();
   return NULL;
}


mad_return_t
  freeze_routing ( p_mad_service_args_t args )
{ 
   LOG_IN();
   routing_service->active--;
   LOG_OUT();
   return NULL;
}


mad_return_t
  restart_routing  ( p_mad_service_args_t args )
{
   LOG_IN();
   routing_service->active++;
   marcel_fprintf(stdout,"Routing Service restarted !! \n");
   LOG_OUT();
   return NULL;
}

