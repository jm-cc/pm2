/**********************************************************************
 * File  : actions.c
 * Author: Dufour Florent
 *         mailto:dufour@enseirb.fr
 * Date  : 27/03/2006
 *********************************************************************/


#include <stdlib.h>
#include <gtk/gtk.h>
#include "mainwindow.h"
#include "actions.h"
#include "load.h"
#include "geneC.h"

void Quitter(GtkWidget *widget, gpointer data)
{
   GtkWidget *dialog;
   GtkWidget *hbox;
   GtkWidget *icone;
   GtkWidget *Question;

   icone = gtk_image_new_from_stock(GTK_STOCK_DIALOG_QUESTION, GTK_ICON_SIZE_DIALOG);
    
   dialog = gtk_dialog_new_with_buttons("Quitter", GTK_WINDOW(data),
                                        GTK_DIALOG_MODAL | GTK_DIALOG_NO_SEPARATOR,
                                        "_Oui", GTK_RESPONSE_YES, "_Non", GTK_RESPONSE_NO, NULL);
                
   gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER_ON_PARENT);
    
   /* Création de le hbox qui va contenir l'icone et la Question */
   hbox = gtk_hbox_new(FALSE, 0);
    
   gtk_box_pack_start(GTK_BOX(hbox), icone, FALSE, FALSE, 10);
       
   Question = gtk_label_new("Voulez vous vraiment\nquitter le programme ?");
   gtk_box_pack_start(GTK_BOX(hbox), Question, FALSE, FALSE, 10);
 
   gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), hbox, FALSE, FALSE, 5);
   gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);   
   gtk_widget_show_all(GTK_DIALOG(dialog)->vbox);
    
   switch(gtk_dialog_run(GTK_DIALOG(dialog)))
   {
      case GTK_RESPONSE_YES:
         gtk_main_quit2(widget, NULL);
         break;
      default:
         gtk_widget_destroy(dialog);
         break;
   }
}

void A_Propos(GtkWidget *widget, gpointer data)
{
   GtkWidget *dialog;
   GtkWidget *hbox;
   GtkWidget *logo;
   GtkWidget *infos;

   if (widget == NULL)
      return;
    
   /* Création boîte de dialogue centrée avec boutton standard OK*/
   dialog = gtk_dialog_new_with_buttons("A Propos ...", GTK_WINDOW(data),
                                        GTK_DIALOG_MODAL | GTK_DIALOG_NO_SEPARATOR,
                                        "_Ok", GTK_RESPONSE_OK, NULL);
    
   gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER_ON_PARENT);

   /* Création de le hbox qui va contenir le logo et les informations */
   hbox = gtk_hbox_new(FALSE, 0);
    
   logo = open_ico("ico/pfag6_logo.png");
   gtk_box_pack_start(GTK_BOX(hbox), logo, FALSE, FALSE, 10);
             
   infos = gtk_label_new("Gestion des bulles et des treads\n\n"
                         "This program is free software and may be distributed according\n"
                         "to the terms of the GNU General Public License\n\n"
                         "Beta Version 0.8 - 2006 PFAG6 Team");
    
   gtk_box_pack_start(GTK_BOX(hbox), infos, FALSE, FALSE, 10);
 
   gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), hbox, FALSE, FALSE, 5); 
   gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE); 
        
   gtk_widget_show_all(GTK_DIALOG(dialog)->vbox);
   gtk_dialog_run(GTK_DIALOG(dialog));
    
   /*La seule action possible */
   gtk_widget_destroy(dialog);
}

