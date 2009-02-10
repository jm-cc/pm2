/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2009 "the PM2 team" (see AUTHORS file)
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

#include <stdlib.h>
#include <gtk/gtk.h>
#include "mainwindow.h"
#include "actions.h"
#include "toolbars.h"
#include "load.h"

/*********************************************************************/

void main_toolbar(GtkWidget *parent, GtkWidget *toolbar, GtkWidget *hpane)
{
   GtkWidget *nouveau = gtk_image_new_from_stock(GTK_STOCK_NEW, GTK_ICON_SIZE_LARGE_TOOLBAR);
   GtkWidget *ouvrir = open_ico("ico/ouvrir.png");
   GtkWidget *enregistrer = open_ico("ico/enregistrer.png");
   GtkWidget *enregistrerSous = open_ico("ico/enregistrerSous.png");;
   GtkWidget *annuler = open_ico("ico/annuler.png");
   GtkWidget *refaire = open_ico("ico/refaire.png");
   GtkWidget *executer = open_ico("ico/executer.png");
   GtkWidget *executer2 = open_ico("ico/executer2.png");
   GtkWidget *basculement_gauche = open_ico("ico/basculement_gauche.png");
   GtkWidget *centrage = open_ico("ico/centrage.png");
   GtkWidget *basculement_droite = open_ico("ico/basculement_droite.png");
   GtkWidget *quit = gtk_image_new_from_stock(GTK_STOCK_QUIT, GTK_ICON_SIZE_LARGE_TOOLBAR);
   GtkWidget *aide = gtk_image_new_from_stock (GTK_STOCK_HELP, GTK_ICON_SIZE_LARGE_TOOLBAR);
   GtkWidget *about = gtk_image_new_from_stock(GTK_STOCK_DIALOG_QUESTION, GTK_ICON_SIZE_LARGE_TOOLBAR);
     
   gtk_toolbar_append_element(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_CHILD_BUTTON,
                              NULL, NULL, "Nouveau projet", NULL, nouveau, G_CALLBACK(Nouveau), parent);
   
   gtk_toolbar_append_element(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_CHILD_BUTTON,
                              NULL, NULL, "Ouvrir un projet", NULL, ouvrir, G_CALLBACK(Ouvrir), parent);
   
   /*g_signal_connect(G_OBJECT(toolbar_item), "enter", G_CALLBACK(Sur_ouvrir),
     GINT_TO_POINTER(iContextId));
     g_signal_connect(G_OBJECT(toolbar_item), "leave", G_CALLBACK(ClearStatus),
     GINT_TO_POINTER(iContextId));*/

   gtk_toolbar_append_element(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_CHILD_BUTTON,
                              NULL, NULL, "Sauvegarder un projet", NULL, enregistrer, G_CALLBACK(Enregistrer), parent);
   
   gtk_toolbar_append_element(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_CHILD_BUTTON,
                              NULL, NULL, "Sauvegarder un projet sous", NULL, enregistrerSous, G_CALLBACK(EnregistrerSous), parent);
   
   gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));
   
   gtk_toolbar_append_element(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_CHILD_BUTTON,
                              NULL, NULL, "Annuler une action", NULL, annuler, G_CALLBACK(Annuler), parent);
   
   gtk_toolbar_append_element(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_CHILD_BUTTON,
			   NULL, NULL, "Refaire une action", NULL, refaire, G_CALLBACK(Refaire), parent);
   
   gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));
     
   gtk_toolbar_append_element(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_CHILD_BUTTON,
                              NULL, NULL, "Exécution de l'ordonnanceur",
                              NULL, executer, G_CALLBACK(Executer), parent);
      
   gtk_toolbar_append_element(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_CHILD_BUTTON,
                              NULL, NULL, "Exécution de l'ordonnanceur, sortie en Flash",
                              NULL, executer2, G_CALLBACK(ExecuterFlash), parent);
      
   gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));

   BiWidget *biwg = malloc(sizeof(BiWidget)); /* cette allocation permet de conserver les elements de cette structure dans le heap, n'est jamais désalloué, mais tant pis. */
   biwg->wg1 = parent;
   biwg->wg2 = hpane;

   gtk_toolbar_append_element(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_CHILD_BUTTON,
                              NULL, NULL, "Bascule sur l'interface de gauche (Ctrl+gauche)", NULL, basculement_gauche,
                              G_CALLBACK(Basculement_gauche), biwg);
   
   gtk_toolbar_append_element(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_CHILD_BUTTON,
                              NULL, NULL, "Recentre les interfaces (Ctrl+haut)", NULL, centrage,
                              G_CALLBACK(Centrage_interfaces), biwg);
      
   gtk_toolbar_append_element(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_CHILD_BUTTON,
                              NULL, NULL, "Bascule sur l'interface de droite (Ctrl+droite)", NULL, basculement_droite,
                              G_CALLBACK(Basculement_droite), hpane);
      
   gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));
   
   gtk_toolbar_append_element(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_CHILD_BUTTON,
                              NULL, NULL, "A Propos ...", NULL, about, G_CALLBACK(A_Propos), parent);
   
   gtk_toolbar_append_element(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_CHILD_BUTTON,
                              NULL, NULL, "Aide (F1)", NULL, aide, G_CALLBACK(Aide), parent);
   
   gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));
     
   gtk_toolbar_append_element(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_CHILD_BUTTON,
                              NULL, NULL, "Quitte le programme (Ctrl+Q)", NULL, quit, G_CALLBACK(Quitter), parent);
}
     
     
     




