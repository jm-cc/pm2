
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
 * swann_types.h
 * -------------
 */

#ifndef SWANN_TYPES_H
#define SWANN_TYPES_H

typedef enum
{
  swann_success =  0,
  swann_failure = -1,
} swann_status_t, *p_swann_status_t;

typedef int swann_net_client_id_t, *p_swann_net_client_id_t;
typedef int swann_net_server_id_t, *p_swann_net_server_id_t;

#endif /* SWANN_TYPES_H */
