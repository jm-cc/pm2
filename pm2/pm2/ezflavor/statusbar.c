
#include <stdarg.h>
#include <stdio.h>
#include <gtk/gtk.h>

#include "statusbar.h"

static char flavorname[1024];
static char msg[1024];

static GtkWidget *bar1, *bar2;
static guint id1, id2;

static void statusbar_update(void)
{
  gtk_statusbar_pop(GTK_STATUSBAR(bar1), id1);
  gtk_statusbar_push(GTK_STATUSBAR(bar1), id1, flavorname);

  gtk_statusbar_pop(GTK_STATUSBAR(bar2), id2);
  gtk_statusbar_push(GTK_STATUSBAR(bar2), id2, msg);

  while (gtk_events_pending())
    gtk_main_iteration ();

  gdk_flush();
}

void statusbar_init(GtkWidget *hbox)
{
  GtkWidget *label;

  strcpy(flavorname, "<none>");
  strcpy(msg, "");

  label = gtk_label_new("Current flavor:");
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

void statusbar_set_current_flavor(char *name)
{
  strcpy(flavorname, name);

  statusbar_update();
}

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

