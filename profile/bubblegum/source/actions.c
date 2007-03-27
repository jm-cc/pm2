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
#define BUBBLELIB_PRIV_ENTITIES_TYPE 1
#include "bubblelib_anim.h"


/*********************************************************************/

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

/*********************************************************************/

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
             
  infos = gtk_label_new("Gestion des bulles et des treads\n\n"
			"This program is free software and may be distributed according\n"
			"to the terms of the GNU General Public License\n\n"
			"Beta Version 0.9 - 2007 PFA Team");
    
  gtk_box_pack_start(GTK_BOX(hbox), infos, FALSE, FALSE, 10);
 
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), hbox, FALSE, FALSE, 5); 
  gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE); 
        
  gtk_widget_show_all(GTK_DIALOG(dialog)->vbox);
  gtk_dialog_run(GTK_DIALOG(dialog));
    
  /*La seule action possible */
  gtk_widget_destroy(dialog);
}

/*********************************************************************/

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
			"Ctrl + c : Fermer un projet\n"
			"Ctrl + e : Exécution de l'ordonnanceur\n"
			"Ctrl + f : Exécution de l'ordonnanceur, sortie en flash\n"
			"Ctrl + Gauche : Bascule sur l'interface de droite\n"
			"Ctrl + Droite : Bascule sur l'interface de gauche\n"
			"Ctrl + Haut : Recentre les interfaces\n"
			"Ctrl + t : Ajouter un thread\n"
			"Ctrl + b : Ajouter une bulle\n"
			"Suppr : Supprimer l'élément sélectionné\n"
			"F1 : Aide\n");
                         
  gtk_box_pack_start(GTK_BOX(hbox), infos, FALSE, FALSE, 10);

  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), hbox, FALSE, FALSE, 5);
  gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);

  gtk_widget_show_all(GTK_DIALOG(dialog)->vbox);
  gtk_dialog_run(GTK_DIALOG(dialog));

  /*La seule action possible */
  gtk_widget_destroy(dialog);
}

/*********************************************************************/

void Nouveau(GtkWidget *widget, gpointer data)
{
  /* XXX: memleak !! */
  /* utilisation de effacer */
  iGaucheVars->bullePrincipale = CreateBulle(1, 0);
  iGaucheVars->zonePrincipale = iGaucheVars->zoneSelectionnee = CreerZone(0,0,200,100);
  Rearanger(iGaucheVars->zonePrincipale);
}

/*********************************************************************/

