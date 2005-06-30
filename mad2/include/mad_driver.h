
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
 * Mad_driver.h
 * ============
 */

#ifndef MAD_DRIVER_H
#define MAD_DRIVER_H

typedef enum
{
  mad_unidirectional_connection,
  mad_bidirectional_connection

} mad_connection_type_t, *p_mad_connection_type_t;

typedef struct s_mad_driver
{
  /* Common use fields */
  TBX_SHARED;
  char                       *name;
  p_mad_madeleine_t           madeleine;
  mad_driver_id_t             id;
  mad_driver_interface_t      interface;
  
  /* Settings */
  mad_buffer_alignment_t      buffer_alignment;
  mad_connection_type_t       connection_type;

  /* Internal use fields */
  tbx_list_t                  adapter_list;

  /* Driver specific */
  p_mad_driver_specific_t     specific;
} mad_driver_t;

#endif /* MAD_DRIVER_H */
