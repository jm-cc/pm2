/**********************************************************************
 * File  : mainwindow.c
 * Author: Dufour Florent
 *         mailto:dufour@enseirb.fr
 * Date  : 27/03/2006
 *********************************************************************/


#include <stdlib.h>
#include <gtk/gtk.h>
#include "mainwindow.h"
#include "actions.h"
#include "menus.h"
#include "load.h"
  
GtkWidget *menu;
GtkWidget *menu_item;

void Menu_fichier(GtkWidget *parent, GtkWidget *menubar)
{   
   GtkWidget *nouveau = gtk_image_new_from_stock (GTK_STOCK_NEW, GTK_ICON_SIZE_LARGE_TOOLBAR);
   GtkWidget *ouvrir = open_ico("ico/ouvrir.png");
   GtkWidget *enregistrer = open_ico("ico/enregistrer.png");
   GtkWidget *fermer = gtk_image_new_from_stock (GTK_STOCK_CLOSE, GTK_ICON_SIZE_LARGE_TOOLBAR);
   GtkWidget *quitter = gtk_image_new_from_stock (GTK_STOCK_QUIT, GTK_ICON_SIZE_LARGE_TOOLBAR);
   
   menu = gtk_menu_new();

   menu_item = gtk_image_menu_item_new_with_mnemonic("_Nouveau");
   gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_item), nouveau);
   g_signal_connect(G_OBJECT(menu_item), "activate", G_CALLBACK(Temp), parent);
   gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

   menu_item = gtk_image_menu_item_new_with_mnemonic("_Ouvrir");
   gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_item), ouvrir);
   g_signal_connect(G_OBJECT(menu_item), "activate", G_CALLBACK(Ouvrir), parent);
   gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

   menu_item = gtk_image_menu_item_new_with_mnemonic("_Enregistrer");
   gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_item), enregistrer);
   g_signal_connect(G_OBJECT(menu_item), "activate", G_CALLBACK(Enregistrer), parent);
   gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

   menu_item = gtk_image_menu_item_new_with_mnemonic("_Fermer");
   gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_item), fermer);
   g_signal_connect(G_OBJECT(menu_item), "activate", G_CALLBACK(Temp), parent);
   gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

   menu_item = gtk_separator_menu_item_new();
   gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

   menu_item = gtk_image_menu_item_new_with_mnemonic("_Quitter");
   gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_item), quitter);
   g_signal_connect(G_OBJECT(menu_item), "activate", G_CALLBACK(Quitter), parent);
   gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

   menu_item = gtk_menu_item_new_with_mnemonic("_Fichier");

   gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item), menu);
   gtk_menu_shell_append(GTK_MENU_SHELL(menubar), menu_item);
}

void Menu_actions(GtkWidget *parent, GtkWidget *menubar)
{
   GtkWidget *executer = open_ico("ico/executer.png");
   
   menu = gtk_menu_new();
   
   menu_item = gtk_image_menu_item_new_with_mnemonic("_Exécuter");
   gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_item), executer);
   g_signal_connect(G_OBJECT(menu_item), "activate", G_CALLBACK(Executer), parent);
   gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
   
   menu_item = gtk_menu_item_new_with_mnemonic("_Actions");

   gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item), menu);
   gtk_menu_shell_append(GTK_MENU_SHELL(menubar), menu_item);
}

void Menu_aide(GtkWidget *parent, GtkWidget *menubar)
{
   GtkWidget *aide = gtk_image_new_from_stock (GTK_STOCK_HELP, GTK_ICON_SIZE_LARGE_TOOLBAR);
   GtkWidget *about = gtk_image_new_from_stock (GTK_STOCK_DIALOG_QUESTION, GTK_ICON_SIZE_LARGE_TOOLBAR);
   
   menu = gtk_menu_new();
   
   menu_item = gtk_image_menu_item_new_with_mnemonic("_Aide");
   gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_item), aide);
   g_signal_connect(G_OBJECT(menu_item), "activate", G_CALLBACK(Aide), parent);
   gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
   
   menu_item = gtk_separator_menu_item_new();
   gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
   
   menu_item = gtk_image_menu_item_new_with_mnemonic("A _Propos");
   gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_item), about);
   g_signal_connect(G_OBJECT(menu_item), "activate", G_CALLBACK(A_Propos), parent);
   gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item); 
   
   menu_item = gtk_menu_item_new_with_label("?");
   
   gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item), menu);
   gtk_menu_shell_append(GTK_MENU_SHELL(menubar), menu_item);
}
