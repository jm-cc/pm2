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
 *
 * 
 * Memory barrier public functions
 */


#include "marcel.h"


/** Public functions **/
void marcel_mb()
{
	ma_mb();
}

void marcel_rmb()
{
	ma_rmb();
}

void marcel_wmb()
{
	ma_wmb();
}

void marcel_barrier()
{
	ma_barrier();
}
