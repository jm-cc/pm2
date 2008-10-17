/* -*- Mode: C; c-basic-offset:4 ; -*- */
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

#ifndef PIOM_POLL_H
#define PIOM_POLL_H

#include "pioman.h"

/* Polling constants. Defines the polling points */
#define PIOM_POLL_AT_TIMER_SIG  1
#define PIOM_POLL_AT_YIELD      2
#define PIOM_POLL_AT_LIB_ENTRY  4
#define PIOM_POLL_AT_IDLE       8
#define PIOM_POLL_WHEN_FORCED   16

/* Used by initializers */
void piom_poll_from_tasklet(unsigned long id);
void piom_poll_timer(unsigned long id);

/* Poll a specified request */
void piom_poll_req(piom_req_t req, unsigned usec);
/* Forced polling */
void piom_poll_force(piom_server_t server);
/* Idem, ensures the polling is done before it returns */
void piom_poll_force_sync(piom_server_t server);

/* Checks to see if some polling jobs should be done */
void piom_check_polling_for(piom_server_t server);

/* Are there requests to poll on this server ? */
int __piom_need_poll(piom_server_t server);

/* Try to group requests (polling version) */
int __piom_poll_group(piom_server_t server,
		      piom_req_t req);

#endif	/* PIOM_POLL_H */
