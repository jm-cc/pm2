
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

#include <stdarg.h>
#include <stdio.h>
#include <gtk/gtk.h>
#include <string.h>

#include "statusbar.h"


static char supertracename[1024];
static char msg[1024];

static GtkWidget *bar1, *bar2;
static guint id1, id2;


/* update the statusbar */
static void statusbar_update(void)
{
  gtk_statusbar_pop(GTK_STATUSBAR(bar1), id1);
  gtk_statusbar_push(GTK_STATUSBAR(bar1), id1, supertracename);

  gtk_statusbar_pop(GTK_STATUSBAR(bar2), id2);
  gtk_statusbar_push(GTK_STATUSBAR(bar2), id2, msg);

  while (gtk_events_pending())
    gtk_main_iteration ();

  gdk_flush();
}

/* initialize the statusbar */
void statusbar_init(GtkWidget *hbox)
{
  GtkWidget *label;

  strcpy(supertracename, "<none>");
  strcpy(msg, "");

  label = gtk_label_new("Current supertrace:");
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 1);
  gtk_widget_show(label);

  bar1 = gtk_statusbar_new();
  gtk_box_pack_start(GTK_BOX(hbox), bar1, FALSE, FALSE, 1);
  gtk_widget_show(bar1);

  id1 = gtk_statusbar_get_context_id(GTK_STATUSBAR(bar1), "toto");

  bar2 = gtk_statusbar_new();
  gtk_box_pack_start(GTK_BOX(hbox), bar2, TRUE, TRUE, 10);
  gtk_widget_show(bar2);

  id2 = gtk_statusbar_get_context_id(GTK_STATUSBAR(bar2), "toto");

  statusbar_update();
}

/* puts the name of the supertrace used in the statusbar */
void statusbar_set_current_supertrace(char *name)
{
  strcpy(supertracename, name);

  statusbar_update();
}

/*messages in teh statusbar */
void statusbar_set_message(char *fmt, ...)
{
  va_list vl;

  va_start(vl, fmt);
  vsprintf(msg, fmt, vl);
  va_end(vl);

  statusbar_update();
}

void statusbar_concat_message(char *fmt, ...)
{
  va_list vl;

  va_start(vl, fmt);
  vsprintf(msg+strlen(msg), fmt, vl);
  va_end(vl);

  statusbar_update();
}

