
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
 * Mad_application_spawn.h
 * =======================
 */

#ifndef MAD_APPLICATION_SPAWN_H
#define MAD_APPLICATION_SPAWN_H

/*
 * Controles du mode de compilation
 * --------------------------------
 */

#ifdef EXTERNAL_SPAWN
#error EXTERNAL_SPAWN cannot be specified with APPLICATION_SPAWN
#endif /* EXTERNAL_SPAWN */

/*
 * Fonctions exportees
 * -------------------
 */

char *
mad_generate_url(p_mad_madeleine_t madeleine);

void
mad_parse_url(p_mad_madeleine_t  madeleine);


#endif /* MAD_APPLICATION_SPAWN_H */
