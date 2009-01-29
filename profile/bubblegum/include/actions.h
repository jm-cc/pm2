#ifndef ACTIONS_H
#define ACTIONS_H

#include <gtk/gtk.h>

#define GTK_RESPONSE_OPEN 98
#define GTK_RESPONSE_MERGE 99
#define STRING_BUFFER_SIZE 1024

void Quitter(GtkWidget *widget, gpointer data);
void A_Propos(GtkWidget *widget, gpointer data);
void Aide(GtkWidget *widget, gpointer data);
void Nouveau(GtkWidget *widget, gpointer data);
void NouveauTmp(GtkWidget *widget, gpointer data);
void Ouvrir(GtkWidget *widget, gpointer data);
void Enregistrer(GtkWidget *widget, gpointer data);
void EnregistrerSous(GtkWidget *widget, gpointer data);
void ExporterProgramme(GtkWidget *widget, gpointer data);
void Executer(GtkWidget *widget, gpointer data);
void ExecuterFlash(GtkWidget *widget, gpointer data);
void Basculement_gauche(GtkWidget *widget, gpointer data);
void Centrage_interfaces(GtkWidget *widget, gpointer data);
void Basculement_droite(GtkWidget *widget, gpointer data);

void Basculement_gauche_hotkey(gpointer data);
void Centrage_interfaces_hotkey(gpointer data);
void Basculement_droite_hotkey(gpointer data);

void gtk_main_quit2(GtkWidget*, gpointer data);

void Annuler(GtkWidget *widget, gpointer data);
void Refaire(GtkWidget *widget, gpointer data);

const char * DemanderFichier(GtkWidget *FileSelection);
int DemanderConfirmation(gpointer data, char* titre, char * message);

#endif
