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
 * Mad_mx.h
 * =========
 */
#ifndef MAD_MX_H
#define MAD_MX_H

/*
 * Structures
 * ----------
 */

/*
 * Functions
 * ---------
 */

/** Initialisations **/
char * mad_mx_register(p_mad_driver_interface_t);

void mad_mx_driver_init (p_mad_driver_t, int *, char ***);
void mad_mx_adapter_init(p_mad_adapter_t);
void mad_mx_channel_init(p_mad_channel_t);
void mad_mx_connection_init(p_mad_connection_t, p_mad_connection_t);
void mad_mx_link_init (p_mad_link_t);
void mad_mx_track_init(p_mad_adapter_t, uint32_t);



/** Connexions **/
void mad_mx_accept(p_mad_connection_t, p_mad_adapter_info_t);
void mad_mx_connect(p_mad_connection_t, p_mad_adapter_info_t);





/** Désallocations **/
void mad_mx_connection_exit(p_mad_connection_t, p_mad_connection_t);
void mad_mx_channel_exit(p_mad_channel_t);
void mad_mx_adapter_exit(p_mad_adapter_t);
void mad_mx_driver_exit(p_mad_driver_t);
void mad_mx_track_exit(p_mad_track_t);


/** Fonctions de transfert **/
void mad_mx_new_message(p_mad_connection_t);
p_mad_connection_t mad_mx_receive_message(p_mad_channel_t);



/** Gestion des pré-postés **/
void mad_mx_init_pre_posted(p_mad_adapter_t, p_mad_track_t);
void mad_mx_replace_pre_posted(p_mad_adapter_t, p_mad_track_t, int);
void mad_mx_remove_all_pre_posted(p_mad_adapter_t);


/** Informations spécifiques **/
tbx_bool_t mad_mx_need_rdv(p_mad_iovec_t);
tbx_bool_t mad_mx_buffer_need_rdv(size_t);
uint32_t   mad_mx_gather_scatter_length_max(void);
uint32_t   mad_mx_cpy_limit_size(void);

// **** Primitives Bas Niveau **** //
void mad_mx_isend(p_mad_track_t, p_mad_iovec_t);
void mad_mx_irecv(p_mad_track_t, p_mad_iovec_t);
tbx_bool_t    mad_mx_s_test(p_mad_track_set_t);
p_mad_iovec_t mad_mx_r_test(p_mad_track_t);

#endif /* MAD_MX_H */
