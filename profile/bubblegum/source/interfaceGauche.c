#include "interfaceGauche.h"
#include "load.h"


interfaceGaucheVars* interfaceGauche()
{
   GtkWidget *b_addBulle = open_ico("ico/addBulle.png");
   GtkWidget *b_addThread = open_ico("ico/addThread.png");
   GtkWidget *b_delete = open_ico("ico/delete.png");
   GtkWidget *b_deleteRec = open_ico("ico/deleteRec.png");
   GtkWidget *toolbar;
   GtkWidget *toolbar_item;
   GtkWidget *menu;
   GtkWidget *item1;
   GtkWidget *item2;
   GtkWidget *item3;
   GtkWidget *ev_box;
   interfaceGaucheVars *iGaucheVars;

   iGaucheVars = malloc(sizeof(interfaceGaucheVars));

   iGaucheVars->interfaceGauche = gtk_vbox_new(FALSE, 0);

   make_left_drawable_zone(iGaucheVars);

   /*menu*/
   menu = gtk_menu_new ();
   item1 = gtk_menu_item_new_with_label ("Option 1");
   gtk_menu_shell_append (GTK_MENU_SHELL(menu), item1);
   g_signal_connect_swapped (G_OBJECT(item1),"activate",G_CALLBACK(imprimer_rep), (gpointer) "L'option 1 a ete clique");
   
   item2 = gtk_menu_item_new_with_label ("Option 2");
   gtk_menu_shell_append (GTK_MENU_SHELL(menu), item2);
   g_signal_connect_swapped (G_OBJECT(item2),"activate",G_CALLBACK(imprimer_rep), (gpointer) "L'option 2 a ete clique");
   
   item3 = gtk_menu_item_new_with_label ("Option 3");
   gtk_menu_shell_append (GTK_MENU_SHELL(menu), item3);
   g_signal_connect_swapped (G_OBJECT(item3),"activate",G_CALLBACK(imprimer_rep), (gpointer) "L'option 3 a ete clique");
   
   gtk_widget_show_all (menu);



   /*event box*/
   ev_box = gtk_event_box_new ();
/*    gtk_container_add (GTK_CONTAINER(drawzone), ev_box); */
   g_signal_connect_swapped (G_OBJECT(ev_box), "event", G_CALLBACK(menuitem_rep), G_OBJECT(menu));
   
   toolbar = gtk_toolbar_new();

   /* ajout de boutons dans la barre d'outil */
   toolbar_item = gtk_toolbar_append_element(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_CHILD_BUTTON,
                                             NULL, NULL, "Nouvelle bulle", NULL, b_addBulle, G_CALLBACK(addBulle), iGaucheVars);
   toolbar_item = gtk_toolbar_append_element(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_CHILD_BUTTON,
                                             NULL, NULL, "Nouveau thread", NULL, b_addThread, G_CALLBACK(addThread), iGaucheVars);
   toolbar_item = gtk_toolbar_append_element(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_CHILD_BUTTON,
                                             NULL, NULL, "Supprimer l'element", NULL, b_delete, G_CALLBACK(delete), iGaucheVars);
   toolbar_item = gtk_toolbar_append_element(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_CHILD_BUTTON,
                                             NULL, NULL, "Supprimer l'element recursivement", NULL, b_deleteRec, G_CALLBACK(deleteRec), iGaucheVars);


   gtk_box_pack_end(GTK_BOX(iGaucheVars->interfaceGauche), toolbar, FALSE, FALSE, 0);


   return iGaucheVars;
}




/* fonction qui affiche une fenêtre popup pour rentrer les attributs
 * d'une bulle */
void addBulle(GtkWidget *pWidget, gpointer data)
{

   GtkWidget *popup = gtk_window_new(GTK_WINDOW_TOPLEVEL);
   GtkWidget *pVBox = gtk_vbox_new(FALSE, 0);
   GtkWidget *pFrame = gtk_frame_new("Priorité");
   GtkWidget *Adjust = (GtkWidget*)gtk_adjustment_new(0, 0, 100, 1, 10, 1);
   GtkWidget *pScrollbar = gtk_hscrollbar_new(GTK_ADJUSTMENT(Adjust));
   GtkWidget *pLabel = gtk_label_new("0");
   GtkWidget *prioriteVBox = gtk_vbox_new(FALSE, 0);
   GtkWidget *boutonOk = gtk_button_new_with_mnemonic("OK");

   /* création de la structure servant à stocker les donnees pour les
    * CallBack */
   DataAddBulle* dataAddBulle = malloc(sizeof (DataAddBulle));
   dataAddBulle->popup = popup;
   dataAddBulle->pScrollbar = pScrollbar;
   dataAddBulle->iGaucheVars = (interfaceGaucheVars*) data;

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
                       G_CALLBACK(traceAddBulle), dataAddBulle);

      gtk_container_add(GTK_CONTAINER(popup), pVBox);

      gtk_box_pack_start(GTK_BOX(pVBox), pFrame, FALSE, FALSE, 20);
      gtk_box_pack_start(GTK_BOX(pVBox), boutonOk, TRUE, FALSE, 20);

      /* Affichage de la popup */
      gtk_widget_show_all(popup);
   }

   return;
}

