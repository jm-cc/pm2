/*
 *    fprintf(stderr, "args 1 à %p 1\n", p_args);
 *    p_args->type    = type ;
 * 
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

void service_args_initialize( p_mad_service_args_t p_args,
			      p_mad_service_t      service,
			      mad_service_type_t   type,
			      char                 name[SIZE_OFNAME],
			      void                *ptr,
			      int                  nb)
{
   LOG_IN();
   p_args->service      = service;
   p_args->type         = type ;
   memcpy(p_args->name, name, SIZE_OFNAME);
   p_args->private_args = ptr;
   p_args->nb_args      = nb;
   LOG_OUT();
}

