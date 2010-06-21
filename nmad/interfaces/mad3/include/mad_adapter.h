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
 * Mad_adapter.h
 * ==============
 */

#ifndef MAD_ADAPTER_H
#define MAD_ADAPTER_H

typedef struct s_mad_adapter_info
{
  p_mad_dir_node_t     dir_node;
  p_mad_dir_adapter_t  dir_adapter;
  char                *channel_parameter;
  char                *connection_parameter;
} mad_adapter_info_t;

typedef struct s_mad_adapter
{
  // Common use fields
  p_mad_driver_t           driver;
  mad_adapter_id_t         id;
  char                    *selector;
  char                    *parameter;
  p_tbx_htable_t           channel_htable;
  p_mad_dir_adapter_t      dir_adapter;
  p_mad_driver_specific_t  specific;
} mad_adapter_t;

#endif /* MAD_ADAPTER_H */
