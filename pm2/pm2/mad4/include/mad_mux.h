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
 * Mad_mux.h
 * =============
 */


#ifndef MAD_MUX_H
#define MAD_MUX_H

#define MAD_MUX_MAX_MTU 0xFFFFFFFFUL
#define MAD_MUX_MAX_SUB 256
#define MAD_MUX_MAX_MUX 256

typedef struct s_mad_xmessage_queue_entry
{
  ntbx_process_grank_t source;
} mad_xmessage_queue_entry_t;

typedef enum e_mad_xblock_buffer_type
{
  mad_xblock_buffer_type_unknown = 0,
  mad_xblock_buffer_type_none,
  mad_xblock_buffer_type_dynamic,
  mad_xblock_buffer_type_static_src,
  mad_xblock_buffer_type_static_dst,
  mad_xblock_buffer_type_dest,
} mad_xblock_buffer_type_t;

typedef enum e_mad_xblock_header_field
{
  mad_xblock_flength_0      = 0,
  mad_xblock_flength_1      = 1,
  mad_xblock_flength_2      = 2,
  mad_xblock_flength_3      = 3,
  mad_xblock_fsource_0      = 4,
  mad_xblock_fsource_1      = 5,
  mad_xblock_fsource_2      = 6,
  mad_xblock_fsource_3      = 7,
  mad_xblock_fdestination_0 = 8,
  mad_xblock_fdestination_1 = 9,
  mad_xblock_fdestination_2 = 10,
  mad_xblock_fdestination_3 = 11,

  mad_xblock_fmux           = 12,

  mad_xblock_fsub           = 13,

  mad_xblock_fis_a_group    = 14,
  mad_xblock_fis_a_new_msg  = 14,
#ifdef MAD_MUX_FLOW_CONTROL
  mad_xblock_fis_an_ack     = 14,
#endif // MAD_MUX_FLOW_CONTROL

  mad_xblock_fclosing       = 15,
  mad_xblock_fsize          = 16,
} mad_xblock_header_field_t;

typedef enum e_mad_xblock_header_field_mask
{
  mad_xblock_flength_0_mask      = 0xFF,
  mad_xblock_flength_1_mask      = 0xFF,
  mad_xblock_flength_2_mask      = 0xFF,
  mad_xblock_flength_3_mask      = 0xFF,
  mad_xblock_fsource_0_mask      = 0xFF,
  mad_xblock_fsource_1_mask      = 0xFF,
  mad_xblock_fsource_2_mask      = 0xFF,
  mad_xblock_fsource_3_mask      = 0xFF,
  mad_xblock_fdestination_0_mask = 0xFF,
  mad_xblock_fdestination_1_mask = 0xFF,
  mad_xblock_fdestination_2_mask = 0xFF,
  mad_xblock_fdestination_3_mask = 0xFF,

  mad_xblock_fmux_mask           = 0xFF,

  mad_xblock_fsub_mask           = 0xFF,

  mad_xblock_fis_a_group_mask    = 0x02,
  mad_xblock_fis_a_new_msg_mask  = 0x04,
#ifdef MAD_MUX_FLOW_CONTROL
  mad_xblock_fis_an_ack_mask     = 0x08,
#endif // MAD_MUX_FLOW_CONTROL

  mad_xblock_fclosing_mask       = 0x01,
} mad_xblock_header_field_mask_t;

typedef enum e_mad_xblock_header_field_shift
{
  mad_xblock_flength_0_shift      =  0,
  mad_xblock_flength_1_shift      =  8,
  mad_xblock_flength_2_shift      = 16,
  mad_xblock_flength_3_shift      = 24,
  mad_xblock_fsource_0_shift      =  0,
  mad_xblock_fsource_1_shift      =  8,
  mad_xblock_fsource_2_shift      = 16,
  mad_xblock_fsource_3_shift      = 24,
  mad_xblock_fdestination_0_shift =  0,
  mad_xblock_fdestination_1_shift =  8,
  mad_xblock_fdestination_2_shift = 16,
  mad_xblock_fdestination_3_shift = 24,

  mad_xblock_fmux_shift           =  0,

  mad_xblock_fsub_shift           =  0,

  mad_xblock_fis_a_group_shift    = -1,
  mad_xblock_fis_a_new_msg_shift  = -2,
#ifdef MAD_MUX_FLOW_CONTROL
  mad_xblock_fis_an_ack_shift     = -3,
#endif // MAD_MUX_FLOW_CONTROL

  mad_xblock_fclosing_shift       =  0,
} mad_xblock_header_field_shift_t;


/*
 * mad_xblock_header fields :
 *
 *   LSB MSB
 * 4B 0 - 3 : length - buffer length or nb of buffer in group
 *            if b31 = 0 : buffer else buffer group
 * 4B 4 - 7 : destination as a vchannel local rank
 *            if b31 = 0 : regular block else new message block
 *
 */
typedef struct s_mad_xblock_header
{
  unsigned char            data[mad_xblock_fsize];
  unsigned int             length;
  unsigned int             source;
  unsigned int             destination;
  tbx_bool_t               is_a_group;
  tbx_bool_t               is_a_new_msg;
  tbx_bool_t               closing;
  tbx_bool_t               data_available;
  unsigned int             mux;
  unsigned int             sub;
#ifdef MAD_MUX_FLOW_CONTROL
  tbx_bool_t               is_an_ack;
#endif // MAD_MUX_FLOW_CONTROL
  p_mad_buffer_t           block;
  mad_xblock_buffer_type_t buffer_type;
  p_mad_connection_t       xout;
  p_mad_link_t             src_link;
  p_mad_driver_interface_t src_interface;
} mad_xblock_header_t;

typedef struct s_mad_mux_receive_block_arg
{
  p_mad_channel_t channel;
  p_mad_channel_t xchannel;
} mad_mux_receive_block_arg_t;

typedef struct s_mad_mux_reemit_block_arg
{
  p_mad_connection_t connection;
  p_mad_channel_t    xchannel;
} mad_mux_reemit_block_arg_t;

typedef struct s_mad_mux_block_queue
{
  marcel_sem_t  block_to_forward;
  p_tbx_slist_t queue;
  p_tbx_slist_t buffer_slist;
} mad_mux_block_queue_t;

typedef struct s_mad_mux_darray_lane
{
  TBX_SHARED;
  p_tbx_darray_t block_queues;
  p_tbx_slist_t  message_queue;
  marcel_sem_t   something_to_forward;
} mad_mux_darray_lane_t;
#endif /* MAD_MUX_H */