void Aide(GtkWidget *widget, gpointer data)
{
   GtkWidget *dialog;
   GtkWidget *hbox;
   GtkWidget *infos;

   if (widget == NULL)
      return;

   /* Création boîte de dialogue centrée avec boutton standard OK*/
   dialog = gtk_dialog_new_with_buttons("Aide", GTK_WINDOW(data),
                                        GTK_DIALOG_MODAL | GTK_DIALOG_NO_SEPARATOR,
                                        "_Ok", GTK_RESPONSE_OK, NULL);

   gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER_ON_PARENT);
   
   /* Création de le hbox qui va contenir les informations */
   hbox = gtk_hbox_new(FALSE, 0);

   infos = gtk_label_new("Raccourcis clavier :\n\n"
                         "Ctrl + q : Quitter le programme\n"
                         "Ctrl + n : Nouveau projet\n"
                         "Ctrl + o : Ouvrir un projet\n"
                         "Ctrl + s : Sauvergarde un projet\n"
                         "Ctrl + s : Fermer un projet\n"
                         "Ctrl + e : Exécution de l'ordonnanceur\n"
                         "Ctrl + f : Exécution de l'ordonnanceur, sortie en flash\n"
                         "Ctrl + Gauche : Bascule sur l'interface de droite\n"
                         "Ctrl + Droite : Bascule sur l'interface de gauche\n"
                         "Ctrl + Haut : Recentre les interfaces\n"
			 "t : Ajouter un thread\n"
			 "b : Ajouter une bulle\n"
			 "Suppr : Supprimer l'�l�ment s�lectionn�\n"
                         "F1 : Aide\n");
                         
   gtk_box_pack_start(GTK_BOX(hbox), infos, FALSE, FALSE, 10);

   gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), hbox, FALSE, FALSE, 5);
   gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);

   gtk_widget_show_all(GTK_DIALOG(dialog)->vbox);
   gtk_dialog_run(GTK_DIALOG(dialog));

   /*La seule action possible */
   gtk_widget_destroy(dialog);
}

void Nouveau(GtkWidget *widget, gpointer data)
{
   /* XXX: memleak !! */
   iGaucheVars->bullePrincipale = CreateBulle(1);
   iGaucheVars->zonePrincipale = iGaucheVars->zoneSelectionnee = CreerZone(0,0,200,100);
   Rearanger(iGaucheVars->zonePrincipale);
}

void Ouvrir(GtkWidget *widget, gpointer data)
{
   GtkWidget *FileSelection;
   GtkWidget *Dialog;
   gchar *Chemin;

   if (widget == NULL)
      return;

   /* Creation de la fenetre de selection */
   FileSelection = gtk_file_chooser_dialog_new("Ouvrir projet", GTK_WINDOW(data),
                                               GTK_FILE_CHOOSER_ACTION_OPEN, "_Annuler", GTK_RESPONSE_CANCEL,
                                               "_Ouvrir", GTK_RESPONSE_OK, NULL);

   gtk_window_set_position(GTK_WINDOW(FileSelection), GTK_WIN_POS_CENTER_ON_PARENT);
   
   /* On limite les actions à cette fenêtre */
   gtk_window_set_modal(GTK_WINDOW(FileSelection), TRUE);
    
   /* Affichage fenetre */
   switch(gtk_dialog_run(GTK_DIALOG(FileSelection)))
   {
      case GTK_RESPONSE_OK:
         /* Recuperation du chemin */
         Chemin = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(FileSelection));
         Dialog = gtk_message_dialog_new(GTK_WINDOW(FileSelection), GTK_DIALOG_MODAL,
                                         GTK_MESSAGE_INFO, GTK_BUTTONS_OK, "Ouverture du fichier :\n%s", Chemin);
         gtk_window_set_position(GTK_WINDOW(Dialog), GTK_WIN_POS_CENTER_ON_PARENT);
         gtk_dialog_run(GTK_DIALOG(Dialog));
         gtk_widget_destroy(Dialog);
         gtk_widget_destroy(FileSelection);
         g_free(Chemin);
         break;
      default:
         gtk_widget_destroy(FileSelection);
         break;
   }
}

