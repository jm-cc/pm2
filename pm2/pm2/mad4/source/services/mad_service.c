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
 * Mad_service.c
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

int
setup_service ( p_mad_service_args_t args,
		mad_func_vector_t    f_vector)
{
   int              index   = 0 ;
   p_mad_service_t  service = NULL;
   
   LOG_IN();
   service                   = args->service;
   service->type             = args->type;
   memcpy(service->name,args->name,SIZE_OFNAME);
   service->interface        = TBX_MALLOC(sizeof(mad_service_interface_t));
   service->active           = 0;
   service->attribute_table  = NULL;

   if (args->type == MOTOR_SERVICE)
     {
	service->specific    = TBX_MALLOC(sizeof(marcel_t));
     }
   else
     {	
	service->specific    = NULL;
     }
   service->interface->start_service = f_vector[index++];
   service->interface->exit_service  = f_vector[index++];
   service->interface->submit        = f_vector[index++];
   service->interface->progress      = f_vector[index++];
   service->interface->freeze        = f_vector[index++];
   service->interface->restart       = f_vector[index++];   
   LOG_OUT();
   return 0 ;
}

void initial_service( p_mad_service_t service )
{   
   mad_func_array_t     f_array; 
   mad_service_args_t   args;
   char                 name[SIZE_OFNAME] = "initial_factory";
   int                  index = -1;
   
   LOG_IN();  
   f_array[0] = start_initial_factory ;
   f_array[1] = exit_initial_factory ;
   for (index = 2 ; index < NB_FUNC_INTERFACE ; index++)
     {
	f_array[index] = factory_func_array[index];
     }
   
   service_args_initialize( &args, service, FACTORY_SERVICE, name, NULL, 0);
   setup_service( &args, f_array );

   LOG_OUT();
}

p_mad_service_t 
  mad_find_service( p_mad_madeleine_t madeleine, 
		    char *name )
{
   p_tbx_slist_t slist     = NULL; 
   p_mad_service_t service = NULL;
   LOG_IN();
   
   slist = madeleine->services_slist;
   
   if (!tbx_slist_is_nil(slist))
     {
	tbx_slist_ref_to_head(slist);
	do
	  {	
	     service = tbx_slist_ref_get( slist );
	     if (strcmp(name, service->name) == 0)
	       {
		  LOG_OUT();
		  return service;
	       }
	  }
	while (tbx_slist_ref_forward(slist));
     }
 
   LOG_OUT();
   return NULL;   
}

void
  mad_store_service( p_mad_madeleine_t madeleine, 
		     p_mad_service_t service)
{
   tbx_slist_enqueue(madeleine->services_slist,service);
}

void
  mad_remove_service( p_mad_madeleine_t madeleine, 
		      p_mad_service_t service)
{
   tbx_slist_search_and_extract(madeleine->services_slist,NULL,service) ; 
}

int 
  mad_test_service_name(p_mad_service_t service, char *name )
{    
   p_mad_madeleine_t madeleine    = NULL;
   p_mad_service_t   base_service = NULL;
   int res = -1;
   
   LOG_IN();
   madeleine    = mad_get_madeleine();
   base_service = mad_find_service(madeleine, name);
   
   if(service == base_service)
     {	
	res = 1;
     }
   else {	
      res = 0;
   }
   LOG_OUT();
   
   return res;
}



void 
  test_factory( p_mad_service_t service )
{
   fprintf(stdout,"Factory name : %s\n", service->name);
   (*service->interface->start_service)(NULL);
}