/* fonction qui affiche une fenêtre popup pour rentrer les attributs
 * d'un thread */
void addThread(GtkWidget *pWidget, gpointer data)
{
   GtkWidget *popup = gtk_window_new(GTK_WINDOW_TOPLEVEL);
   GtkWidget *pVBox = gtk_vbox_new(FALSE, 0);

   GtkWidget *nomFrame = gtk_frame_new("Nom");
   GtkWidget *nomEntry = gtk_entry_new();

   GtkWidget *idFrame = gtk_frame_new("Identifiant");
   GtkWidget *idEntry = gtk_entry_new();

   GtkWidget *prioriteFrame = gtk_frame_new("Priorité");
   GtkWidget *prioriteLabel = gtk_label_new("0");
   GtkWidget *prioriteVBox = gtk_vbox_new(FALSE, 0);
   GtkWidget *prioriteAdjust = (GtkWidget*)gtk_adjustment_new(0, 0, 100, 1, 10, 1);
   GtkWidget *prioriteScrollbar = gtk_hscrollbar_new(GTK_ADJUSTMENT(prioriteAdjust));

   GtkWidget *chargeFrame = gtk_frame_new("charge");
   GtkWidget *chargeLabel = gtk_label_new("0");
   GtkWidget *chargeVBox = gtk_vbox_new(FALSE, 0);
   GtkWidget *chargeAdjust = (GtkWidget*)gtk_adjustment_new(0, 0, 100, 1, 10, 1);
   GtkWidget *chargeScrollbar = gtk_hscrollbar_new(GTK_ADJUSTMENT(chargeAdjust));

   GtkWidget *boutonOk = gtk_button_new_with_mnemonic("OK");

   /* création de la structure servant à stocker les donnees pour les
    * CallBack */
   DataAddThread* dataAddThread = malloc( sizeof (DataAddThread) );
   dataAddThread->prioriteScrollbar = prioriteScrollbar;
   dataAddThread->chargeScrollbar = chargeScrollbar;
   dataAddThread->nomEntry = nomEntry;
   dataAddThread->idEntry = idEntry;
   dataAddThread->popup = popup;
   dataAddThread->iGaucheVars = (interfaceGaucheVars*) data;

   Element* bulleParent;
   parcours* p;
   p = TrouverParcours(dataAddThread->iGaucheVars->zonePrincipale, LireZoneX(dataAddThread->iGaucheVars->zoneSelectionnee) + 1, LireZoneY(dataAddThread->iGaucheVars->zoneSelectionnee)+1);
   bulleParent = LireElementParcours(dataAddThread->iGaucheVars->bullePrincipale, p);

   if(GetTypeElement(bulleParent) == BULLE)
   {


      /* creation de la popup pour la création d'une bulle */
      gtk_window_set_title(GTK_WINDOW(popup), g_locale_to_utf8("Nouveau Thread",
                                                               -1, NULL, NULL, NULL));
      gtk_window_set_default_size(GTK_WINDOW(popup), 300, 50);
      gtk_window_set_position(GTK_WINDOW(popup), GTK_WIN_POS_CENTER);


      /* saisie du nom */
      gtk_container_add(GTK_CONTAINER(nomFrame), nomEntry);   


      /* saisie de l'identifiant */
      gtk_container_add(GTK_CONTAINER(idFrame), idEntry);   


      /* saisie de la priorite (avec un ascenseur) */
      gtk_container_add(GTK_CONTAINER(prioriteFrame), prioriteVBox);
      gtk_box_pack_start(GTK_BOX(prioriteVBox), prioriteLabel, FALSE, FALSE, 10);
      gtk_box_pack_start(GTK_BOX(prioriteVBox), prioriteScrollbar, TRUE, TRUE, 10);
      /* Connexion du signal pour modification de l affichage */
      g_signal_connect(G_OBJECT(prioriteScrollbar), "value-changed",
                       G_CALLBACK(OnScrollbarChange), (GtkWidget*)prioriteLabel);

      /* saisie de la priorite (avec un ascenseur) */
      gtk_container_add(GTK_CONTAINER(chargeFrame), chargeVBox);
      gtk_box_pack_start(GTK_BOX(chargeVBox), chargeLabel, FALSE, FALSE, 10);
      gtk_box_pack_start(GTK_BOX(chargeVBox), chargeScrollbar, TRUE, TRUE, 10);
      /* Connexion du signal pour modification de l affichage */
      g_signal_connect(G_OBJECT(chargeScrollbar), "value-changed",
                       G_CALLBACK(OnScrollbarChange), (GtkWidget*)chargeLabel);


      g_signal_connect(G_OBJECT(boutonOk), "clicked",
                       G_CALLBACK(traceAddThread), dataAddThread);

      /* ajout de tout les éléments dans la popup */
      gtk_box_pack_start(GTK_BOX(pVBox), nomFrame, FALSE, FALSE, 20);
      gtk_box_pack_start(GTK_BOX(pVBox), idFrame, FALSE, FALSE, 20);
      gtk_box_pack_start(GTK_BOX(pVBox), prioriteFrame, FALSE, FALSE, 20);
      gtk_box_pack_start(GTK_BOX(pVBox), chargeFrame, FALSE, FALSE, 20);
      gtk_box_pack_start(GTK_BOX(pVBox), boutonOk, TRUE, FALSE, 20);

      gtk_container_add(GTK_CONTAINER(popup), pVBox);

      /* Affichage de la popup */
      gtk_widget_show_all(popup);

   }
   return;
}


