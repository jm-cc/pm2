
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

#ifndef FLAVOR_IS_DEF
#define FLAVOR_IS_DEF

extern gint flavor_modified;

void flavor_init(GtkWidget *vbox);

/* Returns the list of flavor names (GList of strings) using
   pm2-flavor... */
GList *flavor_list(void);

void flavor_create(void);

void flavor_load(void);

void flavor_check(void);

void flavor_save_as(void);

void flavor_delete(void);

void flavor_mark_modified(void);

void flavor_check_quit(void);


char *flavor_name(void);

char *flavor_builddir(void);

char *flavor_extension(void);

// Format for module is "--modules=name"
gint flavor_uses_module(char *module);

// Format for option is "--module=option"
gint flavor_uses_option(char *option);

void flavor_reset_contents(void);

void flavor_set_builddir(char *name);

void flavor_set_extension(char *name);

void flavor_add_module(char *name);

void flavor_add_option(char *name);

#endif
