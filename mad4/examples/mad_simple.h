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

#include "pm2_common.h"

#define CHANNEL_NAME    "test"                  /* name of the channel as defined in  */
                                                /* the application configuration file */
#define PINGPONG        5                       /* number of pingpongs */
/*
 *
 */
void do_main_code(p_mad_madeleine_t madeleine);

/*
 *
 */
int senddata(p_mad_channel_t channel, ntbx_process_lrank_t dest, int data);

/*
 *
 */
void receivedata(p_mad_channel_t channel, int* data);