void Enregistrer(GtkWidget *widget, gpointer data)
{
   GtkWidget *FileSelection;
   GtkWidget *Dialog;
   gchar *Chemin;

   if (widget == NULL)
      return;
   
   /* Creation de la fenetre de selection */
   FileSelection = gtk_file_chooser_dialog_new("Enregistrer projet", GTK_WINDOW(data),
                                               GTK_FILE_CHOOSER_ACTION_OPEN, "_Annuler", GTK_RESPONSE_CANCEL,
                                               "_Enregistrer", GTK_RESPONSE_OK, NULL);

   gtk_window_set_position(GTK_WINDOW(FileSelection), GTK_WIN_POS_CENTER_ON_PARENT);

   /* On limite les actions à cette fenêtre */
   gtk_window_set_modal(GTK_WINDOW(FileSelection), TRUE);

   /* Affichage fenetre */
   switch(gtk_dialog_run(GTK_DIALOG(FileSelection)))
   {
      case GTK_RESPONSE_OK:
         /* Recuperation du chemin */
         Chemin = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(FileSelection));
         Dialog = gtk_message_dialog_new(GTK_WINDOW(FileSelection), GTK_DIALOG_MODAL,
                                         GTK_MESSAGE_INFO, GTK_BUTTONS_OK, "Sauvegarde de :\n%s", Chemin);
         gtk_window_set_position(GTK_WINDOW(Dialog), GTK_WIN_POS_CENTER_ON_PARENT);
         gtk_dialog_run(GTK_DIALOG(Dialog));
         gtk_widget_destroy(Dialog);
         gtk_widget_destroy(FileSelection);
         g_free(Chemin);
         break;
      default:
         gtk_widget_destroy(FileSelection);
         break;
   }
}

void Basculement_gauche(GtkWidget *widget, gpointer data)
{
   /* data pointe vers une structure BiWidget    */
   /* wg1 comporte un pointeur vers main_window  */
   /* wg2 comporte un pointeur vers hpane        */

   if (widget == NULL)
      return;

   gint width, height;
   gtk_window_get_size(GTK_WINDOW(((BiWidget*)data)->wg1), &width, &height);
   gtk_paned_set_position(GTK_PANED(((BiWidget*)data)->wg2), width);
}

void Centrage_interfaces(GtkWidget *widget, gpointer data)
{
   if (widget == NULL)
      return;

   gint width, height;
   gtk_window_get_size(GTK_WINDOW((((BiWidget*)data)->wg1)), &width, &height);
   gtk_paned_set_position(GTK_PANED((((BiWidget*)data)->wg2)), (gint)(width / 2)- 2);
}

void Basculement_droite(GtkWidget *widget, gpointer data)
{
   if (widget == NULL)
      return;

   gtk_paned_set_position(GTK_PANED(data), 0);
}

void Basculement_gauche_hotkey(gpointer data)
{
   /* fonction spéciale a un seul argument pour la closure des raccourcis */
   Basculement_gauche(((BiWidget*)data)->wg1, data);
}

void Centrage_interfaces_hotkey(gpointer data)
{
   Centrage_interfaces(((BiWidget*)data)->wg1, data);
}

void Basculement_droite_hotkey(gpointer data)
{
   Basculement_droite(((BiWidget*)data)->wg1, data);
}

void Executer(GtkWidget *widget, gpointer data)
{
   GtkWidget *dialog;
   GtkWidget *progress_bar;
   GtkWidget *infos;

   dialog = gtk_dialog_new_with_buttons("Exécution de l'ordonnanceur ...",
                                        GTK_WINDOW(data), GTK_DIALOG_MODAL | GTK_DIALOG_NO_SEPARATOR,
                                        "_Progression de 20%", GTK_RESPONSE_ACCEPT, "_Ok", GTK_RESPONSE_OK, NULL);
                
   gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER_ON_PARENT);
   gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
   
   infos = gtk_label_new("L'ordonnenceur PM2 est en train de s'éxecuter\n\nSoyez patient ...");

   gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), infos, TRUE, FALSE, 10);
   
   progress_bar = gtk_progress_bar_new();
   gtk_progress_bar_set_orientation(GTK_PROGRESS_BAR(progress_bar), GTK_PROGRESS_LEFT_TO_RIGHT);
   gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), progress_bar, TRUE, FALSE, 10);
   
   gtk_widget_show_all(GTK_DIALOG(dialog)->vbox);
   
   // XXX: ne s'affiche pas car on n'appelle pas gtk_dialog_run
   gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress_bar), 0);
   gtk_main_iteration_do(FALSE);
   gen_fichier_C(iGaucheVars->bullePrincipale);
   gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress_bar), 0.2);
   gtk_main_iteration_do(FALSE);
   system("make " GENEC_NAME);
   gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress_bar), 0.4);
   gtk_main_iteration_do(FALSE);
   system("pm2load " GENEC_NAME " --marcel-nvp 4 --marcel-maxarity 2");
   gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress_bar), 0.8);
   gtk_main_iteration_do(FALSE);
   char tracefile[1024];
   snprintf(tracefile,sizeof(tracefile),"/tmp/prof_file_user_%s",getenv("USER"));
   AnimationReset(anim, tracefile);
   gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress_bar), 1);
   gtk_main_iteration_do(FALSE);
   gtk_widget_destroy(dialog);
}

