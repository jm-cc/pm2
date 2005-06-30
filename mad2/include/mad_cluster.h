
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

#ifndef MAD_CLUSTER_H
#define MAD_CLUSTER_H

typedef enum
{
  mad_channel_default = 0,
} mad_channel_description_id_t, *p_mad_channel_description_id_t;

typedef struct s_mad_channel_description
{
  mad_channel_description_id_t    id;
  /* mad_channel_description_alias_t alias; */
  mad_driver_id_t                 driver_id;
  mad_adapter_id_t                adapter_id;
} mad_channel_description_t;

typedef struct s_mad_cluster
{
  TBX_SHARED;
  mad_cluster_id_t            id;
  mad_configuration_t         configuration;
  p_mad_channel_description_t channel_description_array;
} mad_cluster_t;

#endif /* MAD_CLUSTER_H */
