
#include <gtk/gtk.h>

#include "main.h"
#include "menu.h"
#include "flavor.h"
#include "module.h"

static GtkItemFactory *item_factory;

static void quit_callback(GtkWidget *w, gpointer data)
{
  if(flavor_modified) {
    flavor_check_quit();
  } else {
    destroy_phase = TRUE;
    gtk_main_quit();
  }
}

static void delete_callback(GtkWidget *w, gpointer data)
{
  flavor_delete();
}

static void new_callback(GtkWidget *w, gpointer data)
{
  flavor_create();
}

static void load_callback(GtkWidget *w, gpointer data)
{
  flavor_load();
}

static void save_callback(GtkWidget *w, gpointer data)
{
  flavor_save_as();
}

static void select_callback(GtkWidget *w, gpointer data)
{
  module_select();
}

static void deselect_callback(GtkWidget *w, gpointer data)
{
  module_deselect();
}

static void dummy_callback(GtkWidget *w, gpointer data)
{
  g_print("Sorry: not yet implemented!\n");
}

static GtkItemFactoryEntry menu_items[] = {
         { "/_Flavor",         NULL,         NULL, 0, "<Branch>" },
	 { "/Flavor/_New",     "<control>N", new_callback, 0, NULL },
         { "/Flavor/_Load",    "<control>O", load_callback, 0, NULL },
         { "/Flavor/_Save",    "<control>S", save_callback, 0, NULL },
         { "/Flavor/Save _As", "<control>V", save_callback, 0, NULL },
         { "/Flavor/sep1",     NULL,         NULL, 0, "<Separator>" },
	 { "/Flavor/_Check",  "<control>C",  dummy_callback, 0, NULL },
	 { "/Flavor/_Delete",  "<control>X", delete_callback, 0, NULL },
         { "/Flavor/sep1",     NULL,         NULL, 0, "<Separator>" },
         { "/Flavor/Quit",     "<control>Q", quit_callback, 0, NULL },
         { "/_Module",         NULL,         NULL, 0, "<Branch>" },
         { "/Module/_Select",  "<control>T", select_callback, 0, NULL },
         { "/Module/_Deselect","<control>D", deselect_callback, 0, NULL },
         { "/_Help",           NULL,         NULL, 0, "<LastBranch>" },
         { "/_Help/About",     NULL,         dummy_callback, 0, NULL },
       };

void menu_init(GtkWidget *vbox)
{
  GtkAccelGroup *accel_group;
  gint nmenu_items = sizeof (menu_items) / sizeof (menu_items[0]);
  GtkWidget *menubar;

  accel_group = gtk_accel_group_new();

  item_factory = gtk_item_factory_new(GTK_TYPE_MENU_BAR, "<main>", 
				      accel_group);

  gtk_item_factory_create_items(item_factory, nmenu_items, menu_items, NULL);

  gtk_window_add_accel_group(GTK_WINDOW(main_window), accel_group);

  menubar = gtk_item_factory_get_widget(item_factory, "<main>");

  gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, FALSE, 0);
  gtk_widget_show(menubar);

  menu_update_flavor(FALSE, FALSE, FALSE, FALSE, FALSE);
  menu_update_module(FALSE, FALSE);
}

void menu_update_flavor(gint load_enabled, gint create_enabled, gint save_enabled,
			gint save_as, gint delete_enabled)
{
  gtk_widget_set_sensitive(gtk_item_factory_get_widget(item_factory,
						       "/Flavor/Load"),
			   load_enabled);

  gtk_widget_set_sensitive(gtk_item_factory_get_widget(item_factory,
						       "/Flavor/New"),
			   create_enabled);

  gtk_widget_set_sensitive(gtk_item_factory_get_widget(item_factory,
						       "/Flavor/Save"),
			   save_enabled && !save_as);

  gtk_widget_set_sensitive(gtk_item_factory_get_widget(item_factory,
						       "/Flavor/Save As"),
			   save_enabled && save_as);

  gtk_widget_set_sensitive(gtk_item_factory_get_widget(item_factory,
						       "/Flavor/Delete"),
			   delete_enabled);

}

void menu_update_module(gint select_enabled, gint deselect_enabled)
{
  gtk_widget_set_sensitive(gtk_item_factory_get_widget(item_factory,
						       "/Module/Select"),
			   select_enabled);
  gtk_widget_set_sensitive(gtk_item_factory_get_widget(item_factory,
						       "/Module/Deselect"),
			   deselect_enabled);
}
