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
 * Mad_forward.h
 * =============
 */


#ifndef MAD_FORWARD_H
#define MAD_FORWARD_H

#define MAD_FORWARD_MAX_MTU 0xFFFFFFFFUL

#ifdef MARCEL
typedef struct s_mad_fmessage_queue_entry
{
  ntbx_process_grank_t source;
} mad_fmessage_queue_entry_t;

typedef enum e_mad_fblock_type
{
  mad_fblock_type_unknown = 0,
  mad_fblock_type_empty,
  mad_fblock_type_dynamic,
  mad_fblock_type_static_src,
  mad_fblock_type_static_dst,
} mad_fblock_type_t;

typedef enum e_mad_fblock_header_field
{
  mad_fblock_flength_0      = 0,
  mad_fblock_flength_1      = 1,
  mad_fblock_flength_2      = 2,
  mad_fblock_flength_3      = 3,
  mad_fblock_fsource_0      = 4,
  mad_fblock_fsource_1      = 5,
  mad_fblock_fsource_2      = 6,
  mad_fblock_fsource_3      = 7,
  mad_fblock_fdestination_0 = 8,
  mad_fblock_fdestination_1 = 9,
  mad_fblock_fdestination_2 = 10,
  mad_fblock_fdestination_3 = 11,
  mad_fblock_fis_a_group    = 12,
  mad_fblock_fis_a_new_msg  = 13,
  mad_fblock_fis_an_eof_msg = 14,
  mad_fblock_fis_direct     = 15,
  mad_fblock_fclosing       = 15,
#ifdef MAD_FORWARD_FLOW_CONTROL
  mad_fblock_fis_an_ack     = 14,
#endif /*  MAD_FORWARD_FLOW_CONTROL */
  mad_fblock_fsize          = 16,
} mad_fblock_header_field_t;

typedef enum e_mad_fblock_header_field_mask
{
  mad_fblock_flength_0_mask      = 0xFF,
  mad_fblock_flength_1_mask      = 0xFF,
  mad_fblock_flength_2_mask      = 0xFF,
  mad_fblock_flength_3_mask      = 0xFF,
  mad_fblock_fsource_0_mask      = 0xFF,
  mad_fblock_fsource_1_mask      = 0xFF,
  mad_fblock_fsource_2_mask      = 0xFF,
  mad_fblock_fsource_3_mask      = 0xFF,
  mad_fblock_fdestination_0_mask = 0xFF,
  mad_fblock_fdestination_1_mask = 0xFF,
  mad_fblock_fdestination_2_mask = 0xFF,
  mad_fblock_fdestination_3_mask = 0xFF,
  mad_fblock_fis_a_group_mask    = 0x01,
  mad_fblock_fis_a_new_msg_mask  = 0x01,
  mad_fblock_fis_an_eof_msg_mask = 0x01,
  mad_fblock_fis_direct_mask     = 0x01,
  mad_fblock_fclosing_mask       = 0x02,
#ifdef MAD_FORWARD_FLOW_CONTROL
  mad_fblock_fis_an_ack_mask     = 0x04,
#endif /*  MAD_FORWARD_FLOW_CONTROL */
} mad_fblock_header_field_mask_t;

typedef enum e_mad_fblock_header_field_shift
{
  mad_fblock_flength_0_shift      =  0,
  mad_fblock_flength_1_shift      =  8,
  mad_fblock_flength_2_shift      = 16,
  mad_fblock_flength_3_shift      = 24,
  mad_fblock_fsource_0_shift      =  0,
  mad_fblock_fsource_1_shift      =  8,
  mad_fblock_fsource_2_shift      = 16,
  mad_fblock_fsource_3_shift      = 24,
  mad_fblock_fdestination_0_shift =  0,
  mad_fblock_fdestination_1_shift =  8,
  mad_fblock_fdestination_2_shift = 16,
  mad_fblock_fdestination_3_shift = 24,
  mad_fblock_fis_a_group_shift    =  0,
  mad_fblock_fis_a_new_msg_shift  =  0,
  mad_fblock_fis_an_eof_msg_shift =  0,
  mad_fblock_fis_direct_shift     =  0,
  mad_fblock_fclosing_shift       = -1,
#ifdef MAD_FORWARD_FLOW_CONTROL
  mad_fblock_fis_an_ack_shift     = -2,
#endif /*  MAD_FORWARD_FLOW_CONTROL */
} mad_fblock_header_field_shift_t;


/*
 * mad_fblock_header fields :
 *
 *   LSB MSB
 * 4B 0 - 3 : length - buffer length or nb of buffer in group
 *            if b31 = 0 : buffer else buffer group
 * 4B 4 - 7 : destination as a vchannel local rank
 *            if b31 = 0 : regular block else new message block
 *
 */
typedef struct s_mad_fblock_header
{
  unsigned char            data[mad_fblock_fsize];
  unsigned int             length;
  unsigned int             source;
  unsigned int             destination;
  tbx_bool_t               is_a_group;
  tbx_bool_t               is_a_new_msg;
  tbx_bool_t               is_an_eof_msg;
  tbx_bool_t               closing;
#ifdef MAD_FORWARD_FLOW_CONTROL
  tbx_bool_t               is_an_ack;
#endif /*  MAD_FORWARD_FLOW_CONTROL */
  p_mad_buffer_t           block;
  mad_fblock_type_t        type;
  p_mad_connection_t       vout;
  p_mad_link_t             src_link;
  p_mad_driver_interface_t src_interface;
} mad_fblock_header_t;

typedef struct s_mad_forward_receive_block_arg
{
  p_mad_channel_t channel;
  p_mad_channel_t vchannel;
} mad_forward_receive_block_arg_t;

typedef struct s_mad_forward_reemit_block_arg
{
  p_mad_connection_t connection;
  p_mad_channel_t    vchannel;
} mad_forward_reemit_block_arg_t;

typedef struct s_mad_forward_block_queue
{
  marcel_sem_t  block_to_forward;
  p_tbx_slist_t queue;
} mad_forward_block_queue_t;

typedef struct s_mad_forward_poll_channel_arg
{
  p_mad_channel_t channel;
  p_mad_channel_t vchannel;
} mad_forward_poll_channel_arg_t;
#endif /*  MARCEL */

#endif /* MAD_FORWARD_H */