void Ouvrir(GtkWidget *widget, gpointer data)
{
  GtkWidget *FileSelection;
  GtkWidget *Dialog = NULL;
  gchar *Chemin;
  xmlNodePtr racine;

  if (widget == NULL)
    return;

  /* Creation de la fenetre de selection */
  FileSelection = gtk_file_chooser_dialog_new("Ouvrir un fichier", GTK_WINDOW(data),
					      GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					      GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);

  gtk_window_set_position(GTK_WINDOW(FileSelection), GTK_WIN_POS_CENTER_ON_PARENT);
   
  /* On limite les actions a cette fenetre */
  gtk_window_set_modal(GTK_WINDOW(FileSelection), TRUE);
    
  /* Affichage fenetre */
  switch(gtk_dialog_run(GTK_DIALOG(FileSelection)))
    {
    case GTK_RESPONSE_ACCEPT:
      /* Recuperation du chemin */
      Chemin = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(FileSelection));
      Dialog = gtk_message_dialog_new(GTK_WINDOW(FileSelection), GTK_DIALOG_MODAL,
				      GTK_MESSAGE_INFO, GTK_BUTTONS_OK, "Ouverture du fichier :\n%s", Chemin);
      gtk_window_set_position(GTK_WINDOW(Dialog), GTK_WIN_POS_CENTER_ON_PARENT);
      gtk_dialog_run(GTK_DIALOG(Dialog));
      
      /* il suffit d'effacer l'ancienne configuration et de parcourir l'arbre */
      /* Option : Boite de dialogue "La configuration actuelle sera effacee. Continuer ?" */
      Nouveau(NULL, NULL);
      
#if 1 /* ca foire , sais pas pkoi! -> c'était à cause de la dépendence de la libxml*/
      xmlDocPtr xmldoc = NULL;
      xmlNodePtr racine = NULL;
      
      printf("chemin %s\n", Chemin);
      xmldoc = xmlParseFile(Chemin);
      if (!xmldoc)
	{
	  /* Faire une boite de dialogue, pour prévenir que le fichier n'est pas au format XML*/
	  fprintf (stderr, "%s:%d: Erreur d'ouverture du fichier \n", 
		   __FILE__, __LINE__);
	}
      else{
	racine = xmlDocGetRootElement(xmldoc);
	if (racine == NULL) 
	  {
	    fprintf(stderr, "Document XML vierge\n");
	    xmlFreeDoc(xmldoc);
	    return;
	  }
	
	/* Lecture du fichier XML  -> faut-il faire une fonction 
	   qui applique les actions de creation necessaires 
	*/
	
	
	/*On cree les bulles et les threads selon le fichier*/
	else{  
	  lectureNoeud (racine);
	  
	}  
	
	/* on libere */	
	xmlFreeDoc(xmldoc);
	
      }
      
#endif
      
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

void Merger(GtkWidget *widget, gpointer data)
{
  GtkWidget *FileSelection;
  GtkWidget *Dialog = NULL;
  gchar *Chemin;

  if (widget == NULL)
    return;

  /* Creation de la fenetre de selection */
  FileSelection = gtk_file_chooser_dialog_new("Merger un fichier", GTK_WINDOW(data),
					      GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					      GTK_STOCK_OPEN, GTK_RESPONSE_OK, NULL);

  gtk_window_set_position(GTK_WINDOW(FileSelection), GTK_WIN_POS_CENTER_ON_PARENT);
   
  /* On limite les actions a cette fenetre */
  gtk_window_set_modal(GTK_WINDOW(FileSelection), TRUE);
    
  /* Affichage fenetre */
  switch(gtk_dialog_run(GTK_DIALOG(FileSelection)))
    {
    case GTK_RESPONSE_OK:
      /* Recuperation du chemin */
      Chemin = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(FileSelection));
      Dialog = gtk_message_dialog_new(GTK_WINDOW(FileSelection), GTK_DIALOG_MODAL,
				      GTK_MESSAGE_INFO, GTK_BUTTONS_OK, "Merge :\n%s", Chemin);
      gtk_window_set_position(GTK_WINDOW(Dialog), GTK_WIN_POS_CENTER_ON_PARENT);
      gtk_dialog_run(GTK_DIALOG(Dialog));

      /* il suffit d'ouvrir sans effacer les données précédentes */

      
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

void Enregistrer(GtkWidget *widget, gpointer data)
{
  GtkWidget *FileSelection;
  GtkWidget *Dialog;
  gchar *Chemin;

  if (widget == NULL)
    return;
   
  /* Creation de la fenetre de selection */
  FileSelection = gtk_file_chooser_dialog_new("Sauvegarder un fichier", GTK_WINDOW(data),
					      GTK_FILE_CHOOSER_ACTION_SAVE, "_Annuler", GTK_RESPONSE_CANCEL,
					      "_Sauvegarder", GTK_RESPONSE_OK, NULL);

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
      
      /* il suffit de lire l'arbre courant et de le mettre en forme */
#if 1
      /* initialisation de la structure */
      
      xmlDocPtr xmldoc;
      xmldoc = xmlNewDoc ("1.0");
      xmlNodePtr root;
      root = xmlNewNode(NULL, "BubbleGum");
      xmlDocSetRootElement(xmldoc, root);
    
      /* parcours */
      Element * element;
      element=iGaucheVars->bullePrincipale;
      parcourirBullesXml(element, root);
      fprintf(stderr, "parcours ok");
      //parcourirDocXml(iGaucheVars->bullePrincipale, root);
      
      /* sauvegarde de l'arbre  */
      /*  xmlDocFormatDump (Chemin, xmldoc, 1);*/

      xmlSaveFormatFile(Chemin, xmldoc, 1);
      xmlFreeDoc(xmldoc);
#endif
      
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

void Centrage_interfaces(GtkWidget *widget, gpointer data)
{
  if (widget == NULL)
    return;

  gint width, height;
  gtk_window_get_size(GTK_WINDOW((((BiWidget*)data)->wg1)), &width, &height);
  gtk_paned_set_position(GTK_PANED((((BiWidget*)data)->wg2)), (gint)(width / 2)- 2);
}

/*********************************************************************/

void Basculement_droite(GtkWidget *widget, gpointer data)
{
  if (widget == NULL)
    return;

  gtk_paned_set_position(GTK_PANED(data), 0);
}

/*********************************************************************/

void Basculement_gauche_hotkey(gpointer data)
{
  /* fonction spéciale a un seul argument pour la closure des raccourcis */
  Basculement_gauche(((BiWidget*)data)->wg1, data);
}

/*********************************************************************/

void Centrage_interfaces_hotkey(gpointer data)
{
  Centrage_interfaces(((BiWidget*)data)->wg1, data);
}

/*********************************************************************/

void Basculement_droite_hotkey(gpointer data)
{
  Basculement_droite(((BiWidget*)data)->wg1, data);
}


/* TODO: move in a header */
#define STRING_BUFFER_SIZE 1024

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
  gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress_bar), pb_start);
  gtk_main_iteration_do(FALSE);
  pb_start += pb_step;
  
  gen_fichier_C (GENEC_NAME ".c", iGaucheVars->bullePrincipale);

  runCommand ("make -f " GENEC_MAKEFILE " " GENEC_NAME);

  gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress_bar), pb_start);
  gtk_main_iteration_do(FALSE);
  pb_start += pb_step;

  runCommand ("pm2load " GENEC_NAME " --marcel-nvp 4 --marcel-maxarity 2");
  
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
    
  gchar tracefile[STRING_BUFFER_SIZE] = "";

  BubbleMovie mymovie;
    
  initExecDialog (&dialog, &progress_bar, &infos, data);
    
  generateTrace (progress_bar, 0, 0.2);

  snprintf (tracefile, sizeof (tracefile),
	    "/tmp/prof_file_user_%s", getenv ("USER"));

  /* Generates BGLMovie */
  BubbleOps_setBGL();
  mymovie = newBubbleMovieFromFxT (tracefile, NULL);

  /* \todo for Debug purpose : Do not remove the movie here. */
