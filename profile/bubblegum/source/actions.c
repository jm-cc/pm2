/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2006 Florent DUFOUR <mailto:dufour@enseirb.fr>
 * Copyright (C) 2007 Raphaël BOIS <mailto:bois@enseirb.fr>
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

#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <gtk/gtk.h>

#include "animation.h"

#include "mainwindow.h"
#include "actions.h"
#include "load.h"
#include "save.h"
#include "geneC.h"
#include "bulle.h"


#include "bubble_gl_anim.h"

/* avoid enumerator redeclaration error. */
#if 0
#define BUBBLELIB_PRIV_ENTITIES_TYPE 1
#include "bubblelib_anim.h"
#endif

#include "interfaceGauche.h"

extern unsigned int NumTmp;
extern unsigned int NumTmpMax;

static char *bubble_scheduler_name;
static char *synthetic_topology;
static int chosen_bsched = 0;
static gboolean txtViewTopoSensitive = FALSE;

/*********************************************************************/
/*! Demande confirmation avant de quitter le programme */
void Quitter(GtkWidget *widget, gpointer data)
{
  switch(DemanderConfirmation(data, "Quitter", "Voulez-vous vraiment\nquitter le programme ?"))
    {
    case GTK_RESPONSE_YES:
      gtk_main_quit2(widget, NULL);
      break;
    default:
      break;
    }
}

/*********************************************************************/
/*! Callback pour le menu A propos
 */
void A_Propos(GtkWidget *widget, gpointer data)
{
  GtkWidget *dialog;
  GtkWidget *hbox;
  GtkWidget *logo;
  GtkWidget *infos;

  if (widget == NULL)
    return;
    
  /* Création boite de dialogue centrée avec boutton standard OK*/
  dialog = gtk_dialog_new_with_buttons("A Propos ...", GTK_WINDOW(data),
				       GTK_DIALOG_MODAL | GTK_DIALOG_NO_SEPARATOR,
				       "_Ok", GTK_RESPONSE_OK, NULL);
    
  gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER_ON_PARENT);

  /* Création de le hbox qui va contenir le logo et les informations */
  hbox = gtk_hbox_new(FALSE, 0);
    
  logo = open_ico("ico/pfag6_logo.png");
  gtk_box_pack_start(GTK_BOX(hbox), logo, FALSE, FALSE, 10);
             
  infos = gtk_label_new("Gestion des bulles et des threads\n\n"
			"This program is free software and may be distributed according\n"
			"to the terms of the GNU General Public License\n\n"
			"Beta Version 1 - 2007 PFA Team");
    
  gtk_box_pack_start(GTK_BOX(hbox), infos, FALSE, FALSE, 10);
 
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), hbox, FALSE, FALSE, 5); 
  gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE); 
        
  gtk_widget_show_all(GTK_DIALOG(dialog)->vbox);
  gtk_dialog_run(GTK_DIALOG(dialog));
    
  /*La seule action possible */
  gtk_widget_destroy(dialog);
}

/*********************************************************************/
/*! Callback pour l'affichage de l'aide
 */
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
			"F1 : Aide\n"
			"Ctrl+Q : Quitter le programme\n"
			"Ctrl+N : Nouveau projet\n"
			"Ctrl+O : Ouvrir un projet\n"
			"Ctrl+S : Sauvegarder un projet\n"
                        "Ctrl+Shift+S : Sauvegarder un projet sous\n"
			"\n"
			"Ctrl+E : Exécuter l'ordonnanceur\n"
			"Ctrl+F : Exécuter l'ordonnanceur, sortie en flash\n"
			"Ctrl+Gauche : Basculer sur l'interface de droite\n"
			"Ctrl+Droite : Basculer sur l'interface de gauche\n"
			"Ctrl+Haut : Recentrer les interfaces\n"
			"\n"
			"Ctrl+B | F4 : Ajouter une bulle\n"
			"Ctrl+T | F5 : Ajouter un thread\n"
			"Suppr : Supprimer l(es) élément(s) sélectionné(s)\n"
			"Ctrl+souris : Copier l(es) élément(s) sélectionné(s)\n"
			"Ctrl+Z : Annuler la dernière action\n"
			"Ctrl+Y : Refaire la dernière action\n"
			);
                         
  gtk_box_pack_start(GTK_BOX(hbox), infos, FALSE, FALSE, 10);

  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), hbox, FALSE, FALSE, 5);
  gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);

  gtk_widget_show_all(GTK_DIALOG(dialog)->vbox);
  gtk_dialog_run(GTK_DIALOG(dialog));

  /*La seule action possible */
  gtk_widget_destroy(dialog);
}

