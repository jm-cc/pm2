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
 * Mad_service_network.c
 * ====================
 */


/*
 * Headers
 * -------
 */

#include "madeleine.h"

static p_mad_service_t network_service = NULL;

mad_func_array_t network_func_array = { start_network,
                                        exit_network,
                                        submit_network,
                                        progress_network,
                                        freeze_network,
                                        restart_network };

/*
 *  * Declarations
 *  * ------------
 *  */

mad_return_t
  start_network ( p_mad_service_args_t args )
{
   LOG_IN();
   network_service = args->service;
   network_service->active++;
   DISP("Network service started.");
   LOG_OUT();
   return NULL;
}


mad_return_t
  exit_network  ( p_mad_service_args_t args )
{   
   LOG_IN();
   network_service->active--;
   DISP("Network service stopped.");
   LOG_OUT();
   return NULL;
}


mad_return_t
  submit_network ( p_mad_service_args_t args )
{
   LOG_IN();
   LOG_OUT();
   return NULL;
}


mad_return_t
  progress_network ( p_mad_service_args_t args )
{
   LOG_IN();
   LOG_OUT();
   return NULL;
}


mad_return_t
  freeze_network ( p_mad_service_args_t args )
{ 
   LOG_IN();
   network_service->active--;
   LOG_OUT();
   return NULL;
}


mad_return_t
  restart_network  ( p_mad_service_args_t args )
{
   LOG_IN();
   network_service->active++;
   DISP("Network service restarted.");
   LOG_OUT();
   return NULL;
}

