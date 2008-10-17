
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
 * Mad_exit.h
 * ==========
 */

#ifndef MAD_EXIT_H
#define MAD_EXIT_H

void
mad_leonie_sync(p_mad_madeleine_t madeleine);

void
mad_leonie_link_exit(p_mad_madeleine_t madeleine);

void
mad_object_exit(p_mad_madeleine_t madeleine);


/*
 * Directory clean-up
 * ------------------
 */

void
mad_dir_channels_exit(p_mad_madeleine_t madeleine);

void
mad_dir_driver_exit(p_mad_madeleine_t madeleine);

void
mad_directory_exit(p_mad_madeleine_t madeleine);

void
mad_exit(p_mad_madeleine_t madeleine);

#endif /* MAD_EXIT_H */