/*********************************************************************/
/*! Fonction appelé lors d'une demande d'une nouvelle page d'édition
 *  par l'utilisateur, elle a pour objectif d'initialiser toutes les
 *  variables globales et effacer l'arbre de threads et de bulles.
 */ 
void Nouveau(GtkWidget *widget, gpointer data)
{
  NouveauTmp(widget, data);
  /* Mise à jour valeurs pour l'historique */
  NumTmpMax = 0;
  NumTmp = 0;
  
  /* Reset du chemin dans iGaucheVars */
  sprintf(iGaucheVars->chemin,"no path");
  
  /* Reset la base id */
  iGaucheVars->idgeneral = 0;

  return;
}

/*********************************************************************/
/*! Fonction qui est appelé par d'autres fonctions pour effacer
 *  l'arbre de bulles et de threads, aussi bien à l'écran qu'en mémoire.
 *  Voir notatament: 
 *  # chargerXml
 *  # enregisterXml
 */
void NouveauTmp(GtkWidget *widget, gpointer data)
{
  /* XXX: memleak !! */
  /* utilisation de effacer */
  iGaucheVars->bullePrincipale = CreateBulle(1, 0);
  iGaucheVars->zonePrincipale = iGaucheVars->zoneSelectionnee = CreerZone(0,0,200,100);
  Rearanger(iGaucheVars->zonePrincipale);
  /*Slelection Multiple*/
  #if 1
  iGaucheVars->zonePrincipale->next = NULL;
  iGaucheVars->head =  iGaucheVars->zonePrincipale;
  iGaucheVars->last =  iGaucheVars->zonePrincipale;
  iGaucheVars->zoneSelectionnee =  iGaucheVars->zonePrincipale;
  #endif
  /*Fin*/
  
  return;
}


/*********************************************************************/
/*! Fonction appelé lors d'un d'une demande d'ouverture de fichier.
 *  L'utilisateur à la possibilité de simplement mergé le fichier,
 *  dans ce cas il y a ajout des éléments dans la bulle principale.
 */
