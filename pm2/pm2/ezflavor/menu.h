
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
