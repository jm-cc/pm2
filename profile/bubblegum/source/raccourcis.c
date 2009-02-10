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
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include "mainwindow.h"
#include "actions.h"
#include "toolbars.h"
#include "raccourcis.h"
#include "rightwindow.h"
#include "interfaceGauche.h"

void myfunc(){
printf("Detected");
}

void Creer_raccourcis(GtkWidget* window, GtkWidget* pane)
{
   GtkAccelGroup* accel_group;
   
   accel_group = gtk_accel_group_new();
   
   BiWidget* biwg = malloc(sizeof(BiWidget));  // nouvelle structure dans le heap
   biwg->wg1 = window;  // stockant les @ de ces widgets
   biwg->wg2 = pane;
   
   /* Edition des raccourcis */
   gtk_accel_group_connect(accel_group, GDK_q, GDK_CONTROL_MASK, 0,
                           g_cclosure_new_swap(G_CALLBACK(Quitter), window, NULL));
   // en fait ca c'est un peu crade, vu que les callbacks sont prévues pour GTK a la base
   // elles ont deux arguments: widget et gpointer.
   // or une callback de type 'closure' de la glib, n'a qu'un argument void* : gpointer.
   // donc, les liaisons faites par ces accelerateurs écrivent leur argument dans le premier
   // des callback gtk, et laisse le deuxieme indeterminé, ce n'est pas grave dans ce sens :)
   
   gtk_accel_group_connect(accel_group, GDK_F1, 0, 0,
                           g_cclosure_new_swap(G_CALLBACK(Aide), window, NULL));
   
   gtk_accel_group_connect(accel_group, GDK_n, GDK_CONTROL_MASK, 0,
                           g_cclosure_new_swap(G_CALLBACK(Nouveau), window, NULL));
   
   gtk_accel_group_connect(accel_group, GDK_o, GDK_CONTROL_MASK, 0,
                           g_cclosure_new_swap(G_CALLBACK(Ouvrir), window, NULL));
   
   gtk_accel_group_connect(accel_group, GDK_s, GDK_CONTROL_MASK, 0,
                           g_cclosure_new_swap(G_CALLBACK(Enregistrer), window, NULL));
   
   gtk_accel_group_connect(accel_group, GDK_s, GDK_CONTROL_MASK | GDK_SHIFT_MASK, 0,
                           g_cclosure_new_swap(G_CALLBACK(EnregistrerSous), window, NULL));
   
   gtk_accel_group_connect(accel_group, GDK_z, GDK_CONTROL_MASK, 0,
                           g_cclosure_new_swap(G_CALLBACK(Annuler), window, NULL));

   gtk_accel_group_connect(accel_group, GDK_y, GDK_CONTROL_MASK, 0,
                           g_cclosure_new_swap(G_CALLBACK(Refaire), window, NULL));

   gtk_accel_group_connect(accel_group, GDK_e, GDK_CONTROL_MASK, 0,
                           g_cclosure_new_swap(G_CALLBACK(Executer), window, NULL));
   
   gtk_accel_group_connect(accel_group, GDK_f, GDK_CONTROL_MASK, 0,
                           g_cclosure_new_swap(G_CALLBACK(ExecuterFlash), window, NULL));

   // pour ces fonctions la, soyons certains que ça marchera en redéfinissant des closure qui vont bien:   
   gtk_accel_group_connect(accel_group, GDK_Right, GDK_CONTROL_MASK, 0,
                           g_cclosure_new_swap(G_CALLBACK(Basculement_gauche_hotkey), biwg, NULL));
   
   gtk_accel_group_connect(accel_group, GDK_Up, GDK_CONTROL_MASK, 0,
                           g_cclosure_new_swap(G_CALLBACK(Centrage_interfaces_hotkey), biwg, NULL));
   
   gtk_accel_group_connect(accel_group, GDK_Left, GDK_CONTROL_MASK, 0,
                           g_cclosure_new_swap(G_CALLBACK(Basculement_droite_hotkey), pane, NULL));

   gtk_accel_group_connect(accel_group, GDK_Delete, 0, 0,
                           g_cclosure_new_swap(G_CALLBACK(deleteRec2), iGaucheVars, NULL));
   
   gtk_accel_group_connect(accel_group, GDK_BackSpace, 0, 0,
                           g_cclosure_new_swap(G_CALLBACK(deleteRec2), iGaucheVars, NULL));
   
   gtk_accel_group_connect(accel_group,GDK_w,GDK_BUTTON_PRESS_MASK | 
			   GDK_CONTROL_MASK, 0 ,
			   g_cclosure_new_swap(G_CALLBACK(addThreadAutoOnOff), iGaucheVars, NULL));

   gtk_accel_group_connect(accel_group, GDK_b, GDK_CONTROL_MASK , 0,
			   g_cclosure_new_swap(G_CALLBACK(addBulleAutoOnOff), iGaucheVars, NULL));

   gtk_accel_group_connect(accel_group, GDK_t, GDK_CONTROL_MASK, 0,
			   g_cclosure_new_swap(G_CALLBACK(addThreadAutoOnOff), iGaucheVars, NULL));
   
   gtk_accel_group_connect(accel_group, GDK_F4, 0, 0,
                           g_cclosure_new_swap(G_CALLBACK(addBulleAutoOnOff), iGaucheVars, NULL));
   
   gtk_accel_group_connect(accel_group, GDK_F5, 0, 0,
                           g_cclosure_new_swap(G_CALLBACK(addThreadAutoOnOff), iGaucheVars, NULL));
   
   gtk_window_add_accel_group(GTK_WINDOW(window), accel_group);
   
}