void Ouvrir(GtkWidget *widget, gpointer data)
{
  GtkWidget *FileSelection;
  GtkWidget *Dialog = NULL;
  gchar *Chemin;
  
  if (widget == NULL)
    return;
  
  /* Creation de la fenetre de selection */
  FileSelection = gtk_file_chooser_dialog_new("Ouvrir/Importer un fichier", GTK_WINDOW(data), GTK_FILE_CHOOSER_ACTION_OPEN,
					      "_Annuler", GTK_RESPONSE_CANCEL,
					      "_Ouvrir", GTK_RESPONSE_OPEN, 
					      "_Importer", GTK_RESPONSE_MERGE, NULL);
  gtk_dialog_set_default_response(GTK_DIALOG(FileSelection), GTK_RESPONSE_OPEN);
  gtk_window_set_position(GTK_WINDOW(FileSelection), GTK_WIN_POS_CENTER_ON_PARENT);
   
  /* On limite les actions a cette fenetre */
  gtk_window_set_modal(GTK_WINDOW(FileSelection), TRUE);
    
  /* Affichage fenetre */
  switch(gtk_dialog_run(GTK_DIALOG(FileSelection)))
    {
    case GTK_RESPONSE_OPEN:
      /* Recuperation du chemin */
      Chemin = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(FileSelection));
      
      NouveauTmp(NULL, NULL);
      

      /* Boite de dialogue, pour prévenir que le fichier n'est pas au format XML
      ou que le chargement n'a pas fonctionné. 
      */
      if(chargerXml(Chemin) == -1){
	Dialog = gtk_message_dialog_new(GTK_WINDOW(FileSelection), GTK_DIALOG_MODAL,
					GTK_MESSAGE_INFO, GTK_BUTTONS_OK, 
					"Problème d'ouverture du fichier :\n%s", Chemin);
	gtk_window_set_position(GTK_WINDOW(Dialog), GTK_WIN_POS_CENTER_ON_PARENT);
	gtk_dialog_run(GTK_DIALOG(Dialog));
	printf("erreur de chargement!\n");
	/* Charge le dernier état connu */
	sprintf(Chemin, "/tmp/bubblegum/bubbblegumT%d.xml", NumTmp);
	chargerXml(Chemin);
      }
      else {
	/* met à jour le chemin dans iGaucheVars */
	sprintf(iGaucheVars->chemin,Chemin);
	/* pour l'historique */
	enregistrerTmp();
      }
      gtk_widget_destroy(Dialog);
      gtk_widget_destroy(FileSelection);
      g_free(Chemin);
      break;
    
    case GTK_RESPONSE_MERGE:
      /* Recuperation du chemin */
      Chemin = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(FileSelection));
      /* Boite de dialogue, pour prévenir que le fichier n'est pas au format XML
	 ou que le chargement n'a pas fonctionné. 
      */
      if(chargerXml(Chemin) == -1){
	Dialog = gtk_message_dialog_new(GTK_WINDOW(FileSelection), GTK_DIALOG_MODAL,
					GTK_MESSAGE_INFO, GTK_BUTTONS_OK, 
					"Problème d'ouverture du fichier :\n%s", Chemin);
	gtk_window_set_position(GTK_WINDOW(Dialog), GTK_WIN_POS_CENTER_ON_PARENT);
	gtk_dialog_run(GTK_DIALOG(Dialog));
	printf("erreur de chargement!\n");
      }
      else {
	/* pour l'historique */
	enregistrerTmp();
      }

      gtk_widget_destroy(Dialog);
      gtk_widget_destroy(FileSelection);
      g_free(Chemin);
      break;
    default:
      gtk_widget_destroy(FileSelection);
      break;
    }
}

/*********************************************************************/
/*! Fonction appelée lors d'un d'une demande de sauvegarde de fichier,
 *  Cette fonction est appelé lorsque soit l'utilisateur le demande
 *  explicitement, soit que le chemin de sauvegarde n'as pas été entré.
 */
void EnregistrerSous(GtkWidget *widget, gpointer data)
{
  GtkWidget *FileSelection;
  char Chemin[128];
  
  if (widget == NULL)
    return;
  
  strcpy(Chemin, "no path");

  /* Creation de la fenetre de selection */
  FileSelection = gtk_file_chooser_dialog_new("Sauvegarder un fichier", GTK_WINDOW(data), GTK_FILE_CHOOSER_ACTION_SAVE,
					      "_Annuler", GTK_RESPONSE_CANCEL,
					      "_Sauvegarder", GTK_RESPONSE_OK, NULL);
  gtk_dialog_set_default_response(GTK_DIALOG(FileSelection), GTK_RESPONSE_OK);
  strcpy(Chemin,DemanderFichier(FileSelection));
  
  if (strcmp(Chemin, "no path") == 0)
    return;
  else {
    enregistrerXml(Chemin, iGaucheVars->bullePrincipale);
    
    /* met à jour le chemin dans iGaucheVars */
    sprintf(iGaucheVars->chemin, Chemin);
  }
  
  return;
}

