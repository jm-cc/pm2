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
 * Mad_service_forward.c
 * =============
 */


/*
 * Headers
 * -------
 */

#include "madeleine.h"

mad_func_array_t forward_func_array = { start_forward,
                                        exit_forward,
                                        submit_forward,
                                        progress_forward,
                                        freeze_forward,
                                        restart_forward };

/*
 *  * Declarations
 *  * ------------
 *  */

mad_return_t
  start_forward ( p_mad_service_args_t args )
{
   LOG_IN();
   DISP("Forward Service started.");
   LOG_OUT();
   return NULL;
}


mad_return_t
  exit_forward  ( p_mad_service_args_t args )
{   
   LOG_IN();
   DISP("Forward Service stopped.");
   LOG_OUT();
   return NULL;
}


mad_return_t
  submit_forward ( p_mad_service_args_t args )
{
   LOG_IN();
   LOG_OUT();
   return NULL;
}


mad_return_t
  progress_forward ( p_mad_service_args_t args )
{
   LOG_IN();
   LOG_OUT();
   return NULL;
}


mad_return_t
  freeze_forward ( p_mad_service_args_t args )
{ 
   LOG_IN();
   LOG_OUT();
   return NULL;
}


mad_return_t
  restart_forward  ( p_mad_service_args_t args )
{
   LOG_IN();
   DISP("Forward Service restarted.");
   LOG_OUT();
   return NULL;
}

