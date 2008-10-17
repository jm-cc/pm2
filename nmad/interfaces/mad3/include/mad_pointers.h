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
 * Mad_pointers.h
 * ==============
 */

#ifndef MAD_POINTERS_H
#define MAD_POINTERS_H

typedef struct s_mad_connection		*p_mad_connection_t;
typedef struct s_mad_driver_interface	*p_mad_driver_interface_t;
typedef struct s_mad_link		*p_mad_link_t;
typedef struct s_mad_driver		*p_mad_driver_t;
typedef struct s_mad_adapter 		*p_mad_adapter_t;
typedef struct s_mad_adapter_info	*p_mad_adapter_info_t;
typedef struct s_mad_buffer		*p_mad_buffer_t;
typedef struct s_mad_buffer_pair	*p_mad_buffer_pair_t;
typedef struct s_mad_buffer_group	*p_mad_buffer_group_t;
typedef struct s_mad_channel		*p_mad_channel_t;
typedef struct s_mad_channel_process	*p_mad_channel_process_t;
typedef struct s_mad_madeleine          *p_mad_madeleine_t;
typedef struct s_mad_settings           *p_mad_settings_t;
typedef struct s_mad_dynamic            *p_mad_dynamic_t;
typedef struct s_mad_session		*p_mad_session_t;
typedef struct s_mad_adapter_description	*p_mad_adapter_description_t;
typedef struct s_mad_adapter_set	*p_mad_adapter_set_t;

typedef struct s_mad_dir_node		*p_mad_dir_node_t;
typedef struct s_mad_dir_adapter	*p_mad_dir_adapter_t;
typedef struct s_mad_dir_driver_process_specific
*p_mad_dir_driver_process_specific_t;
typedef struct s_mad_dir_driver		*p_mad_dir_driver_t;
typedef struct s_mad_dir_connection_data	*p_mad_dir_connection_data_t;
typedef struct s_mad_dir_channel	*p_mad_dir_channel_t;
typedef struct s_mad_dir_connection     *p_mad_dir_connection_t;
typedef struct s_mad_directory		*p_mad_directory_t;

typedef struct s_mad_fmessage_queue_entry
*p_mad_fmessage_queue_entry_t;
typedef struct s_mad_fblock_header	*p_mad_fblock_header_t;
typedef struct s_mad_forward_receive_block_arg
*p_mad_forward_receive_block_arg_t;
typedef struct s_mad_forward_reemit_block_arg
*p_mad_forward_reemit_block_arg_t;
typedef struct s_mad_forward_block_queue
*p_mad_forward_block_queue_t;
typedef struct s_mad_forward_poll_channel_arg
*p_mad_forward_poll_channel_arg_t;

typedef struct s_mad_xmessage_queue_entry
*p_mad_xmessage_queue_entry_t;
typedef struct s_mad_xblock_header	*p_mad_xblock_header_t;
typedef struct s_mad_mux_receive_block_arg
*p_mad_mux_receive_block_arg_t;
typedef struct s_mad_mux_reemit_block_arg
*p_mad_mux_reemit_block_arg_t;
typedef struct s_mad_mux_block_queue	*p_mad_mux_block_queue_t;
typedef struct s_mad_mux_darray_lane	*p_mad_mux_darray_lane_t;

#endif /* MAD_POINTERS_H */

