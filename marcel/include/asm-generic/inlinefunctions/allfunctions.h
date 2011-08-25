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


#include "asm/inlinefunctions/linux_atomic.h"
#include "asm/inlinefunctions/linux_bitops.h"
#include "asm/inlinefunctions/linux_rwlock.h"
#include "asm/inlinefunctions/marcel_compareexchange.h"
#include "asm/inlinefunctions/marcel_testandset.h"
