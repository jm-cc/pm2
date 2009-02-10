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
#include <gdk/gdkkeysyms.h>
#include "interfaceGauche.h"
#include "load.h"
#include "save.h"
#include "mainwindow.h"
#include "actions.h"
#include "util.h"

unsigned int NumTmp = 0;
unsigned int NumTmpMax = 0;
/* Selection Multiple*/
#if 1
extern interfaceGaucheVars* iGaucheVars;
#endif
/*Fin*/

/*! Change la charge d un thread selectionné
 * \todo comm
 */
static gboolean changecharge(GtkWidget * w,GtkScrollType scroll,gdouble value, gpointer data){
  if(iGaucheVars->ThreadSelect!=NULL){
    SetCharge(iGaucheVars->ThreadSelect,gtk_range_get_value(GTK_RANGE((DataAddThread*)iGaucheVars->charge)));
    enregistrerTmp();
  }

  else
    iGaucheVars->defcharge=gtk_range_get_value(GTK_RANGE((DataAddThread*)iGaucheVars->charge));

  return FALSE;
}

/*Change la priorite d un thread selectionné
 * \todo comm
 */
static gboolean changepriorite(GtkWidget * w,GtkScrollType scroll,gdouble value, gpointer data){
  if(iGaucheVars->ThreadSelect!=NULL){
    SetPrioriteThread(iGaucheVars->ThreadSelect,gtk_range_get_value(GTK_RANGE((DataAddThread*)iGaucheVars->priorite)));
    enregistrerTmp();
  }
  else
    iGaucheVars->defpriorite=gtk_range_get_value(GTK_RANGE((DataAddThread*)iGaucheVars->priorite));

  return FALSE;
}

/*Change le nom d'un thread selectionné
 * \todo comm
 */
static gboolean changenom(GtkWidget   *widget,  GdkEventKey *event, gpointer  user_data){
  printf("changenom\n");
  if(iGaucheVars->ThreadSelect!=NULL){
    SetNom(iGaucheVars->ThreadSelect,gtk_entry_get_text(GTK_ENTRY((DataAddThread*)iGaucheVars->nom)));
    enregistrerTmp();
  }
  else
    iGaucheVars->defnom=gtk_entry_get_text(GTK_ENTRY((DataAddThread*)iGaucheVars->nom));

  return FALSE;
}

/*Change la priorité d'une bulle sélectionnée
 * \todo comm
 */
static gboolean changeprioritebulle(GtkWidget * w,GtkScrollType scroll,gdouble value, gpointer data){
  printf("change valeur priorite bulle\n");
  if(iGaucheVars->BulleSelect!=NULL){
    SetPrioriteBulle(iGaucheVars->BulleSelect,gtk_range_get_value(GTK_RANGE((DataAddThread*)iGaucheVars->prioritebulle)));
    enregistrerTmp();
  }
  else
    iGaucheVars->defprioritebulle=gtk_range_get_value(GTK_RANGE((DataAddThread*)iGaucheVars->prioritebulle));

  return FALSE;
}

/*! Fonction qui fait les enregistrements temporaires
 *  afin de garder un historique des actions.
 *  \param NumTmp est une variable globale.
 *  \param NumTmpMax est une variable globale.
 */
void enregistrerTmp(void) {
  char chemin[STRING_BUFFER_SIZE], *ptr;

  NumTmp++;
  NumTmpMax = NumTmp;

  ptr = (char *)chemin;
  get_tmp_bubblegum_file(NumTmp, &ptr);
#if 0
  printf("DEBUG : Save %s\n", chemin);
  printf("DEBUG : NumTmp %d NumTmpMax %d\n", NumTmp, NumTmpMax);
#endif
  enregistrerXml(chemin, iGaucheVars->bullePrincipale);
}

