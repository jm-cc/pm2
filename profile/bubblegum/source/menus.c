/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2006 Dufour Florent <mailto:dufour@enseirb.fr>
 * Copyright (C) 2007 Rigann LUN <mailto:lun@enseirb.fr>
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

#include <stdlib.h>
#include <gtk/gtk.h>
#include "mainwindow.h"
#include "actions.h"
#include "menus.h"
#include "load.h"
  
GtkWidget *menu;
GtkWidget *menu_item;
/*********************************************************************/
/*! Creer le "Menu Fichier" glissant */
void Menu_fichier(GtkWidget *parent, GtkWidget *menubar)
{   
   GtkWidget *nouveau = gtk_image_new_from_stock (GTK_STOCK_NEW, GTK_ICON_SIZE_LARGE_TOOLBAR);
   GtkWidget *ouvrir = open_ico("ico/ouvrir.png");
   GtkWidget *enregistrer = open_ico("ico/enregistrer.png");
   GtkWidget *enregistrerSous = open_ico("ico/enregistrerSous.png");
   GtkWidget *exporterProgramme = open_ico("ico/enregistrerSous.png");
   GtkWidget *quitter = gtk_image_new_from_stock (GTK_STOCK_QUIT, GTK_ICON_SIZE_LARGE_TOOLBAR);

   GtkWidget *annuler = open_ico("ico/annuler.png");
   GtkWidget *refaire = open_ico("ico/refaire.png");
   
   menu = gtk_menu_new();

   menu_item = gtk_image_menu_item_new_with_mnemonic("_Nouveau");
   gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_item), nouveau);
   g_signal_connect(G_OBJECT(menu_item), "activate", G_CALLBACK(Nouveau), parent);
   gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

   menu_item = gtk_image_menu_item_new_with_mnemonic("_Ouvrir...");
   gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_item), ouvrir);
   g_signal_connect(G_OBJECT(menu_item), "activate", G_CALLBACK(Ouvrir), parent);
   gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

   menu_item = gtk_image_menu_item_new_with_mnemonic("_Enregistrer");
   gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_item), enregistrer);
   g_signal_connect(G_OBJECT(menu_item), "activate", G_CALLBACK(Enregistrer), parent);
   gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

   menu_item = gtk_image_menu_item_new_with_mnemonic("Enregi_strer sous...");
   gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_item), enregistrerSous);
   g_signal_connect(G_OBJECT(menu_item), "activate", G_CALLBACK(EnregistrerSous), parent);
   gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

   menu_item = gtk_image_menu_item_new_with_mnemonic("Exporter le programme...");
   gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_item), exporterProgramme);
   g_signal_connect(G_OBJECT(menu_item), "activate", G_CALLBACK(ExporterProgramme), parent);
   gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

   menu_item = gtk_image_menu_item_new_with_mnemonic("_Annuler");
   gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_item), annuler);
   g_signal_connect(G_OBJECT(menu_item), "activate", G_CALLBACK(Annuler), parent);
   gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

   menu_item = gtk_image_menu_item_new_with_mnemonic("_Refaire");
   gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_item), refaire);
   g_signal_connect(G_OBJECT(menu_item), "activate", G_CALLBACK(Refaire), parent);
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

/*********************************************************************/
/*! Creer le "Menu Action" glissant */
void Menu_actions(GtkWidget *parent, GtkWidget *menubar)
{
   GtkWidget *executer = open_ico("ico/executer.png");
   GtkWidget *executer2 = open_ico("ico/executer2.png");
   GtkWidget *options = gtk_image_new_from_stock (GTK_STOCK_PREFERENCES, GTK_ICON_SIZE_LARGE_TOOLBAR);
   
   menu = gtk_menu_new();
   
   menu_item = gtk_image_menu_item_new_with_mnemonic("_Exécuter");
   gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_item), executer);
   g_signal_connect(G_OBJECT(menu_item), "activate", G_CALLBACK(Executer), parent);
   gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
   
   menu_item = gtk_image_menu_item_new_with_mnemonic("E_xécuter Flash");
   gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_item), executer2);
   g_signal_connect(G_OBJECT(menu_item), "activate", G_CALLBACK(ExecuterFlash), parent);
   gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

   menu_item = gtk_image_menu_item_new_with_mnemonic("Options...");
   gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_item), options);
   g_signal_connect(G_OBJECT(menu_item), "activate", G_CALLBACK(Options), parent);
   gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
   
   menu_item = gtk_menu_item_new_with_mnemonic("_Actions");

   gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item), menu);
   gtk_menu_shell_append(GTK_MENU_SHELL(menubar), menu_item);
}

/*********************************************************************/
/*! Creer le "Menu Aide" glissant */
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

