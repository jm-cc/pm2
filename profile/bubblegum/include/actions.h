#ifndef ACTIONS_H
#define ACTIONS_H

#include <gtk/gtk.h>

#include <libxml/tree.h>
#include <libxml/xpath.h>

void Quitter(GtkWidget *widget, gpointer data);
void A_Propos(GtkWidget *widget, gpointer data);
void Aide(GtkWidget *widget, gpointer data);
void Nouveau(GtkWidget *widget, gpointer data);
void Ouvrir(GtkWidget *widget, gpointer data);
void Enregistrer(GtkWidget *widget, gpointer data);
void Executer(GtkWidget *widget, gpointer data);
void ExecuterFlash(GtkWidget *widget, gpointer data);
void Basculement_gauche(GtkWidget *widget, gpointer data);
void Centrage_interfaces(GtkWidget *widget, gpointer data);
void Basculement_droite(GtkWidget *widget, gpointer data);

void Basculement_gauche_hotkey(gpointer data);
void Centrage_interfaces_hotkey(gpointer data);
void Basculement_droite_hotkey(gpointer data);

void Temp(GtkWidget *widget, gpointer data);

void gtk_main_quit2(GtkWidget*, gpointer data);

#endif
