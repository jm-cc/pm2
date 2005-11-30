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
 * Mad_directory.h
 * ================
 */

#ifndef MAD_DIRECTORY_H
#define MAD_DIRECTORY_H

/*
 * Madeleine directory
 * --------------------
 */

typedef struct s_mad_dir_node
{
  char                       *name;
  unsigned long               ip; // network form !
  p_ntbx_process_container_t  pc;
} mad_dir_node_t;

typedef struct s_mad_dir_adapter
{
  char         *name;
  char         *selector;
  char         *parameter;
  unsigned int  mtu;
} mad_dir_adapter_t;

typedef struct s_mad_dir_driver_process_specific
{
  p_tbx_htable_t  adapter_htable;
  p_tbx_slist_t   adapter_slist;
  char           *parameter;
} mad_dir_driver_process_specific_t;

typedef struct s_mad_dir_driver
{
  mad_driver_id_t             id;
  char                       *network_name;
  char                       *device_name;
  p_ntbx_process_container_t  pc;
} mad_dir_driver_t;

typedef struct s_mad_dir_connection_data
{
  char                 *channel_name;
  ntbx_process_grank_t  destination_rank;
} mad_dir_connection_data_t;

typedef struct s_mad_dir_connection
{
  p_ntbx_process_container_t  pc;
  char *in_parameter;
  char *out_parameter;
  char *channel_parameter;
  char *adapter_name;
} mad_dir_connection_t;

typedef struct s_mad_dir_channel
{
  mad_channel_id_t            id;
  char                       *name;
  p_ntbx_process_container_t  pc;
  p_mad_dir_driver_t          driver;

  // Regular channel
  tbx_bool_t                  not_private;
  tbx_bool_t                  mergeable; 
  p_ntbx_topology_table_t     ttable;

  // Forwarding channel
  char                       *cloned_channel_name;

  // Virtual/Mux
  p_tbx_slist_t               dir_channel_slist;

  // Virtual
  p_tbx_slist_t               dir_fchannel_slist;

  // Mux
  p_tbx_slist_t               sub_channel_name_slist;
} mad_dir_channel_t;

typedef struct s_mad_directory
{
  p_tbx_darray_t process_darray;
  p_tbx_slist_t  process_slist;

  p_tbx_htable_t node_htable;
  p_tbx_slist_t  node_slist;

  p_tbx_htable_t driver_htable;
  p_tbx_slist_t  driver_slist;

  p_tbx_htable_t channel_htable;
  p_tbx_slist_t  channel_slist;
  p_tbx_slist_t  fchannel_slist;
  p_tbx_slist_t  vchannel_slist;
  p_tbx_slist_t  xchannel_slist;
} mad_directory_t;

#endif /* MAD_DIRECTORY_H */


/*
 * Local variables:
 *  c-basic-offset: 2
 *  c-hanging-braces-alist: '((defun-open before after) (class-open before after) (inline-open before after) (block-open before after) (brace-list-open) (brace-entry-open) (substatement-open before after) (block-close . c-snug-do-while) (extern-lang-open before after) (inexpr-class-open before after) (inexpr-class-close before))
 * End:
 */
