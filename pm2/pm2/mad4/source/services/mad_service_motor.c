
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
 * Mad_service_motor.c
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

static p_mad_service_t motor_service = NULL;

mad_func_array_t motor_func_array = {start_motor,
                                     exit_motor,
                                     submit_motor,
                                     progress_motor,
                                     freeze_motor,
                                     restart_motor};



/*
 * Declarations
 * ------------
 */

void motor_loop()
{
   while( 1 )
     {
	DISP("Motor check ...");
	marcel_yield();
     }
}


mad_return_t 
start_motor ( p_mad_service_args_t args )
{
   marcel_attr_t     attributes;
   
   LOG_IN();  
   motor_service = args->service;
   marcel_attr_init(&attributes);
   marcel_attr_setdetachstate(&attributes, TRUE);
   marcel_create ( (marcel_t *)(args->service->specific), &attributes,
		  (marcel_func_t) motor_loop, NULL);   
   /* init queues */
   motor_service->active++;
   DISP("Motor Service started.");
   LOG_OUT();
   return NULL;
}

mad_return_t 
exit_motor  ( p_mad_service_args_t args )
{
   LOG_IN();
   marcel_cancel(*((marcel_t *)(args->service->specific)));
   motor_service->active--;
   DISP("Motor Service stopped.");
   LOG_OUT();
   return NULL;
}

mad_return_t
submit_motor ( p_mad_service_args_t args )
{
   LOG_IN();
   LOG_OUT();
   return NULL;
}

mad_return_t
progress_motor ( p_mad_service_args_t args )
{
   LOG_IN();
   LOG_OUT();
   return NULL;
}

mad_return_t
freeze_motor ( p_mad_service_args_t args )
{
   LOG_IN();
   motor_service->active--;
   LOG_OUT();
   return NULL;
}

mad_return_t
restart_motor  ( p_mad_service_args_t args )
{
   LOG_IN();
   motor_service->active++;
   DISP("Motor Service restarted.");
   LOG_OUT();
   return NULL;
}