void ExporterProgramme (GtkWidget *widget, gpointer data)
{
  GtkWidget *FileSelection;
  char Chemin[128];
  
  if (widget == NULL)
    return;
  
  strcpy(Chemin, "no path");

  /* Creation de la fenetre de selection */
  FileSelection = gtk_file_chooser_dialog_new("Enregistrer le programme g�n�r� sous...", GTK_WINDOW(data), GTK_FILE_CHOOSER_ACTION_SAVE,
					      "_Annuler", GTK_RESPONSE_CANCEL,
					      "_Sauvegarder", GTK_RESPONSE_OK, NULL);
  gtk_dialog_set_default_response(GTK_DIALOG(FileSelection), GTK_RESPONSE_OK);
  strcpy(Chemin,DemanderFichier(FileSelection));
  
  if (strcmp(Chemin, "no path") == 0)
    return;
  else {
    char command[256];
    gen_fichier_C (GENEC_NAME ".c", iGaucheVars->bullePrincipale);
    snprintf (command, 256, "cp %s.c %s", GENEC_NAME, Chemin);
    system (command);
    
    /* met à jour le chemin dans iGaucheVars */
    sprintf(iGaucheVars->chemin, Chemin);
  }
  
  return;
}


/*********************************************************************/
/*! Fonction appelée lors d'un d'une demande de sauvegarde de fichier.
 *  C'est la fonction courante d'enregistrement de l'utilisateur.
 */
void Enregistrer(GtkWidget *widget, gpointer data)
{
  if (widget == NULL)
    return;
    
  if (strcmp(iGaucheVars->chemin,"no path") == 0)
    EnregistrerSous(widget, data);
  else
    enregistrerXml(iGaucheVars->chemin, iGaucheVars->bullePrincipale);
  
}

/***************************************************************************
 * Pour l'historique
 */
/*! Permet d'annuler une action effectuer 
 */
void Annuler(GtkWidget *widget, gpointer data) {
  /* Si aucune action n'a été faite */
  if (NumTmp <= 0)
    return;
  
  NouveauTmp(NULL, NULL);
  /* Pour la premiere action */
  if(NumTmp == 0)
    return;

  NumTmp--;
  
  char chemin[128];
  sprintf(chemin, "/tmp/bubblegum/bubbblegumT%d.xml", NumTmp);
#if 0  
  printf("DEBUG :%d\n", NumTmp);
  printf("DEBUG :%s\n", chemin);
#endif
  
  chargerXml(chemin);
  return;
}

/*********************************************************************/
/*! Permet de refaire une action annuler 
 */
void Refaire(GtkWidget *widget, gpointer data) {
  /* Si le fichier n'existe pas, on ne fait rien */
  if (NumTmp >= NumTmpMax)
    return;
  
  NouveauTmp(NULL, NULL);
  char chemin[128];
  NumTmp++;
  sprintf(chemin, "/tmp/bubblegum/bubbblegumT%d.xml", NumTmp);
#if 0  
  printf("DEBUG :%d\n", NumTmp);
  printf("DEBUG :%s\n", chemin);
#endif
  
  chargerXml(chemin);
  return;
}

static void 
activate_txtBox (GtkWidget *widget, gpointer data)
{
  txtViewTopoSensitive = !txtViewTopoSensitive;
  gtk_widget_set_sensitive (GTK_WIDGET (data), txtViewTopoSensitive);
  if (!txtViewTopoSensitive)
    gtk_entry_set_text (GTK_ENTRY (data), "");
}

