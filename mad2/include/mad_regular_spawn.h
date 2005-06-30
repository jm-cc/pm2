
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
 * Mad_regular_spawn.h
 * ===================
 */

#ifndef MAD_REGULAR_SPAWN_H
#define MAD_REGULAR_SPAWN_H

/*
 * Fonctions exportees
 * -------------------
 */

void
mad_slave_spawn(p_mad_madeleine_t   madeleine,
		int                 argc,
		char              **argv);

p_mad_madeleine_t
mad_init(int                  *argc,
	 char                **argv,
	 char                 *configuration_file,
	 p_mad_adapter_set_t   adapter_set);

#endif /* MAD_REGULAR_SPAWN_H */
