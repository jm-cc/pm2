/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2003 "the PM2 team" (see AUTHORS file)
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
 * Mad_service_interface.h
 * ==============================
 */

#ifndef MAD_SERVICE_INTERFACE_H
#define MAD_SERVICE_INTERFACE_H


/* Interface generique d'un service */
struct s_mad_service_interface {
   
   /* démarre le service */
   mad_return_t
     (*start_service)( p_mad_service_args_t service_args );
   
   /* Termine l'execution du service */
   mad_return_t
     (*exit_service)( p_mad_service_args_t service_args );
   
   /* Appel au service */
   mad_return_t
     (*submit)( p_mad_service_args_t service_args );

   mad_return_t
     (*progress)( p_mad_service_args_t service_args );
   
   mad_return_t
     (*freeze)( p_mad_service_args_t service_args );
   
   mad_return_t
     (*restart)( p_mad_service_args_t service_args );
};

typedef struct s_mad_service_interface    mad_service_interface_t ;
typedef struct s_mad_service_interface *p_mad_service_interface_t ;

#endif /* MAD_SERVICE_INTERFACE_H */
