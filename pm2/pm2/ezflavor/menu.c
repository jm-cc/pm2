
#include <gtk/gtk.h>

#include "main.h"
#include "menu.h"

static void dummy_callback(GtkWidget *w, gpointer data)
{
  g_print("Sorry: menus are not yet implemented!\n");
}

static GtkItemFactoryEntry menu_items[] = {
         { "/_Flavor",         NULL,         NULL, 0, "<Branch>" },
	 { "/Flavor/_New",     "<control>N", dummy_callback, 0, NULL },
         { "/Flavor/_Load",    "<control>O", dummy_callback, 0, NULL },
         { "/Flavor/_Save",    "<control>S", dummy_callback, 0, NULL },
         { "/Flavor/sep1",     NULL,         NULL, 0, "<Separator>" },
         { "/Flavor/Quit",     "<control>Q", gtk_main_quit, 0, NULL },
         { "/_Module",         NULL,         NULL, 0, "<Branch>" },
         { "/Module/_Select",  NULL,         dummy_callback, 0, NULL },
         { "/Module/_Deselect",NULL,         dummy_callback, 0, NULL },
         { "/_Help",           NULL,         NULL, 0, "<LastBranch>" },
         { "/_Help/About",     NULL,         dummy_callback, 0, NULL },
       };

void menu_init(GtkWidget *vbox)
{
  GtkItemFactory *item_factory;
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
}