interfaceGaucheVars* interfaceGauche()
{
  GtkWidget *b_addBulle = open_ico("ico/addBulle.png");
  GtkWidget *b_addThread = open_ico("ico/addThread.png");
  GtkWidget *b_deleteRec = open_ico("ico/deleteRec.png");
  GtkWidget *b_encapsuler = open_ico("ico/encapsuler.png");
  GtkWidget *toolbar;
  GtkWidget *toolbar2;
  GtkWidget *toolbar3;

  GtkWidget *toolbarB;
  GtkWidget *toolbarV;
  GtkWidget *toolbarG;

  GtkWidget *toolbar_item;
  GtkWidget *menu;
  GtkWidget *item1;
  GtkWidget *item2;
  GtkWidget *item3;
  GtkWidget *ev_box;
  interfaceGaucheVars *iGaucheVars;

  iGaucheVars = malloc(sizeof(interfaceGaucheVars));
  sprintf(iGaucheVars->chemin,"no path");
  iGaucheVars->interfaceGauche = gtk_vbox_new(FALSE, 0);
  iGaucheVars->mode_auto = MODE_AUTO_ON;

  /* initialisation des valeurs par défaut pour créer les bulles et threads*/
  iGaucheVars->idgeneral=0; // on initialise l'identification
  iGaucheVars->ThreadSelect=NULL; // aucun thread n est séléctionné au début
  iGaucheVars->BulleSelect=NULL; // aucune bulle n est séléctionnée au début
  iGaucheVars->defpriorite=1;
  iGaucheVars->defcharge=10;
  iGaucheVars->defprioritebulle=1;
  iGaucheVars->defnom="";

  make_left_drawable_zone(iGaucheVars);


  /*menu*/
  menu = gtk_menu_new ();
  item1 = gtk_menu_item_new_with_label ("Option 1");
  gtk_menu_shell_append (GTK_MENU_SHELL(menu), item1);
  g_signal_connect_swapped (G_OBJECT(item1),"activate",G_CALLBACK(imprimer_rep), (gpointer) "L'option 1 a été cliquée.");

  item2 = gtk_menu_item_new_with_label ("Option 2");
  gtk_menu_shell_append (GTK_MENU_SHELL(menu), item2);
  g_signal_connect_swapped (G_OBJECT(item2),"activate",G_CALLBACK(imprimer_rep), (gpointer) "L'option 2 a été cliquée.");

  item3 = gtk_menu_item_new_with_label ("Option 3");
  gtk_menu_shell_append (GTK_MENU_SHELL(menu), item3);
  g_signal_connect_swapped (G_OBJECT(item3),"activate",G_CALLBACK(imprimer_rep), (gpointer) "L'option 3 a été cliquée.");

  gtk_widget_show_all (menu);

  /*event box*/
  ev_box = gtk_event_box_new ();
  /*    gtk_container_add (GTK_CONTAINER(drawzone), ev_box); */
  g_signal_connect_swapped (G_OBJECT(ev_box), "event", G_CALLBACK(menuitem_rep), G_OBJECT(menu));

  toolbar = gtk_toolbar_new();

  toolbarG = gtk_toolbar_new();
  gtk_toolbar_set_orientation(GTK_TOOLBAR(toolbarG),GTK_ORIENTATION_VERTICAL);

  /* ajout de boutons dans la barre d'outil */
  toolbarB = gtk_toolbar_new();

  toolbar_item = gtk_toolbar_append_element(GTK_TOOLBAR(toolbarB), GTK_TOOLBAR_CHILD_BUTTON,
					    NULL, NULL, "Nouvelle bulle", NULL, b_addBulle, G_CALLBACK(addBulleAutoOnOff), iGaucheVars);
  toolbar_item = gtk_toolbar_append_element(GTK_TOOLBAR(toolbarB), GTK_TOOLBAR_CHILD_BUTTON,
					    NULL, NULL, "Nouveau thread", NULL, b_addThread, G_CALLBACK(addThreadAutoOnOff), iGaucheVars);
  toolbar_item = gtk_toolbar_append_element(GTK_TOOLBAR(toolbarB), GTK_TOOLBAR_CHILD_BUTTON,
					    NULL, NULL, "Supprimer l'element recursivement", NULL, b_deleteRec, G_CALLBACK(deleteRec), iGaucheVars);
  toolbar_item = gtk_toolbar_append_element(GTK_TOOLBAR(toolbarB), GTK_TOOLBAR_CHILD_BUTTON,
					    NULL, NULL, "Encapsuler", NULL, b_encapsuler, G_CALLBACK(encapsuler), iGaucheVars);
  gtk_toolbar_append_widget(GTK_TOOLBAR(toolbarG),toolbarB,NULL,NULL);


  toolbarV = gtk_toolbar_new();

  iGaucheVars->prioritebulle=gtk_hscale_new_with_range(0,10,1);
  gtk_range_set_value(GTK_RANGE(iGaucheVars->prioritebulle), iGaucheVars->defprioritebulle);

  gtk_widget_set_size_request(iGaucheVars->prioritebulle, 150,  40);

  g_signal_connect( GTK_OBJECT(iGaucheVars->prioritebulle), "button-release-event", GTK_SIGNAL_FUNC(changeprioritebulle), (gpointer) "coucou");


  gtk_toolbar_append_widget(GTK_TOOLBAR(toolbarV),gtk_label_new("Priorité (B)"),NULL,NULL);
  gtk_toolbar_append_widget(GTK_TOOLBAR(toolbarV),iGaucheVars->prioritebulle,NULL,NULL);
  gtk_toolbar_append_widget(GTK_TOOLBAR(toolbarG),toolbarV,NULL,NULL);

  gtk_toolbar_append_widget(GTK_TOOLBAR(toolbar),toolbarG,NULL,NULL);

/* Ajout des étiquettes charge-priorité-nom en vertical */
  toolbar2 = gtk_vbox_new (TRUE, 10);
  gtk_box_pack_start (GTK_BOX (toolbar2), gtk_label_new ("\n   Charge"), TRUE, TRUE, 5);
  gtk_box_pack_start (GTK_BOX (toolbar2), gtk_label_new ("\n   Priorité"), TRUE, TRUE, 5);
  gtk_box_pack_start (GTK_BOX (toolbar2), gtk_label_new ("Nom"), TRUE, TRUE, 5);

  gtk_toolbar_append_widget(GTK_TOOLBAR(toolbar),toolbar2,NULL,NULL);

  /* Ajout des ascenceurs plus la zone texte pour le nom */
  toolbar3 = gtk_vbox_new (TRUE, 10);
  iGaucheVars->charge=gtk_hscale_new_with_range(0,MAX_CHARGE,10);
  gtk_range_set_value(GTK_RANGE(iGaucheVars->charge), iGaucheVars->defcharge);
  iGaucheVars->priorite=gtk_hscale_new_with_range(0,MAX_PRIO,1);
  gtk_range_set_value(GTK_RANGE(iGaucheVars->priorite), iGaucheVars->defpriorite);
  iGaucheVars->nom=gtk_entry_new();

  // On associe une fonction au changement de valeurs
  g_signal_connect( GTK_OBJECT(iGaucheVars->charge), "button-release-event", GTK_SIGNAL_FUNC(changecharge), (gpointer) "coucou");
  g_signal_connect( GTK_OBJECT(iGaucheVars->priorite), "button-release-event", GTK_SIGNAL_FUNC(changepriorite), (gpointer) "coucou");
  g_signal_connect( GTK_OBJECT(iGaucheVars->nom),"key-release-event" ,GTK_SIGNAL_FUNC(changenom), (gpointer) "coucou");

  gtk_box_pack_start (GTK_BOX (toolbar3), iGaucheVars->charge, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (toolbar3), iGaucheVars->priorite, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (toolbar3), iGaucheVars->nom, TRUE, TRUE, 0);


  gtk_toolbar_append_widget(GTK_TOOLBAR(toolbar),toolbar3,NULL,NULL);


  gtk_box_pack_end(GTK_BOX(iGaucheVars->interfaceGauche), toolbar, FALSE, FALSE, 0);


  return iGaucheVars;
}



