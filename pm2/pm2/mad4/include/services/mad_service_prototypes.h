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
 * Service_prototypes.h
 * ==============================
 */

#ifndef SERVICE_PROTOTYPES_H
#define SERVICE_PROTOTYPES_H

/* Generic Part */

void service_args_initialize( p_mad_service_args_t p_args,
			      p_mad_service_t      service,
			      mad_service_type_t   type,
			      char                 name[SIZE_OFNAME],
			      void                *ptr,
			      int                  nb);
int             setup_service   ( p_mad_service_args_t args,
				  mad_func_array_t f_array );
void            inital_service  ( p_mad_service_t service );
void            test_factory    ( p_mad_service_t service );
p_mad_service_t mad_find_service( p_mad_madeleine_t madeleine, 
				  char *name );
void            mad_store_service( p_mad_madeleine_t madeleine,
				   p_mad_service_t service);
void            mad_remove_service( p_mad_madeleine_t madeleine,
				    p_mad_service_t service);    
int
                mad_test_service_name(p_mad_service_t service, 
				      char *name);
extern mad_func_vectors_t func_vectors;

/* Buffer Service */

mad_return_t start_buffer    ( p_mad_service_args_t args );
mad_return_t exit_buffer     ( p_mad_service_args_t args );
mad_return_t submit_buffer   ( p_mad_service_args_t args );
mad_return_t progress_buffer ( p_mad_service_args_t args );
mad_return_t freeze_buffer   ( p_mad_service_args_t args );
mad_return_t restart_buffer  ( p_mad_service_args_t args );
extern mad_func_array_t buffer_func_array;

/* Factory Service */
mad_return_t start_initial_factory 
                              ( p_mad_service_args_t args );
mad_return_t exit_initial_factory 
                              ( p_mad_service_args_t args );
mad_return_t start_factory    ( p_mad_service_args_t args );
mad_return_t exit_factory     ( p_mad_service_args_t args );
mad_return_t submit_factory   ( p_mad_service_args_t args );
mad_return_t progress_factory ( p_mad_service_args_t args );
mad_return_t freeze_factory   ( p_mad_service_args_t args );
mad_return_t restart_factory  ( p_mad_service_args_t args );
extern mad_func_array_t factory_func_array;

/* Forward Service */

mad_return_t start_forward    ( p_mad_service_args_t args );
mad_return_t exit_forward     ( p_mad_service_args_t args );
mad_return_t submit_forward   ( p_mad_service_args_t args );
mad_return_t progress_forward ( p_mad_service_args_t args );
mad_return_t freeze_forward   ( p_mad_service_args_t args );
mad_return_t restart_forward  ( p_mad_service_args_t args );
extern mad_func_array_t forward_func_array;

/* Link Service */

mad_return_t start_link    ( p_mad_service_args_t args );
mad_return_t exit_link     ( p_mad_service_args_t args );
mad_return_t submit_link   ( p_mad_service_args_t args );
mad_return_t progress_link ( p_mad_service_args_t args );
mad_return_t freeze_link   ( p_mad_service_args_t args );
mad_return_t restart_link  ( p_mad_service_args_t args );
extern mad_func_array_t link_func_array;

p_mad_link_t
  mad_link_default_choice(p_mad_connection_t   connection,
			  size_t               msg_size,
			  p_mad_send_mode_t    send_mode    TBX_UNUSED,
			  size_t               smode_size   TBX_UNUSED,
			  p_mad_receive_mode_t receive_mode TBX_UNUSED,
			  size_t               rmode_size   TBX_UNUSED);
/* Motor Service */

mad_return_t start_motor    ( p_mad_service_args_t args );
mad_return_t exit_motor     ( p_mad_service_args_t args );
mad_return_t submit_motor   ( p_mad_service_args_t args );
mad_return_t progress_motor ( p_mad_service_args_t args );
mad_return_t freeze_motor   ( p_mad_service_args_t args );
mad_return_t restart_motor  ( p_mad_service_args_t args );
extern mad_func_array_t motor_func_array;

/* Network Service */

mad_return_t start_network    ( p_mad_service_args_t args );
mad_return_t exit_network     ( p_mad_service_args_t args );
mad_return_t submit_network   ( p_mad_service_args_t args );
mad_return_t progress_network ( p_mad_service_args_t args );
mad_return_t freeze_network   ( p_mad_service_args_t args );
mad_return_t restart_network  ( p_mad_service_args_t args );
extern mad_func_array_t network_func_array;

/* Routing Service */

mad_return_t start_routing    ( p_mad_service_args_t args );
mad_return_t exit_routing    ( p_mad_service_args_t args );
mad_return_t submit_routing   ( p_mad_service_args_t args );
mad_return_t progress_routing ( p_mad_service_args_t args );
mad_return_t freeze_routing   ( p_mad_service_args_t args );
mad_return_t restart_routing  ( p_mad_service_args_t args );
extern mad_func_array_t routing_func_array;

/* Stats Service */

mad_return_t start_stats    ( p_mad_service_args_t args );
mad_return_t exit_stats     ( p_mad_service_args_t args );
mad_return_t submit_stats   ( p_mad_service_args_t args );
mad_return_t progress_stats ( p_mad_service_args_t args );
mad_return_t freeze_stats   ( p_mad_service_args_t args );
mad_return_t restart_stats  ( p_mad_service_args_t args );
extern mad_func_array_t stats_func_array;

/* User Service */

mad_return_t start_user    ( p_mad_service_args_t args );
mad_return_t exit_user     ( p_mad_service_args_t args );
mad_return_t submit_user   ( p_mad_service_args_t args );
mad_return_t progress_user ( p_mad_service_args_t args );
mad_return_t freeze_user   ( p_mad_service_args_t args );
mad_return_t restart_user  ( p_mad_service_args_t args );

/* mad3 Service */

mad_return_t start_mad3    ( p_mad_service_args_t args );
mad_return_t exit_mad3     ( p_mad_service_args_t args );
mad_return_t submit_mad3   ( p_mad_service_args_t args );
mad_return_t progress_mad3 ( p_mad_service_args_t args );
mad_return_t freeze_mad3   ( p_mad_service_args_t args );
mad_return_t restart_mad3  ( p_mad_service_args_t args );
extern mad_func_array_t mad3_func_array;

#endif /* SERVICE_PROTOTYPES_H */