void Options (GtkWidget *widget, gpointer data)
{
  GtkWidget *dialog;
  GtkWidget *hboxBubbleSched, *hboxTopo;
  GtkWidget *txtViewTopo;
  GtkWidget *chkBtnTopo;
  GtkWidget *labelBubbleSched, *labelTopo;
  GtkWidget *comboBoxBubbleSched;

  if (widget == NULL)
    return;

  /* Création boîte de dialogue centrée avec boutton standard OK*/
  dialog = gtk_dialog_new_with_buttons("Options", GTK_WINDOW(data),
				       GTK_DIALOG_MODAL | GTK_DIALOG_NO_SEPARATOR,
				       "_Ok", GTK_RESPONSE_OK, NULL);

  gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER_ON_PARENT);
   
  /* Création de le hbox qui va contenir les informations */
  hboxBubbleSched = gtk_hbox_new (FALSE, 0);
  comboBoxBubbleSched = gtk_combo_box_new_text();
  gtk_combo_box_append_text ((GtkComboBox *)comboBoxBubbleSched, "null"); 
  gtk_combo_box_append_text ((GtkComboBox *)comboBoxBubbleSched, "cache");
  gtk_combo_box_append_text ((GtkComboBox *)comboBoxBubbleSched, "memory");
  gtk_combo_box_append_text ((GtkComboBox *)comboBoxBubbleSched, "spread");
  gtk_combo_box_set_active ((GtkComboBox *)comboBoxBubbleSched, chosen_bsched);
  labelBubbleSched = gtk_label_new ("Bubble Scheduler:");
                         
  gtk_box_pack_start (GTK_BOX (hboxBubbleSched), labelBubbleSched, FALSE, FALSE, 5);
  gtk_box_pack_start (GTK_BOX (hboxBubbleSched), comboBoxBubbleSched, FALSE, FALSE, 5);
  gtk_box_pack_start (GTK_BOX(GTK_DIALOG(dialog)->vbox), hboxBubbleSched, FALSE, FALSE, 5);

  hboxTopo = gtk_hbox_new (FALSE, 0);
  chkBtnTopo = gtk_check_button_new_with_label ("Use a synthetic topology");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (chkBtnTopo), txtViewTopoSensitive);
  txtViewTopo = gtk_entry_new ();
  gtk_widget_set_sensitive (GTK_WIDGET (txtViewTopo), txtViewTopoSensitive);
  g_signal_connect (G_OBJECT(chkBtnTopo), "clicked", G_CALLBACK (activate_txtBox), txtViewTopo);
  if(synthetic_topology)
    gtk_entry_set_text ((GtkEntry *)txtViewTopo, synthetic_topology);
  labelTopo = gtk_label_new ("Synthetic Topology:");
                         
  gtk_box_pack_start (GTK_BOX (hboxTopo), labelTopo, FALSE, FALSE, 5);
  gtk_box_pack_start (GTK_BOX (hboxTopo), txtViewTopo, FALSE, FALSE, 5);
  gtk_box_pack_start (GTK_BOX(GTK_DIALOG(dialog)->vbox), chkBtnTopo, FALSE, FALSE, 5);
  gtk_box_pack_start (GTK_BOX(GTK_DIALOG(dialog)->vbox), hboxTopo, FALSE, FALSE, 5);

  gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);

  gtk_widget_show_all(GTK_DIALOG(dialog)->vbox);
  gtk_dialog_run(GTK_DIALOG(dialog));

  if (synthetic_topology != NULL)
    free (synthetic_topology);

  if (bubble_scheduler_name != NULL)
    free (bubble_scheduler_name);

  synthetic_topology = (char *)gtk_entry_get_text (GTK_ENTRY (txtViewTopo));
  if (*synthetic_topology == '\0')
    synthetic_topology = NULL;
  else
    synthetic_topology = strdup (synthetic_topology);

  bubble_scheduler_name = strdup (gtk_combo_box_get_active_text (GTK_COMBO_BOX (comboBoxBubbleSched)));
  chosen_bsched = gtk_combo_box_get_active (GTK_COMBO_BOX (comboBoxBubbleSched));

/*La seule action possible */
  gtk_widget_destroy(dialog);
}

/*********************************************************************/
/*! Permet de mettre en plein écran la fenêtre de gauche.
*/
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

/*********************************************************************/
/*! Permet de couper en deux et de façon égale la fenêtre principale.
*/
void Centrage_interfaces(GtkWidget *widget, gpointer data)
{
  if (widget == NULL)
    return;

  gint width, height;
  gtk_window_get_size(GTK_WINDOW((((BiWidget*)data)->wg1)), &width, &height);
  gtk_paned_set_position(GTK_PANED((((BiWidget*)data)->wg2)), (gint)(width / 2)- 2);
}