void addBulleAutoOnOff(GtkWidget* pWidget, gpointer data)
{
  //interfaceGaucheVars* iGaucheVars = (interfaceGaucheVars*) data;
  DataAddBulle* dataAddBulle;

  if (iGaucheVars->mode_auto == MODE_AUTO_OFF)
    {
      addBulleAutoOff(data);
    }
  else
    {
      dataAddBulle = malloc(sizeof (DataAddBulle));
      dataAddBulle->popup = NULL;
      dataAddBulle->pScrollbar = NULL;

      dataAddBulle->idBulle = 0;
      dataAddBulle->iGaucheVars = iGaucheVars;
      AddBulle(NULL, dataAddBulle);
    }
  enregistrerTmp();
  return;
}

void addThreadAutoOnOff(GtkWidget* pWidget, gpointer data)
{
  //interfaceGaucheVars* iGaucheVars = (interfaceGaucheVars*) data;
  DataAddThread* dataAddThread;

  dataAddThread = malloc( sizeof (DataAddThread) );
  dataAddThread->prioriteScrollbar = iGaucheVars->priorite;
  dataAddThread->chargeScrollbar = iGaucheVars->charge;
  dataAddThread->nomEntry = iGaucheVars->nom;

  dataAddThread->idThread =SetId(iGaucheVars);
  dataAddThread->idEntry = NULL;
  dataAddThread->popup = NULL;
  dataAddThread->iGaucheVars = iGaucheVars;
  AddThread(NULL, dataAddThread);

  enregistrerTmp();
  return;
}

/* fonction qui affiche une fenêtre popup pour rentrer les attributs
 * d'une bulle */
