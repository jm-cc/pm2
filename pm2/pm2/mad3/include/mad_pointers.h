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

/* ... Point-to-point connection ..................................... */
typedef struct s_mad_connection
*p_mad_connection_t;

/* ... Protocol generic interface .................................... */
typedef struct s_mad_driver_interface
*p_mad_driver_interface_t;

/* ... Transfer method ............................................... */
typedef struct s_mad_link
*p_mad_link_t;

/* ... Protocol module ............................................... */
typedef struct s_mad_driver
*p_mad_driver_t;

/* ... Network interface card ........................................ */
typedef struct s_mad_adapter
*p_mad_adapter_t;

/* ... Information required to connect to a remote adapter ........... */
typedef struct s_mad_adapter_info
*p_mad_adapter_info_t;

/* ... Virtual buffers ............................................... */
typedef struct s_mad_buffer
*p_mad_buffer_t;

/* ... Virtual buffer pair ........................................... */
typedef struct s_mad_buffer_pair
*p_mad_buffer_pair_t;

/* ... Group of virtual buffers ...................................... */
typedef struct s_mad_buffer_group
*p_mad_buffer_group_t;

/* ... Communication channel ......................................... */
typedef struct s_mad_channel
*p_mad_channel_t;

/* ... Channel member ................................................ */
typedef struct s_mad_channel_process
*p_mad_channel_process_t;

/* ... Main madeleine object ......................................... */
typedef struct s_mad_madeleine
*p_mad_madeleine_t;

/* ... Madeleine module settings ..................................... */
typedef struct s_mad_settings
*p_mad_settings_t;

/* ... Madeleine module session ...................................... */
typedef struct s_mad_session
*p_mad_session_t;

/* ... Adapter identification ........................................ */
typedef struct s_mad_adapter_description
*p_mad_adapter_description_t;

/* ... Set of adapter identifications ................................ */
typedef struct s_mad_adapter_set
*p_mad_adapter_set_t;

/* ... Directory: node data .......................................... */
typedef struct s_mad_dir_node
*p_mad_dir_node_t;

/* ... Directory: adapter data ....................................... */
typedef struct s_mad_dir_adapter
*p_mad_dir_adapter_t;

/* ... Directory: per process specific data .......................... */
typedef struct s_mad_dir_driver_process_specific
*p_mad_dir_driver_process_specific_t;

/* ... Directory: driver data ........................................ */
typedef struct s_mad_dir_driver
*p_mad_dir_driver_t;

/* ... Directory: per process specific data .......................... */
typedef struct s_mad_dir_channel_process_specific
*p_mad_dir_channel_process_specific_t;

/* ... Directory: channel data ....................................... */
typedef struct s_mad_dir_channel
*p_mad_dir_channel_t;

/* ... Directory: virtual channel routing table element .............. */
typedef struct s_mad_dir_vchannel_process_routing_table
*p_mad_dir_vchannel_process_routing_table_t;

/* ... Directory: per process virtual channel specific data .......... */
typedef struct s_mad_dir_vchannel_process_specific
*p_mad_dir_vchannel_process_specific_t;

/* ... Directory: forwarding channel data ............................ */
typedef struct s_mad_dir_fchannel
*p_mad_dir_fchannel_t;

/* ... Directory: virtual channel data ............................... */
typedef struct s_mad_dir_vchannel
*p_mad_dir_vchannel_t;

/* ... Directory ..................................................... */
typedef struct s_mad_directory
*p_mad_directory_t;

/* ... Forwarding message queue entry ................................ */
typedef struct s_mad_fmessage_queue_entry
*p_mad_fmessage_queue_entry_t;

/* ... Forwarding block type ......................................... */
typedef enum e_mad_fblock_type
*p_mad_fblock_type_t;

/* ... Forwarding block header fields ................................ */
typedef enum e_mad_fblock_header_field
*p_mad_fblock_header_field_t;

/* ... Forwarding block header field masks ........................... */
typedef enum e_mad_fblock_header_field_mask
*p_mad_fblock_header_field_mask_t;

/* ... Forwarding block header field shifts .......................... */
typedef enum e_mad_fblock_header_field_shift
*p_mad_fblock_header_field_shift_t;

/* ... Forwarding block header ....................................... */
typedef struct s_mad_fblock_header
 *p_mad_fblock_header_t;

/* ... Forwarding receive function args .............................. */
typedef struct s_mad_forward_receive_block_arg
*p_mad_forward_receive_block_arg_t;

/* ... Forwarding send function args ................................. */
typedef struct s_mad_forward_reemit_block_arg
*p_mad_forward_reemit_block_arg_t;

/* ... Forwarding block queue ........................................ */
typedef struct s_mad_forward_block_queue
*p_mad_forward_block_queue_t;

/* ... Forwarding polling function args .............................. */
typedef struct s_mad_forward_poll_channel_arg
*p_mad_forward_poll_channel_arg_t;

#endif /* MAD_POINTERS_H */