void delete(GtkWidget* pWidget, gpointer data)
{
   interfaceGaucheVars* iGaucheVars = (interfaceGaucheVars*)(data);

   if(iGaucheVars->zonePrincipale != iGaucheVars->zoneSelectionnee)
   {

   }   
/*    void RemoveElement(Element* conteneur, int position); */
   
/*    void RemoveElementOnCascade(Element* conteneur, int position); */

   return;
}

void deleteRec(GtkWidget* pWidget, gpointer data)
{
   interfaceGaucheVars* iGaucheVars = (interfaceGaucheVars*)(data);
 
   if(iGaucheVars->zonePrincipale != iGaucheVars->zoneSelectionnee)
   {
      Effacer(iGaucheVars->bullePrincipale, iGaucheVars->zonePrincipale,
              LireZoneX(iGaucheVars->zoneSelectionnee) + 1,
              LireZoneY(iGaucheVars->zoneSelectionnee) + 1);
   }

   Rearanger(iGaucheVars->zonePrincipale);

   iGaucheVars->zoneSelectionnee = iGaucheVars->zonePrincipale;

   return;
}



void traceAddBulle(GtkWidget* widget, DataAddBulle* data)
{
   interfaceGaucheVars* iGaucheVars = (interfaceGaucheVars*)(data->iGaucheVars);

   parcours* p;
   p = TrouverParcours(iGaucheVars->zonePrincipale, LireZoneX(iGaucheVars->zoneSelectionnee) + 1, LireZoneY(iGaucheVars->zoneSelectionnee) + 1);
   Element* bulleParent = LireElementParcours(iGaucheVars->bullePrincipale, p);

   if(GetTypeElement(bulleParent) == BULLE)
   {

      if(LireZoneParcours(iGaucheVars->zonePrincipale, p) != iGaucheVars->zoneSelectionnee)
         printf("erreur dans TraceAddBulle!!!!!!!\n");


/*    printf("traceAddBulle p->taille%d %p\n",LireParcoursTaille(p), iGaucheVars->zoneSelectionnee); */

      if (bulleParent != NULL)
      {
         EffacerParcours(p);
   
         Element *nouvelleBulle;
      
         gint val;
   
         if (widget == NULL)
            return;
   
         /* Recuperation de la valeur de la scrollbar */
         val = gtk_range_get_value(GTK_RANGE((DataAddBulle*)data->pScrollbar));
   
         nouvelleBulle = CreateBulle(val);

         AddElement(bulleParent, nouvelleBulle);

         AjouterSousZones(iGaucheVars->zoneSelectionnee, CreerZone(0,0,LARGEUR_B, HAUTEUR_B));

         Rearanger(iGaucheVars->zonePrincipale);

      }

      gtk_widget_destroy((GtkWidget*)(DataAddThread*)data->popup);

      free(data);
   }

   return;
}



/* fonction qui ne fait pour l'instant que récupérer les données
 * necéssaires à la création d'un thread et qui ferme la fenêtre */
void traceAddThread(GtkWidget* widget, DataAddThread* data)
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
         printf("erreur dans traceAddThread!!!!!!!--\n");

      EffacerParcours(p);

      Element *nouveauThread;

      if (widget == NULL)
         return;

      gint val1;
      gint val2;
      const gchar *text1= gtk_entry_get_text(GTK_ENTRY((DataAddThread*)data->nomEntry));

      strcpy(nom, text1);

      /* Recuperation de la valeur de la scrollbar */
      val1 = gtk_range_get_value(GTK_RANGE((DataAddThread*)data->prioriteScrollbar));
      val2 = gtk_range_get_value(GTK_RANGE((DataAddThread*)data->chargeScrollbar));

      nouveauThread = CreateThread(0, 0, nom, 0);
   
      AddElement(bulleParent, nouveauThread);

      AjouterSousZones(iGaucheVars->zoneSelectionnee, CreerZone(0,0,LARGEUR_T, HAUTEUR_T));

      Rearanger(iGaucheVars->zonePrincipale);

      gtk_widget_destroy((GtkWidget*)(DataAddThread*)data->popup);

      free(data);

   }
   return;
}



/* fonction qui mets à jour le label pour qu'il affiche la valeur de
 * la scrollbar correspondante */
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

