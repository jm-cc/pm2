
/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 "the PM2 team" (pm2-dev@listes.ens-lyon.fr)
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
 * Mad_configuration.h
 * ===================
 */

#ifndef MAD_CONFIGURATION_H
#define MAD_CONFIGURATION_H

/*
 * Madeleine configuration
 * -----------------------
 */
typedef struct s_mad_configuration
{
  mad_configuration_size_t   size;
  ntbx_host_id_t             local_host_id;
  char                     **host_name; /* configuration host name list */
  char                     **program_name; /* program name list */
} mad_configuration_t;

#endif /* MAD_CONFIGURATION_H */
