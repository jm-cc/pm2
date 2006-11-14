/**********************************************************************
 * File  : raccourcis.c
 * Author: Dufour Florent
 *         mailto:dufour@enseirb.fr
 * Date  : 07/04/2006
 *********************************************************************/


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
                           g_cclosure_new_swap(G_CALLBACK(Temp), window, NULL));
   
   gtk_accel_group_connect(accel_group, GDK_o, GDK_CONTROL_MASK, 0,
                           g_cclosure_new_swap(G_CALLBACK(Ouvrir), window, NULL));
   
   gtk_accel_group_connect(accel_group, GDK_s, GDK_CONTROL_MASK, 0,
                           g_cclosure_new_swap(G_CALLBACK(Enregistrer), window, NULL));
   
   gtk_accel_group_connect(accel_group, GDK_c, GDK_CONTROL_MASK, 0,
                           g_cclosure_new_swap(G_CALLBACK(Temp), window, NULL));
   
   gtk_accel_group_connect(accel_group, GDK_e, GDK_CONTROL_MASK, 0,
                           g_cclosure_new_swap(G_CALLBACK(Executer), window, NULL));

   // pour ces fonctions la, soyons certain que ca marchera en redefinissant des closure qui vont bien:   
   gtk_accel_group_connect(accel_group, GDK_Right, GDK_CONTROL_MASK, 0,
                           g_cclosure_new_swap(G_CALLBACK(Basculement_gauche_hotkey), biwg, NULL));
   
   gtk_accel_group_connect(accel_group, GDK_Up, GDK_CONTROL_MASK, 0,
                           g_cclosure_new_swap(G_CALLBACK(Centrage_interfaces_hotkey), biwg, NULL));
   
   gtk_accel_group_connect(accel_group, GDK_Left, GDK_CONTROL_MASK, 0,
                           g_cclosure_new_swap(G_CALLBACK(Basculement_droite_hotkey), pane, NULL));

   gtk_accel_group_connect(accel_group, GDK_Delete, 0, 0,
                           g_cclosure_new_swap(G_CALLBACK(deleteRec2), iGaucheVars, NULL));

   gtk_window_add_accel_group(GTK_WINDOW(window), accel_group);
      
}
