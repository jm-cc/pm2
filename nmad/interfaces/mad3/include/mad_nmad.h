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
 * Mad_nmad.h
 * =========
 */
#ifndef MAD_NMAD_H
#define MAD_NMAD_H

#include <nm_session_interface.h>

#include <nm_log.h>
#define LOG_IN     NM_LOG_IN
#define LOG_OUT    NM_LOG_OUT
#define DISP       NM_DISPF
#define TRACE      NM_TRACEF
#define TRACE_VAL  NM_TRACE_VAL
#define TRACE_STR  NM_TRACE_STR
#define TRACE_CHAR NM_TRACE_CHAR

/*
 * Structures
 * ----------
 */

/*
 * Functions 
 * ---------
 */

char * mad_nmad_register(p_mad_driver_interface_t);

nm_session_t mad_nmad_get_session(void);

#endif /* MAD_NMAD_H */