void ExecuterFlash(GtkWidget *widget, gpointer data)
{
   GtkWidget *dialog;
   GtkWidget *progress_bar;
   GtkWidget *infos;

   dialog = gtk_dialog_new_with_buttons("Exécution de l'ordonnanceur ...",
                                        GTK_WINDOW(data), GTK_DIALOG_MODAL | GTK_DIALOG_NO_SEPARATOR,
                                        "_Progression de 20%", GTK_RESPONSE_ACCEPT, "_Ok", GTK_RESPONSE_OK, NULL);
                
   gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER_ON_PARENT);
   gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
   
   infos = gtk_label_new("L'ordonnenceur PM2 est en train de s'éxecuter\n\nSoyez patient ...");

   gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), infos, TRUE, FALSE, 10);
   
   progress_bar = gtk_progress_bar_new();
   gtk_progress_bar_set_orientation(GTK_PROGRESS_BAR(progress_bar), GTK_PROGRESS_LEFT_TO_RIGHT);
   gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), progress_bar, TRUE, FALSE, 10);
   
   gtk_widget_show_all(GTK_DIALOG(dialog)->vbox);
   
   // XXX: ne s'affiche pas car on n'appelle pas gtk_dialog_run
   gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress_bar), 0);
   gtk_main_iteration_do(FALSE);
   gen_fichier_C(iGaucheVars->bullePrincipale);
   gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress_bar), 0.2);
   gtk_main_iteration_do(FALSE);
   char command[1024];
   snprintf(command,sizeof(command),"( make "GENEC_NAME" ; pm2load "GENEC_NAME" --marcel-nvp 4 --marcel-maxarity 2 ; bubbles -x 1024 -y 800 -d /tmp/prof_file_user_%s ; realplay autobulles.swf ) &",getenv("USER"));
   system(command);
   gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress_bar), 1);
   gtk_main_iteration_do(FALSE);
   gtk_widget_destroy(dialog);
}

void Temp(GtkWidget *widget, gpointer data)
{
   GtkWidget *dialog;
   GtkWidget *hbox;
   GtkWidget *icone;
   GtkWidget *infos;

   if (widget == NULL)
      return;

   /* Création boîte de dialogue centrée avec boutton standard OK*/
   dialog = gtk_dialog_new_with_buttons("Information", GTK_WINDOW(data),
                                        GTK_DIALOG_MODAL | GTK_DIALOG_NO_SEPARATOR,
                                        "_Ok", GTK_RESPONSE_OK, NULL);

   gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER_ON_PARENT);

   /* Création de le hbox qui va contenir le logo et les infos */
   hbox = gtk_hbox_new(FALSE, 0);

   icone = open_ico("ico/pfag6_logo.png");
   gtk_box_pack_start(GTK_BOX(hbox), icone, FALSE, FALSE, 10);

   infos = gtk_label_new("Cette fonction n'a pas encore été implémentée ...");

   gtk_box_pack_start(GTK_BOX(hbox), infos, FALSE, FALSE, 10);

   gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), hbox, FALSE, FALSE, 5);
   gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);

   gtk_widget_show_all(GTK_DIALOG(dialog)->vbox);
   gtk_dialog_run(GTK_DIALOG(dialog));

   /* La seule action possible */
   gtk_widget_destroy(dialog);

}

void gtk_main_quit2(GtkWidget* widget, gpointer data)
{
   widget = NULL;  // étiver le warning
   
   printf("fin du programme\n");
   
   gtk_main_quit();
}
