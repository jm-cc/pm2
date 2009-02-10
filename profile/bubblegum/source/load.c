/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2008 "the PM2 team" (see AUTHORS file)
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

#include <gtk/gtk.h>
#include <stdlib.h>
#include <stdio.h>
#include "load.h"

gchar *pm2_root(void)
{
  gchar *ptr = getenv("PM2_ROOT");
  if (!ptr) {
    fprintf(stderr, "Error: undefined PM2_ROOT variable\n");
    exit(1);
  }
  return ptr;
}

GtkWidget *open_ico(const char *file) {
  gchar buf[1024];
  GtkWidget *widget;
  g_snprintf(buf, sizeof(buf), "%s/profile/bubblegum/%s", pm2_root(), file);
  widget = gtk_image_new_from_file(buf);
  return widget;
}

Texture *open_texture(const char *file) {
  gchar buf[1024];
  Texture *text;
  g_snprintf(buf, sizeof(buf), "%s/profile/bubblegum/%s", pm2_root(), file);
  text = LoadTextureFromFile(buf);
  return text;
}
