
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
 * Mad_adapter_description.h
 * ==================
 */

#ifndef MAD_ADAPTER_DESCRIPTION_H
#define MAD_ADAPTER_DESCRIPTION_H

typedef struct s_mad_adapter_description
{
  mad_driver_id_t   driver_id;
  char             *adapter_selector;
} mad_adapter_description_t;

typedef struct s_mad_adapter_set
{
  int                           size;
  p_mad_adapter_description_t   description;
} mad_adapter_set_t;

#endif /* MAD_ADAPTER_DESCRIPTION_H */
