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
 * leonie.h
 * --------
 */

#ifndef LEONIE_H
#define LEONIE_H

/*
 * Checks
 * ------
 */
#ifndef __GNUC__
#error The GNU C Compiler is required to build this tool
#endif // __GNUC__

//#define TBX_FAILURE_CLEANUP() leonie_failure_cleanup()

#include "tbx.h"
#include "ntbx.h"
#include "leoparse.h"

/* Leonie: data structures */
#include "leo_pointers.h"
#include "leo_types.h"
#include "leo_commands.h"

/* Leonie: internal interface */
#include "leo_interface.h"

void
leonie_failure_cleanup(p_leo_settings_t settings);

#endif /* LEONIE_H */
