/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 the PM2 team (see AUTHORS file)
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


#ifndef __MARCEL_ERRNO_H__
#define __MARCEL_ERRNO_H__


#include "sys/marcel_flags.h"
#include "marcel_alias.h"


/** Public macros **/
#define marcel_errno (*marcel___errno_location())
#define pmarcel_errno (*pmarcel___errno_location())
#define marcel_h_errno (*marcel___h_errno_location())
#define pmarcel_h_errno (*pmarcel___h_errno_location())


/** Public functions **/
int *pmarcel___errno_location(void);
int *pmarcel___h_errno_location(void);
DEC_MARCEL_POSIX(int *, __errno_location, (void));
DEC_MARCEL_POSIX(int *, __h_errno_location, (void));


#endif /** __MARCEL_ERRNO_H__ **/
