
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
 * swann.h
 * --------
 */

#ifndef SWANN_H
#define SWANN_H

/*
 * Checks
 * ------
 */
#ifndef __GNUC__
#error The GNU C Compiler is required to build this tool
#endif __GNUC__

#define FAILURE_CLEANUP() swann_failure_cleanup()

#include "tbx.h"
#include "ntbx.h"
#include "leoparse.h"

#include "swann_pointers.h"
#include "swann_types.h"
#include "swann_interface.h"

#endif /* SWANN_H */
