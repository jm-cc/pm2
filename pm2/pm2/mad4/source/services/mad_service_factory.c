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
 * Mad_service_factory.c
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


static p_mad_service_t initial_factory_service = NULL;

mad_func_array_t factory_func_array = { start_factory,
                                        exit_factory,
                                        submit_factory,
                                        progress_factory,
                                        freeze_factory,  
                                        restart_factory } ;

/*
 *  GM Note :
 *  the three latter arrays must be indexed
 *  in the same order ...
 *  */

mad_service_names_t service_names =
{   
   "initial_factory",
   "motor_service",
   "mad3_service",
   "link_service",
   "buffer_service",
   "network_service",
   "routing_service",
   "forward_service",
   "stats_service"
};

mad_service_array_t service_types =
{
   FACTORY_SERVICE,
   MOTOR_SERVICE,
   MAD3_SERVICE,
   LINK_SERVICE,
   BUFFER_SERVICE ,
   NETWORK_SERVICE ,
   ROUTING_SERVICE,
   FORWARD_SERVICE,
   STATS_SERVICE,
};

mad_func_vectors_t func_vectors =
{
   factory_func_array,
   motor_func_array,
   mad3_func_array,
   link_func_array,
   buffer_func_array,
   network_func_array,
   routing_func_array,
   forward_func_array,
   stats_func_array
};

/*****************************************************************/

mad_return_t
start_initial_factory ( p_mad_service_args_t args )
{  
   mad_service_args_t internal_args;	
   p_mad_service_t    dummy_service[SERVICE_NUMBER]; 
   int index;  
   
   LOG_IN();
   initial_factory_service = args->service;
   initial_factory_service->active++;
   
   for(index = 1; index < SERVICE_NUMBER ; index ++)
     {
	dummy_service[index] = TBX_MALLOC(sizeof(mad_service_t));
	service_args_initialize( &internal_args, 
				 dummy_service[index], 
				 service_types[index],
				 service_names[index],
				  func_vectors[index], 1);
	(*initial_factory_service->interface->submit)( &internal_args );	
     }
   
   DISP("Initial Factory Service started.");   
   LOG_OUT();
   return NULL;   
}

mad_return_t
exit_initial_factory ( p_mad_service_args_t args )
{
   p_mad_madeleine_t  madeleine = NULL;
   p_mad_service_t    service   = NULL;
   int index ;
   
   LOG_IN();
   madeleine = mad_get_madeleine();

   for(index = SERVICE_NUMBER - 1 ; index > 1 ; index--)  
     {
	service = mad_find_service(madeleine, service_names[index]);
	mad_remove_service(madeleine, service);
     }
   initial_factory_service->active--;
   DISP("Initial Factory Service stopped.");
   LOG_OUT();
   return NULL;
}


mad_return_t
start_factory ( p_mad_service_args_t args )
{
   LOG_IN();
   DISP("Starting Factory");
   LOG_OUT();
   return NULL;
}


mad_return_t
exit_factory ( p_mad_service_args_t args )
{
   LOG_IN();
   DISP("Factory service stopped");
   LOG_OUT();
   return NULL;
}


mad_return_t
submit_factory ( p_mad_service_args_t args )
{
   p_mad_madeleine_t  madeleine = NULL;
   mad_func_vector_t  f_vector  = NULL;
   
   LOG_IN();
   madeleine = mad_get_madeleine();
   
   switch( args->type )
     {
      case BUFFER_SERVICE :
	  {  
	     f_vector = (mad_func_vector_t) buffer_func_array;;	 
	  }
	break;
      case FACTORY_SERVICE :	
      case USER_SERVICE :
	  {	     
	     f_vector = (mad_func_vector_t) args->private_args;	     
	  }
	break ;
      case FORWARD_SERVICE :
	  {  
	     f_vector = (mad_func_vector_t) forward_func_array;	 
	  }
	break;
      case LINK_SERVICE :
	  {
	     f_vector = (mad_func_vector_t) link_func_array;
	  }
	break;
      case MAD3_SERVICE :
	  {
	     f_vector = (mad_func_vector_t) mad3_func_array;
	  }
	break;
      case MOTOR_SERVICE :
	  {
	     f_vector = (mad_func_vector_t) motor_func_array;
	  }
	break;
      case NETWORK_SERVICE :
	  {  
	     f_vector = (mad_func_vector_t) network_func_array;;	 
	  }
	break;
      case ROUTING_SERVICE :
	  {  
	     f_vector = (mad_func_vector_t) routing_func_array;;	 
	  }
	break;
      case STATS_SERVICE :
	  {  
	     f_vector = (mad_func_vector_t) stats_func_array;;	 
	  }
	break;
	   default:
	FAILURE("unknown service type");
     }
   setup_service(args, f_vector);
   mad_store_service(madeleine, args->service);
   
   LOG_OUT();
   return NULL;
}

mad_return_t
progress_factory ( p_mad_service_args_t args )
{
   LOG_IN();
   LOG_OUT();
   return NULL;
}


mad_return_t
freeze_factory ( p_mad_service_args_t args )
{
   LOG_IN();
   
   LOG_OUT();
   return NULL;
}

mad_return_t
restart_factory( p_mad_service_args_t args )
{
   LOG_IN();
   LOG_OUT();
   return NULL;
}