/*********************************************************************/
/*! Permet de mettre en plein écran la fenêtre de droite.
*/
void Basculement_droite(GtkWidget *widget, gpointer data)
{
  if (widget == NULL)
    return;

  gtk_paned_set_position(GTK_PANED(data), 0);
}

/*********************************************************************/
/*! Permet de mettre en plein écran la fenêtre de gauche.
*/
void Basculement_gauche_hotkey(gpointer data)
{
  /* fonction spéciale a un seul argument pour la closure des raccourcis */
  Basculement_gauche(((BiWidget*)data)->wg1, data);
}

/*********************************************************************/
/*! Permet de couper en deux et de façon égale la fenêtre principale.
*/
void Centrage_interfaces_hotkey(gpointer data)
{
  Centrage_interfaces(((BiWidget*)data)->wg1, data);
}

/*********************************************************************/
/*! Permet de mettre en plein écran la fenêtre de droite.
*/
void Basculement_droite_hotkey(gpointer data)
{
  Basculement_droite(((BiWidget*)data)->wg1, data);
}

/*! Initializes the execution dialog box.
 *
 *  \param pdialog  a pointer of pointer to receive the dialog box.
 *  \param pprogressBar
 *                  a pointer of pointer to receive the progress bar.
 *  \param pinfos   a pointer of pointer to receive the info label.
 *  \param data     user data.
 *
 *  \warning The dialog box don't currently displays.
 */
static void
initExecDialog (GtkWidget **pdialog, GtkWidget **pprogressBar,
                GtkWidget **pinfos, gpointer data) {
  GtkWidget *dialog;
  GtkWidget *progressBar;
  GtkWidget *infos;

  dialog =
    gtk_dialog_new_with_buttons("Exécution de l'ordonnanceur ...",
				GTK_WINDOW(data),
				GTK_DIALOG_MODAL | GTK_DIALOG_NO_SEPARATOR,
				"_Progression de 20%", GTK_RESPONSE_ACCEPT,
				"_Ok", GTK_RESPONSE_OK, NULL);
                
  gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER_ON_PARENT);
  gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
   
  infos = gtk_label_new("L'ordonnanceur PM2 est en train de s'exécuter\n"
			"\n"
			"Soyez patient ...");

  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), infos,
		     TRUE, FALSE, 10);
   
  progressBar = gtk_progress_bar_new();
  gtk_progress_bar_set_orientation(GTK_PROGRESS_BAR(progressBar),
				   GTK_PROGRESS_LEFT_TO_RIGHT);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), progressBar,
		     TRUE, FALSE, 10);
   
  gtk_widget_show_all(GTK_DIALOG(dialog)->vbox);
	
  /* XXX: ne s'affiche pas car on n'appelle pas gtk_dialog_run */
  gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progressBar), 0);
  gtk_main_iteration_do(FALSE);
  
  if (pdialog)
    *pdialog = dialog;
  if (pprogressBar)
    *pprogressBar = progressBar;
  if (pinfos)
    *pinfos = infos;
}

/*! Destroys the Dialog box. 
 *
 *  \param dialog   a pointer to a GtkWidget.
 *  \param progressBar
 *                  a pointer to a GtkProgressBar.
 *  \param infos    a pointer to a GtkLabel.
 */
static void
destroyExecDialog (GtkWidget *dialog, GtkWidget *progressBar,
                   GtkWidget *infos) {
  gtk_widget_destroy (dialog);

  dialog      = NULL;
  progressBar = NULL;
  infos       = NULL;
}

/*! Runs a system command.
 *  \param command_fmt
 *                  a printf format string.
 *  \param ...
 */
static void
runCommand (const char *command_fmt, ...) {
  va_list vl;
    
  gchar command[STRING_BUFFER_SIZE] = "";
  int ret;
    
  va_start (vl, command_fmt);
    
  /* TODO: dynamic size ? */
  vsnprintf (command, sizeof (command), command_fmt, vl);
    
  ret = system (command);
    
  if (!WIFEXITED (ret)) {
    if (WIFSIGNALED (ret)) {
      printf ("command got signal %d\n", WTERMSIG (ret));
            
      if (WCOREDUMP (ret))
	printf ("core dumped\n");
    } else
      printf ("return status %d\n", ret);
  }
	
  va_end (vl);
}