/*   destroyBGLMovie (mymovie); */

/*   AnimationReset(anim, tracefile); */
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

  /*! \todo use user defined output file. */
  const char *out_file = "autobulles.swf";
    
  BubbleMovie mymovie;

  gchar tracefile[STRING_BUFFER_SIZE] = "";

  initExecDialog (&dialog, &progress_bar, &infos, data);	
    
  generateTrace (progress_bar, 0, 0.2);

  snprintf (tracefile, sizeof (tracefile),
	    "/tmp/prof_file_user_%s", getenv ("USER"));

  /* Generates BGLMovie */
  BubbleOps_setSWF();
    
  fprintf(stderr,"tracefile %p %s %d\n", tracefile, tracefile, sizeof(tracefile));

  MOVIEX = 1024;  /* -x bubbles cl arg. */
  MOVIEY = 800;   /* -y bubbles cl arg. */

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

/*********************************************************************/

void Temp(GtkWidget *widget, gpointer data)
{
  GtkWidget *dialog;
  GtkWidget *hbox;
  GtkWidget *icone;
  GtkWidget *infos;

  if (widget == NULL)
    return;

  /* Création boÃ®te de dialogue centrée avec boutton standard OK*/
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

/*********************************************************************/

void gtk_main_quit2(GtkWidget* widget, gpointer data)
{
  widget = NULL;  // éviter le warning
   
  printf("fin du programme\n");
   
  gtk_main_quit();
}
