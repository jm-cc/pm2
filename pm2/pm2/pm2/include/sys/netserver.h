
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

#ifndef NETSERVER_EST_DEF
#define NETSERVER_EST_DEF

#include "marcel.h"
#include "madeleine.h"

enum {
   NETSERVER_END,
   NETSERVER_RAW_RPC  /* Must be the last one */
};

#ifdef MAD2
void netserver_start(p_mad_channel_t channel);
#else
void netserver_start(void);
#endif

void netserver_wait_end(void);

void netserver_stop(void);

#endif