void addBulleAutoOff(interfaceGaucheVars* data)
{
  //interfaceGaucheVars* iGaucheVars = (interfaceGaucheVars*) data;

  GtkWidget *popup = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  GtkWidget *pVBox = gtk_vbox_new(FALSE, 0);
  GtkWidget *pFrame = gtk_frame_new("Priorité");
  GtkWidget *Adjust = (GtkWidget*)gtk_adjustment_new(DEF_PRIO, 0, MAX_PRIO, 1, 10, 1);
  GtkWidget *pScrollbar = gtk_hscrollbar_new(GTK_ADJUSTMENT(Adjust));
  GtkWidget *pLabel = gtk_label_new("0");
  GtkWidget *prioriteVBox = gtk_vbox_new(FALSE, 0);
  GtkWidget *boutonOk = gtk_button_new_with_mnemonic("OK");

  /* création de la structure servant à stocker les donnees pour les
   * CallBack */
  DataAddBulle* dataAddBulle = malloc(sizeof (DataAddBulle));
  dataAddBulle->popup = popup;
  dataAddBulle->pScrollbar = pScrollbar;
  dataAddBulle->iGaucheVars = iGaucheVars;

  Element* bulleParent;
  parcours* p;
  p = TrouverParcours(dataAddBulle->iGaucheVars->zonePrincipale, LireZoneX(dataAddBulle->iGaucheVars->zoneSelectionnee) + 1, LireZoneY(dataAddBulle->iGaucheVars->zoneSelectionnee)+1);
  bulleParent = LireElementParcours(dataAddBulle->iGaucheVars->bullePrincipale, p);

  if(GetTypeElement(bulleParent) == BULLE)
    {

      /* creation de la popup pour la création d'une bulle */
      gtk_window_set_title(GTK_WINDOW(popup), g_locale_to_utf8("Nouvelle bulle",
                                                               -1, NULL, NULL, NULL));
      gtk_window_set_default_size(GTK_WINDOW(popup), 300, 50);
      gtk_window_set_position(GTK_WINDOW(popup), GTK_WIN_POS_CENTER);

      /* ajout des éléments dans cette fenêtre */
      gtk_container_add(GTK_CONTAINER(pFrame), prioriteVBox);
      gtk_box_pack_start(GTK_BOX(prioriteVBox), pLabel, FALSE, FALSE, 10);
      gtk_box_pack_start(GTK_BOX(prioriteVBox), pScrollbar, TRUE, TRUE, 10);

      /* Connexion du signal pour modification de l affichage */
      g_signal_connect(G_OBJECT(pScrollbar), "value-changed",
                       G_CALLBACK(OnScrollbarChange), (GtkWidget*)pLabel);


      g_signal_connect(G_OBJECT(boutonOk), "clicked",
                       G_CALLBACK(AddBulle), dataAddBulle);

      gtk_container_add(GTK_CONTAINER(popup), pVBox);

      gtk_box_pack_start(GTK_BOX(pVBox), pFrame, FALSE, FALSE, 20);
      gtk_box_pack_start(GTK_BOX(pVBox), boutonOk, TRUE, FALSE, 20);

      /* Affichage de la popup */
      gtk_widget_show_all(popup);
    }
  enregistrerTmp();
  return;
}

#if 1
void  deleteRec(GtkWidget* pWidget, gpointer data)
{
 zone * zoneSeleceted = iGaucheVars->head, *next;
 while(zoneSeleceted != NULL)
{
  next = zoneSeleceted->next;
  if(iGaucheVars->zonePrincipale != zoneSeleceted)
    {
      Effacer(iGaucheVars->bullePrincipale, iGaucheVars->zonePrincipale,
              LireZoneX(zoneSeleceted) + 1,
              LireZoneY(zoneSeleceted) + 1);
    }
   Rearanger(iGaucheVars->zonePrincipale);
   zoneSeleceted = next;
 }
  iGaucheVars->zonePrincipale->next = NULL;
  iGaucheVars->zoneSelectionnee = iGaucheVars->zonePrincipale;
  iGaucheVars->head = iGaucheVars->zonePrincipale;
  //enregistrerTmp();
  return;

}
#endif

