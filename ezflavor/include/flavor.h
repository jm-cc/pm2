
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
   pm2_flavor... */
GList *flavor_list(void);

void flavor_create(void);

void flavor_load(void);

void flavor_check(void);

void flavor_save_as(void);

void flavor_delete(void);

void flavor_mark_modified(void);

void flavor_check_quit(void);


char *flavor_name(void);

// Format for module is "--modules=name"
gint flavor_uses_module(const char *module);

// Format for option is "--module=option"
char *flavor_uses_option(const char *option);

void flavor_reset_contents(void);

void flavor_add_module(const char *name);

void flavor_add_option(const char *name);

gint flavor_get_option_value(const char *option, char *value);

#endif
