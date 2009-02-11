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

/**********************************************************************
 * File  : mainwindow.c
 * Author: Dufour Florent
 *         mailto:dufour@enseirb.fr
 * Date  : 27/03/2006
 *********************************************************************/

#include "animation.h"
#include "mainwindow.h"
#include "util.h"
#include <locale.h>
#include <sys/stat.h>

interfaceGaucheVars* iGaucheVars;


int main (int argc, char **argv) {
   /* Création des Widgets */
   GtkWidget *main_window;
   GtkWidget *iGauche;
   GtkWidget *vbox;
   GtkWidget *menubar;
   GtkWidget *toolbar;
   GtkWidget *hpane;

   GtkWidget *left_viewport;
   GtkWidget *left_frame;

   GtkWidget *right_viewport;
   GtkWidget *right_window;
   AnimationData *p_anim;

   setlocale (LC_ALL,"");

   /* Initialisation de gtk */
   gtk_init (&argc, &argv);

   /* Initiliasation d'opengl */
   gtk_gl_init (&argc, &argv);

   /* Parametrage de la fenetre main_window */
   main_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
   gtk_window_set_title (GTK_WINDOW (main_window), "BubbleGum");
   gtk_window_set_default_size (GTK_WINDOW (main_window),
                                WINDOW_WIDTH, WINDOW_HEIGHT);
   gtk_window_set_position (GTK_WINDOW (main_window), GTK_WIN_POS_CENTER);

   iGaucheVars = interfaceGauche();
   iGauche = iGaucheVars->interfaceGauche;   /* gtk_window_new(GTK_WINDOW_TOPLEVEL); */



   /* Connexion au signal destroy */
   g_signal_connect (G_OBJECT(main_window), "destroy",
                     G_CALLBACK(gtk_main_quit2), NULL);

   g_signal_connect (G_OBJECT(main_window), "delete_event",
                     G_CALLBACK(gtk_main_quit2), NULL);

   /* Création de la Vbox et ajout dans la main_window */
   vbox = gtk_vbox_new (FALSE, 0);
   gtk_container_add (GTK_CONTAINER (main_window), vbox);

   /* Création de la barre de menus */
   menubar = gtk_menu_bar_new();
   gtk_box_pack_start (GTK_BOX (vbox), menubar, FALSE, FALSE, 0);

   Menu_fichier (main_window, menubar);
   Menu_actions (main_window, menubar);
   Menu_aide (main_window, menubar);

   /* Création du découpeur vertical */
   hpane = gtk_hpaned_new();

   /* Création de la toolbar */
   toolbar = gtk_toolbar_new();
   gtk_box_pack_start (GTK_BOX (vbox), toolbar, FALSE, FALSE, 0);

   main_toolbar (main_window, toolbar, hpane);

   /* Connexion des raccourcis */
   Creer_raccourcis (main_window, hpane);

   /* Séparateur de type pane entre interfaces */
   gtk_box_pack_start (GTK_BOX(vbox), hpane, TRUE, TRUE, 0);

   /* Création viewport de gauche */
   left_viewport = gtk_scrolled_window_new (NULL, NULL);
   gtk_paned_pack1 (GTK_PANED (hpane), left_viewport, TRUE, TRUE);
   gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (left_viewport),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

   /* Insertion de l'interface gauche dans le viewport de gauche*/
   left_frame = gtk_frame_new ("Interface Gauche");
   gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (left_viewport),
                                          iGauche);

   /* Création viewport de droite */
   right_viewport = gtk_scrolled_window_new (NULL, NULL);
   gtk_paned_pack2 (GTK_PANED (hpane), right_viewport, TRUE, TRUE);
   gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (right_viewport),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

   /* Récupération de la vbox de droite par la fonction right_window() */

   p_anim = newAnimationData ();
   right_window = right_window_init (p_anim);
   if (right_window)
       gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (right_viewport),
                                              right_window);
   else {
       /*! \todo Destroys p_anim. */
       /*! XXX: memleak.  */
       /*! \todo Disable main menu buttons related to right window. */
   }

   /* Création de la barre d'état */
   /*    statusbar = gtk_statusbar_new(); */
   /*    gtk_box_pack_end(GTK_BOX(vbox), statusbar, FALSE, FALSE, 0); */
   /*    iContextId = gtk_statusbar_get_context_id(GTK_STATUSBAR(statusbar), "ContextStack"); */

   /* Affiche tous les Widgets de la fenêtre principale*/
   gtk_widget_show_all(main_window);

   /* Boucle principale */
   create_tmp_directory();
   gtk_main();

   return EXIT_SUCCESS;
}
