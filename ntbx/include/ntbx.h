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
 * Ntbx.h
 * ======
 */

#ifndef NTBX_H
#define NTBX_H

/*
 * Checks
 * ------
 */
#ifndef __GNUC__
#error The GNU C Compiler is required to build the library
#endif /*  __GNUC__ */

/*
 * Headers
 * -------
 */
#ifdef MARCEL
#include "marcel.h"
#endif /* MARCEL */
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>

#include "tbx.h"
#include "ntbx_types.h"
#include "ntbx_net_commands.h"

#ifdef NTBX_TCP
#include "ntbx_tcp.h"
#endif /* NTBX_TCP */

#include "ntbx_interface.h"

#endif /* NTBX_H */

