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
 * Mad_service_stats.c
 * =============
 */


/*
 * Headers
 * -------
 */

#include "madeleine.h"

static p_mad_service_t stats_service = NULL;

mad_func_array_t stats_func_array = { start_stats,
                                      exit_stats,
                                      submit_stats,
                                      progress_stats,
                                      freeze_stats,
                                      restart_stats };

/*
 *  * Declarations
 *  * ------------
 *  */

mad_return_t
  start_stats ( p_mad_service_args_t args )
{
   LOG_IN();
   stats_service = args->service;
   stats_service->active++; 
   marcel_fprintf(stdout,"Stats Service started. \n");
   LOG_OUT();
   return NULL;
}


mad_return_t
  exit_stats  ( p_mad_service_args_t args )
{   
   LOG_IN();
   stats_service->active--; 
   marcel_fprintf(stdout,"Stats Service stopped.\n");
   LOG_OUT();
   return NULL;
}


mad_return_t
  submit_stats ( p_mad_service_args_t args )
{
   LOG_IN();
   LOG_OUT();
   return NULL;
}


mad_return_t
  progress_stats ( p_mad_service_args_t args )
{
   LOG_IN();
   LOG_OUT();
   return NULL;
}


mad_return_t
  freeze_stats ( p_mad_service_args_t args )
{ 
   LOG_IN();
   stats_service->active--; 
   LOG_OUT();
   return NULL;
}


mad_return_t
  restart_stats  ( p_mad_service_args_t args )
{
   LOG_IN();
   stats_service->active++; 
   marcel_fprintf(stdout,"Stats Service restarted !! \n");
   LOG_OUT();
   return NULL;
}