/*! Generates a trace.
 *
 *  \param progress_bar
 *                  a pointer to a GtkProgressBar.
 *  \param pb_start an integer representing the start value of the progress bar.
 *  \param pb_step  an integer representing the step value for the progress bar.
 */
static void
generateTrace (GtkWidget *progress_bar, float pb_start, float pb_step) {  
  char *pm2_command;

  gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress_bar), pb_start);
  gtk_main_iteration_do(FALSE);
  pb_start += pb_step;
  
  gen_fichier_C (GENEC_NAME ".c", iGaucheVars->bullePrincipale);

  runCommand ("make -f " GENEC_MAKEFILE " " GENEC_NAME);

  gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress_bar), pb_start);
  gtk_main_iteration_do(FALSE);
  pb_start += pb_step;

  asprintf (&pm2_command, 
	    "pm2-load  %s --marcel-bubble-scheduler \"%s\" %s%s%s", 
	    GENEC_NAME,
	    bubble_scheduler_name ? : "null", 
	    synthetic_topology ? " --marcel-synthetic-topology \"" : "", 
	    synthetic_topology ? : "",
	    synthetic_topology ? "\"" : "");

  printf ("%s\n", pm2_command);
  runCommand (pm2_command);
  free (pm2_command);
  
  gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress_bar), pb_start);
  gtk_main_iteration_do(FALSE);
  pb_start += pb_step;

}


/*! Builds openGL animation and runs it.
 *
 */
void Executer(GtkWidget *widget, gpointer data) {
  GtkWidget *dialog       = NULL;
  GtkWidget *progress_bar = NULL;
  GtkWidget *infos        = NULL;
    
  gchar tracefile[STRING_BUFFER_SIZE];

  BubbleMovie mymovie;
    
  initExecDialog (&dialog, &progress_bar, &infos, data);
    
  generateTrace (progress_bar, 0, 0.2);

  snprintf (tracefile, sizeof (tracefile),
	    "/tmp/prof_file_user_%s", getenv ("USER"));

  /* Generates BGLMovie */
  BubbleOps_setBGL();
  mymovie = newBubbleMovieFromFxT (tracefile, NULL);

  /* destroy previous movie */
  destroyBubbleMovie (AnimationData_getMovie (anim));
  resetAnimationData (anim, mymovie);

  destroyExecDialog (dialog, progress_bar, infos);
}

/*! Builds Flash animation and runs it.
 *
 */
void ExecuterFlash(GtkWidget *widget, gpointer data)
{
  GtkWidget *dialog       = NULL;
  GtkWidget *progress_bar = NULL;
  GtkWidget *infos        = NULL;
  
  gchar out_file[STRING_BUFFER_SIZE] = "/tmp/bubblegum/autobulles.swf";
  GtkWidget *FileSelection;
  
  /* ask if user wants save */
  switch(DemanderConfirmation(data, "Enregistrement Flash", "Voulez-vous enregistrer la sortie flash?"))
    {
    case GTK_RESPONSE_YES:
      /* Creation de la fenetre de selection */
      FileSelection = gtk_file_chooser_dialog_new("Sauvegarder un fichier", GTK_WINDOW(data), 
						  GTK_FILE_CHOOSER_ACTION_SAVE,
						  "_Annuler", GTK_RESPONSE_CANCEL,
						  "_Sauvegarder", GTK_RESPONSE_OK, NULL);
        
      snprintf (out_file, STRING_BUFFER_SIZE, DemanderFichier(FileSelection));
      break;
    }
  
  BubbleMovie mymovie;
  
  gchar tracefile[STRING_BUFFER_SIZE];
  
  initExecDialog (&dialog, &progress_bar, &infos, data);	
  
  generateTrace (progress_bar, 0, 0.2);
  
  snprintf (tracefile, sizeof (tracefile),
	    "/tmp/prof_file_user_%s", getenv ("USER"));
  
  /* Generates BGLMovie */
  BubbleOps_setSWF();

  mymovie = newBubbleMovieFromFxT (tracefile, out_file);

#if 0 /*! \todo implements destroySWFMovie */
  destroyBubbleMovie (mymovie);
#endif

  runCommand (
#if 0
	      "bubbles -x 1024 -y 800 -d %s && "
#endif
	      "%s %s &",
#if 0
	      tracefile,
#endif
# ifdef DARWIN_SYS
	      "open",
# else
	      "realplay",
# endif
	      out_file
	      );
    
  destroyExecDialog (dialog, progress_bar, infos);
}


