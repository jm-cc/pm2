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

#ifndef MODULE_IS_DEF
#define MODULE_IS_DEF

#include <glib.h>

void module_init(GtkWidget *leftbox, GtkWidget *rightbox);

void module_select(void);

void module_deselect(void);

void module_update_with_current_flavor(void);

void module_save_to_flavor(void);

extern char *known_static_modules[];

#endif
