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
 * Mad_service_args.h
 * ==============================
 */

#ifndef MAD_SERVICE_ARGS_H
#define MAD_SERVICE_ARGS_H

#define NB_FUNC_INTERFACE 6

/* global structure */
typedef struct s_mad_service_args {
   struct s_mad_service  *service;
   mad_service_type_t     type;
   char                   name[SIZE_OFNAME];
   void                  *private_args;
   int                    nb_args;
} mad_service_args_t, *p_mad_service_args_t;

/* mad3 service specific args */
typedef struct s_mad_mad3_args {
   mad_way_t            way;
   p_mad_channel_t      channel ;
   ntbx_process_lrank_t remote_rank;
   p_mad_connection_t   connection ;
   void                *user_buffer;
   size_t               user_buffer_length;
   mad_send_mode_t      send_mode ;
   mad_receive_mode_t   receive_mode;
} mad_mad3_args_t, *p_mad_mad3_args_t ;

typedef void              *mad_return_t;
typedef mad_return_t     (*p_mad_func_t)( p_mad_service_args_t );
typedef p_mad_func_t      *mad_func_vector_t;
typedef p_mad_func_t       mad_func_array_t[NB_FUNC_INTERFACE];
typedef mad_func_vector_t  mad_func_vectors_t[SERVICE_NUMBER];

#endif /* MAD_SERVICE_ARGS_H */
