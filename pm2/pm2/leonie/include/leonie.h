
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
 * leonie.h
 * --------
 */

#ifndef __LEONIE_H
#define __LEONIE_H

/*
 * Checks
 * ------
 */
#ifndef __GNUC__
#error The GNU C Compiler is required to build this tool
#endif __GNUC__

#include "tbx_debug.h"
#include "ntbx.h"
#include "tbx.h"
#include "leoparse.h"

/* Leonie: data structures */
#include "leo_pointers.h"
#include "leo_types.h"
#include "leo_main.h"

/* Leonie: internal interface */
#include "leo_net.h"
#include "leo_swann.h"
#include "leo_interface.h"

#endif /* __LEONIE_H */
