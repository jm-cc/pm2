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
  int                         id;
  char                       *name;
  p_ntbx_process_container_t  pc;
} mad_dir_node_t;

typedef struct s_mad_dir_adapter
{
  int           id;
  char         *name;
  char         *selector;
  char         *parameter;
  unsigned int  mtu;
} mad_dir_adapter_t;

typedef struct s_mad_dir_driver_process_specific
{
  p_tbx_htable_t adapter_htable;
  p_tbx_slist_t  adapter_slist;
} mad_dir_driver_process_specific_t;

typedef struct s_mad_dir_driver
{
  int                         id;
  char                       *name;
  p_ntbx_process_container_t  pc;
} mad_dir_driver_t;

typedef struct s_mad_dir_channel_process_specific
{
  char *adapter_name;
} mad_dir_channel_process_specific_t;

typedef struct s_mad_dir_channel
{
  int                         id;
  char                       *name;
  p_ntbx_process_container_t  pc;
  p_mad_dir_driver_t          driver;
  tbx_bool_t                  public;
  p_ntbx_topology_table_t     ttable;
} mad_dir_channel_t;

typedef struct s_mad_dir_vchannel_process_routing_table
{
  char                 *channel_name;
  ntbx_process_grank_t  destination_rank;
} mad_dir_vchannel_process_routing_table_t;

typedef struct s_mad_dir_vchannel_process_specific
{
  p_ntbx_process_container_t  pc;
} mad_dir_vchannel_process_specific_t;

typedef struct s_mad_dir_fchannel
{
  int   id;
  char *name;
  char *channel_name;
} mad_dir_fchannel_t;


typedef struct s_mad_dir_vchannel
{
  int                         id;
  char                       *name;
  p_tbx_slist_t               dir_channel_slist;
  p_tbx_slist_t               dir_fchannel_slist;
  p_ntbx_process_container_t  pc;
} mad_dir_vchannel_t;

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

  p_tbx_htable_t fchannel_htable;  
  p_tbx_slist_t  fchannel_slist;

  p_tbx_htable_t vchannel_htable;  
  p_tbx_slist_t  vchannel_slist;
} mad_directory_t;

#endif /* MAD_DIRECTORY_H */