/*! Fonction qui ouvre une boite de dialogue pour que l'utilisateur
 *  selectionne le chemin désiré.
 *  \param window
 *  \return une chaîne de caractère qui est le chemin choisi.
 */
const char * DemanderFichier(GtkWidget *FileSelection) {
  GtkWidget *Dialog;
  gchar *Chemin;

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
				      GTK_MESSAGE_INFO, GTK_BUTTONS_OK, 
				      "Sauvegarde du fichier :\n%s", Chemin);
      gtk_window_set_position(GTK_WINDOW(Dialog), GTK_WIN_POS_CENTER_ON_PARENT);
      gtk_dialog_run(GTK_DIALOG(Dialog));
      
      /* On libère */
      gtk_widget_destroy(Dialog);
      gtk_widget_destroy(FileSelection);
      
      return Chemin;
      break;
    default:
      gtk_widget_destroy(FileSelection);
      break;
    }
  Chemin = "no path";
  return Chemin;
}

/*! Fonction qui affiche une boite de dialogue pour confirmer une action
 *  \param data
 *  \param titre titre de la boite de dialogue.
 *  \param message message d'action à confirmer.
 */
int DemanderConfirmation(gpointer data, char* titre, char * message) {
  GtkWidget *dialogQ; 
  GtkWidget *hbox;
  GtkWidget *icon;
  GtkWidget *Question;
  int confirm;
  
  icon = gtk_image_new_from_stock(GTK_STOCK_DIALOG_QUESTION, GTK_ICON_SIZE_DIALOG);
    
  dialogQ = gtk_dialog_new_with_buttons(titre, GTK_WINDOW(data),
				       GTK_DIALOG_MODAL | GTK_DIALOG_NO_SEPARATOR,
				       "_Oui", GTK_RESPONSE_YES, "_Non", GTK_RESPONSE_NO, NULL);
                
  gtk_window_set_position(GTK_WINDOW(dialogQ), GTK_WIN_POS_CENTER_ON_PARENT);
    
  /* hbox with question and icon */
  hbox = gtk_hbox_new(FALSE, 0);
    
  gtk_box_pack_start(GTK_BOX(hbox), icon, FALSE, FALSE, 10);
       
  Question = gtk_label_new(message);
  
  gtk_box_pack_start(GTK_BOX(hbox), Question, FALSE, FALSE, 10);
 
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialogQ)->vbox), hbox, FALSE, FALSE, 5);
  gtk_window_set_resizable(GTK_WINDOW(dialogQ), FALSE);   
  gtk_widget_show_all(GTK_DIALOG(dialogQ)->vbox);
  
  
  confirm = gtk_dialog_run(GTK_DIALOG(dialogQ));
  
  gtk_widget_destroy(dialogQ);
  
  return confirm;
}


/*********************************************************************/
/*! Quite le programme bubblegum
 *  Efface en sortant le répertoire des fichiers temporaires
 */
void gtk_main_quit2(GtkWidget* widget, gpointer data)
{
  widget = NULL;  // éviter le warning
  
  /* efface les fichiers temporaire */
  system("rm -rf /tmp/bubblegum/");
  
  printf("Fin du programme\n");
   
  gtk_main_quit();
}