#if 0
void deleteRec(GtkWidget* pWidget, gpointer data)
{
  //interfaceGaucheVars* iGaucheVars = (interfaceGaucheVars*)(data);
  zone *tmp = iGaucheVars->head;
  while(tmp != NULL){
  if(iGaucheVars->zonePrincipale != iGaucheVars->zoneSelectionnee)
    {
      Effacer(iGaucheVars->bullePrincipale, iGaucheVars->zonePrincipale,
              LireZoneX(iGaucheVars->zoneSelectionnee) + 1,
              LireZoneY(iGaucheVars->zoneSelectionnee) + 1);
    }

  Rearanger(iGaucheVars->zonePrincipale);

  //iGaucheVars->zoneSelectionnee = iGaucheVars->zonePrincipale;
  enregistrerTmp();
  return;

}
#endif
void deleteRec2(GtkWidget* pWidget, gpointer data)
{
  deleteRec(NULL,(gpointer)pWidget);
  enregistrerTmp();
  return;
}

/*Selection Multiple*/
#if 1
/*! Encapsuler la sélection dans une bulle
 *
 */
void encapsuler(GtkWidget* pWidget, gpointer data)
{
  /*printf("...\n");
    printf("DEBUG : appel encapsuler\n"); */
  parcours *p, *pParent;
  pWidget = NULL;  // inutilisé
  interfaceGaucheVars* iGaucheVarsTmp = (interfaceGaucheVars*)data;
  zone * ZoneSelectionnee, *ZoneParent, *z_encapsulation,*tmp;
  Element* elementSelectionne, * elementParent, *nouvelleBulle;
  if(iGaucheVars->zoneSelectionnee == iGaucheVars->zonePrincipale)
    {
      /* printf("DEBUG : encapsuler, zoneSelectionnee = zonePrincipale, encapsulation impossible\n"); */
      return;
    }
  /* on crée la zone qui va contenir cette nouvelle bulle */
  z_encapsulation = CreerZone(0,0,LARGEUR_B, HAUTEUR_B);
  /* Création de la bulle d'accueil avec pour paramètre la priorité par défaut d'une bulle et une nouvelle id */
  nouvelleBulle = CreateBulle(iGaucheVars->defprioritebulle, SetId(iGaucheVars));
  tmp = iGaucheVars->head;
  while(tmp!=NULL && tmp != iGaucheVars->zonePrincipale){

  /* Voilà l'élément sélectionné qu'on va encapsuler */
  p = TrouverParcours(iGaucheVars->zonePrincipale, tmp->posX + 1, tmp->posY + 1);
  /* wprintf(L"Trace parcours élément à déplacer\n");
     traceParcours(p); */
  ZoneSelectionnee = LireZoneParcours(iGaucheVars->zonePrincipale, p);
  //iGaucheVars->zoneSelectionnee = ZoneSelectionnee;
  elementSelectionne = LireElementParcours(iGaucheVars->bullePrincipale, p);
  EffacerParcours(p);
  /* printf("DEBUG : idSelectionB : %d\n", elementSelectionne->bulle.id); */
  /*  printf("DEBUG : idSelectionT : %d\n", elementSelectionne->thread.id); */

  /* Voilà l'élément parent de l'élément sélectionné */
  pParent = TrouverParcours(iGaucheVars->zonePrincipale, LireZoneX(tmp)-1, LireZoneY(tmp)-1);
  /* wprintf(L"Trace parcours élément Parent\n");
     traceParcours(pParent); */
  ZoneParent = LireZoneParcours(iGaucheVars->zonePrincipale, pParent);
  elementParent = LireElementParcours(iGaucheVars->bullePrincipale, pParent);
  //  bulleParent = &elementParent->bulle;
  EffacerParcours(pParent);
  /* printf("DEBUG : idParent : %d\n", elementParent->bulle.id); */
  /* wprintf(L"DEBUG : encapsuler, fin des calculs des éléments sélectionnés et parent\n"); */


  /* on l'insère dans la bulle de destination qui est la bulle parent de l'élément sélectioné */
  AddElement(elementParent, nouvelleBulle);
  /* printf("DEBUG : idbullePrincipale : %d\n", iGaucheVars->bullePrincipale->bulle.id); */
  /* printf("DEBUG : idParent : %d, idNouvelleBulle :%d\n", elementParent->bulle.id, nouvelleBulle->bulle.id); */

  /* et on l'insère dans la zone conteneur */
  AjouterSousZones(ZoneParent, z_encapsulation);

  /* copie de la sélection dans la nouvelleBulle avec copie des id */
  CopyElement(nouvelleBulle, z_encapsulation, elementSelectionne, iGaucheVarsTmp, 0);
  //iGaucheVars->zoneSelectionnee = tmp;
  /* suppression de la sélection */
  //deleteRec(tmp);
  tmp = tmp->next;
  }
  deleteRec2(pWidget, data);
   //enregistrerTmp(); /* pour l'historique */
  //iGaucheVars->head = NULL;
  iGaucheVars->zoneSelectionnee = iGaucheVars->zonePrincipale;
  iGaucheVars->zoneSelectionnee = NULL;
  iGaucheVars->head = iGaucheVars->zonePrincipale;
  iGaucheVars->head->next = NULL;
  Rearanger(iGaucheVars->zonePrincipale);

  return;
}

#endif

/*Fin*/

/*Selection Simple*/
#if 0
/*! Encapsuler la sélection dans une bulle
 *
 */
void encapsuler(GtkWidget* pWidget, gpointer data)
{
  /*printf("...\n");
    printf("DEBUG : appel encapsuler\n"); */
  parcours *p, *pParent;
  pWidget = NULL;  // inutilisé
  interfaceGaucheVars* iGaucheVars = (interfaceGaucheVars*)data;
  zone * ZoneSelectionnee, *ZoneParent, *z_encapsulation;
  Element* elementSelectionne, * elementParent, *nouvelleBulle;

  if(iGaucheVars->zoneSelectionnee == iGaucheVars->zonePrincipale)
    {
      /* printf("DEBUG : encapsuler, zoneSelectionnee = zonePrincipale, encapsulation impossible\n"); */
      return;
    }

  /* Voilà l'élément sélectionné qu'on va encapsuler */
  p = TrouverParcours(iGaucheVars->zonePrincipale, iGaucheVars->mousePosClic_left_x, iGaucheVars->mousePosClic_left_y);
  /* wprintf(L"Trace parcours élément à déplacer\n");
     traceParcours(p); */
  ZoneSelectionnee = LireZoneParcours(iGaucheVars->zonePrincipale, p);
  iGaucheVars->zoneSelectionnee = ZoneSelectionnee;
  elementSelectionne = LireElementParcours(iGaucheVars->bullePrincipale, p);
  EffacerParcours(p);
  /* printf("DEBUG : idSelectionB : %d\n", elementSelectionne->bulle.id); */
  /*  printf("DEBUG : idSelectionT : %d\n", elementSelectionne->thread.id); */

  /* Voilà l'élément parent de l'élément sélectionné */
  pParent = TrouverParcours(iGaucheVars->zonePrincipale, LireZoneX(iGaucheVars->zoneSelectionnee)-1, LireZoneY(iGaucheVars->zoneSelectionnee)-1);
  /* wprintf(L"Trace parcours élément Parent\n");
     traceParcours(pParent); */
  ZoneParent = LireZoneParcours(iGaucheVars->zonePrincipale, pParent);
  elementParent = LireElementParcours(iGaucheVars->bullePrincipale, pParent);
  //  bulleParent = &elementParent->bulle;
  EffacerParcours(pParent);
  /* printf("DEBUG : idParent : %d\n", elementParent->bulle.id); */
  /* wprintf(L"DEBUG : encapsuler, fin des calculs des éléments sélectionnés et parent\n"); */

  /* Création de la bulle d'accueil avec pour paramètre la priorité par défaut d'une bulle et une nouvelle id */
  nouvelleBulle = CreateBulle(iGaucheVars->defprioritebulle, SetId(iGaucheVars));

  /* on l'insère dans la bulle de destination qui est la bulle parent de l'élément sélectioné */
  AddElement(elementParent, nouvelleBulle);
  /* printf("DEBUG : idbullePrincipale : %d\n", iGaucheVars->bullePrincipale->bulle.id); */
  /* printf("DEBUG : idParent : %d, idNouvelleBulle :%d\n", elementParent->bulle.id, nouvelleBulle->bulle.id); */

  /* on crée la zone qui va contenir cette nouvelle bulle */
  z_encapsulation = CreerZone(0,0,LARGEUR_B, HAUTEUR_B);
  /* et on l'insère dans la zone conteneur */
  AjouterSousZones(ZoneParent, z_encapsulation);

  /* copie de la sélection dans la nouvelleBulle avec copie des id */
  CopyElement(nouvelleBulle, z_encapsulation, elementSelectionne, iGaucheVars, 0);

  /* suppression de la sélection */
  deleteRec2(pWidget, data);
  iGaucheVars->zoneSelectionnee = NULL;

  Rearanger(iGaucheVars->zonePrincipale);
  enregistrerTmp(); /* pour l'historique */
  return;
}
#endif
/*Fin */

void AddBulle(GtkWidget* widget, DataAddBulle* data)
{
  interfaceGaucheVars* iGaucheVars = (interfaceGaucheVars*)(data->iGaucheVars);

  parcours* p;
  p = TrouverParcours(iGaucheVars->zonePrincipale, LireZoneX(iGaucheVars->zoneSelectionnee) + 1, LireZoneY(iGaucheVars->zoneSelectionnee) + 1);
  Element* bulleParent = LireElementParcours(iGaucheVars->bullePrincipale, p);

  if (data->popup != NULL)
    {
      if(GetTypeElement(bulleParent) == BULLE)
	{
	  if(LireZoneParcours(iGaucheVars->zonePrincipale, p) != iGaucheVars->zoneSelectionnee)
            printf("erreur dans AddBulle!!!!!!!\n");

	  if (bulleParent != NULL)
	    {
	      EffacerParcours(p);
	      Element *nouvelleBulle;
	      gint val;
	      if (widget == NULL)
		return;
	      /* Recuperation de la valeur de la scrollbar */
	      val = gtk_range_get_value(GTK_RANGE((DataAddBulle*)data->pScrollbar));
	      nouvelleBulle = CreateBulle(val,SetId(iGaucheVars));
	      AddElement(bulleParent, nouvelleBulle);
	      AjouterSousZones(iGaucheVars->zoneSelectionnee, CreerZone(0,0,LARGEUR_B, HAUTEUR_B));

	      Rearanger(iGaucheVars->zonePrincipale);
	    }
	  gtk_widget_destroy((GtkWidget*)(DataAddThread*)data->popup);
	  free(data);
	}
    }
  /* data->popup == NULL*/
  else
    {
      if(GetTypeElement(bulleParent) == BULLE)
	{
	  if(LireZoneParcours(iGaucheVars->zonePrincipale, p) != iGaucheVars->zoneSelectionnee)
            printf("erreur dans AddBulle!!!!!!!\n");
	  if (bulleParent != NULL)
	    {
	      EffacerParcours(p);
	      Element *nouvelleBulle;
	      nouvelleBulle = CreateBulle(gtk_range_get_value(GTK_RANGE((DataAddBulle*)iGaucheVars->prioritebulle)), SetId(iGaucheVars));
	      AddElement(bulleParent, nouvelleBulle);
	      AjouterSousZones(iGaucheVars->zoneSelectionnee, CreerZone(0,0,LARGEUR_B, HAUTEUR_B));
	      Rearanger(iGaucheVars->zonePrincipale);
	    }
	  free(data);
	}
    }
  return;
}



/* fonction qui ne fait pour l'instant que récupérer les données
 * necéssaires à la création d'un thread et qui ferme la fenêtre
 */
void AddThread(GtkWidget* widget, DataAddThread* data)
{
  char *nom = malloc(50*sizeof(char));

  interfaceGaucheVars* iGaucheVars = (interfaceGaucheVars*)(data->iGaucheVars);

  Element* bulleParent;
  parcours* p;
  p = TrouverParcours(iGaucheVars->zonePrincipale, LireZoneX(iGaucheVars->zoneSelectionnee) + 1, LireZoneY(iGaucheVars->zoneSelectionnee)+1);
  bulleParent = LireElementParcours(iGaucheVars->bullePrincipale, p);
  if(GetTypeElement(bulleParent) == BULLE)
    {
      if(LireZoneParcours(iGaucheVars->zonePrincipale, p) != iGaucheVars->zoneSelectionnee)
	printf("erreur dans AddThread!!!!!!!--\n");
      EffacerParcours(p);
      Element *nouveauThread;
      /* Recuperation du nom du thread */
      gint val1;
      gint val2;
      const gchar *text1= gtk_entry_get_text(GTK_ENTRY((DataAddThread*)data->nomEntry));
      strcpy(nom, text1);

      /* Recuperation de la valeur des scrollbars */
      val1 = gtk_range_get_value(GTK_RANGE((DataAddThread*)data->prioriteScrollbar));
      val2 = gtk_range_get_value(GTK_RANGE((DataAddThread*)data->chargeScrollbar));
      /* printf("DEBUG : nom : %s\n",nom);
	 printf("DEBUG : priorite : %d\n",(int)val1);
	 printf("DEBUG : charge : %d\n",(int)val2);
	 printf("DEBUG : identifiant : %d\n",data->idThread); */
      nouveauThread = CreateThread(val1, data->idThread, nom, val2);
      AddElement(bulleParent, nouveauThread);
      AjouterSousZones(iGaucheVars->zoneSelectionnee, CreerZone(0,0,LARGEUR_T, HAUTEUR_T));
      Rearanger(iGaucheVars->zonePrincipale);
      free(data);
    }
  return;
}



/* fonction qui mets à jour le label pour qu'il affiche la valeur de
 * la scrollbar correspondante
 */
void OnScrollbarChange(GtkWidget *pWidget, gpointer data)
{
  gchar* sLabel;
  gint iValue;

  /* Recuperation de la valeur de la scrollbar */
  iValue = gtk_range_get_value(GTK_RANGE(pWidget));
  /* Creation du nouveau label */
  sLabel = g_strdup_printf("%d", iValue);
  /* Modification du label */
  gtk_label_set_text(GTK_LABEL(data), sLabel);
  /* Liberation memoire */
  g_free(sLabel);
  /* avoir si on a reelement besoin */
  enregistrerTmp();
}


int menuitem_rep(GtkWidget *widget,GdkEvent *event) {
  if (event->type == GDK_BUTTON_PRESS) {
    GdkEventButton *bevent = (GdkEventButton *) event;

    if (bevent->button == 3) {
      gtk_menu_popup (GTK_MENU (widget), NULL, NULL, NULL, NULL, bevent->button, bevent->time);
      return TRUE;
    }
  }

  return FALSE;
}

void imprimer_rep (gchar *string) {
  g_print ("%s\n",string);
}

/*********************************************************************/
/*! Copie le 'contenu' dans la liste du 'conteneur' en tenant compte
 * de ce que contient déjà le 'contenu' en recréant chaque fils en profondeur.
 * \param bulleAccueil l'élément qui va recevoir l'élément à copier, en clair c'est une la bulle d'accueil
 * \param z_conteneur la zone d'accueil
 * \param contenu l'élément qui va être copié
 * \param iGaucheVars les variables de l'interface gauche
 * \param newid nouvelle id générée si 1, id copiée sinon
 */
void CopyElement(Element* bulleAccueil, zone* z_conteneur, Element* contenu, interfaceGaucheVars * iGaucheVars, int newid)
{
  ListeElement* liste;
  ListeElement* liste2=NULL;
  Element * nouvelleBulle, * nouveauThread;
  zone * zoneprinc, * z_copie; /* la zone qui va contenir l'élément copié */

  zoneprinc = iGaucheVars->zonePrincipale;

  /* On vérifie quand même que la bulle d'accueil est bien une bulle, on n'est jamais sûr. */
  if(GetTypeElement(bulleAccueil) == BULLE)
    {
      /* On sépare le traitement d'une bulle ou d'un thread en 2 cas distincts. */
      if (GetTypeElement(contenu) == BULLE)
	{
	  /* on récupère la liste de la bulle pour copier ses fils, s'il en a. */
	  liste = contenu->bulle.liste;

	  /* on crée la bulle avec la même paramètre que l'originale */
          if (newid == 1)
            nouvelleBulle = CreateBulle(GetPrioriteBulle(contenu), SetId(iGaucheVars));
          else
            nouvelleBulle = CreateBulle(GetPrioriteBulle(contenu), GetId(contenu));
	  /* on l'insère dans la bulle de destination */
	  AddElement(bulleAccueil, nouvelleBulle);
	  /* on crée la zone qui va contenir cette nouvelle bulle */
	  z_copie = CreerZone(0,0,LARGEUR_B, HAUTEUR_B);
	  /* et on l'insère dans la zone conteneur */
	  AjouterSousZones(z_conteneur, z_copie);
	  Rearanger(zoneprinc);
	  /* printf("DEBUG : CopyElement, createBulle\n"); */

	  /* ici on copie les fils en parcourant la liste de contenu, récursivement */
	  while(liste != NULL)
	    {
	      /* wprintf(L"DEBUG : CopyElement, Copie récursive\n"); */
	      CopyElement(nouvelleBulle, z_copie, liste->element, iGaucheVars, newid);
	      liste2 = liste;
	      liste = liste->suivant;
	    }
	}

      else if (GetTypeElement(contenu) == THREAD)
	{
          /* on génére un nouvel identifiant si newid == 1 */
          if (newid == 1)
            nouveauThread = CreateThread(GetPrioriteThread(contenu), SetId(iGaucheVars), GetNom(contenu), GetCharge(contenu));
          else
            nouveauThread = CreateThread(GetPrioriteThread(contenu), GetId(contenu), GetNom(contenu), GetCharge(contenu));
	  AddElement(bulleAccueil, nouveauThread);
	  AjouterSousZones(z_conteneur, CreerZone(0,0,LARGEUR_T, HAUTEUR_T));
	}
      /* printf("DEBUG : CopyElement, contenu->bulle.taille : %d\n", contenu->bulle.taille); */
      /* printf("DEBUG : CopyElement, conteneur->bulle.taille : %d\n",bulleAccueil->bulle.taille); */
      return;
    }
  /* donc dans le cas où la bulle d'accueil n'est pas une bulle, on prévient quand même l'utilisateur/développeur. */
  else
    {
      printf("Erreur dans la fonction CopyElement\n");
      return;
    }
}


