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
 * Mad_constructor.h
 * ==================
 */

#ifndef MAD_CONSTRUCTOR_H
#define MAD_CONSTRUCTOR_H


p_mad_dir_node_t
mad_dir_node_cons(void);

p_mad_dir_adapter_t
mad_dir_adapter_cons(void);

p_mad_dir_driver_process_specific_t
mad_dir_driver_process_specific_cons(void);

p_mad_dir_driver_t
mad_dir_driver_cons(void);

p_mad_dir_connection_data_t
mad_dir_connection_data_cons(void);

p_mad_dir_channel_t
mad_dir_channel_cons(void);

p_mad_dir_connection_t
mad_dir_connection_cons(void);

p_mad_directory_t
mad_directory_cons(void);

#ifdef MAD3_PMI
p_mad_pmi_t
mad_pmi_cons(void);
#endif /* MAD3_PMI */

p_mad_session_t
mad_session_cons(void);

p_mad_settings_t
mad_settings_cons(void);

p_mad_dynamic_t
mad_dynamic_cons(void);

/*
p_mad_configuration_t
mad_configuration_cons(void);
*/

p_mad_driver_interface_t
mad_driver_interface_cons(void);

p_mad_driver_t
mad_driver_cons(void);

p_mad_adapter_t
mad_adapter_cons(void);

p_mad_channel_t
mad_channel_cons(void);

p_mad_connection_t
mad_connection_cons(void);

p_mad_link_t
mad_link_cons(void);

p_mad_madeleine_t
mad_madeleine_cons(void);

#endif // MAD_CONSTRUCTOR_H
