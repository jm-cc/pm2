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
