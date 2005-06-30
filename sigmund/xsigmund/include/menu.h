
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

#ifndef MENU_IS_DEF
#define MENU_IS_DEF

#include <gtk/gtk.h>

void menu_init(GtkWidget *vbox);

void menu_update_flavor(gint load_enabled, gint reload,
			gint create_enabled,
			gint save_enabled, gint save_as,
			gint delete_enabled,
			gint check_enabled);

void menu_update_module(gint select_enabled, gint deselect_enabled);

void menu_update_common_opt(gint displayed);

#endif
