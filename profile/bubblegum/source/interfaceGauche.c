#include <gdk/gdkkeysyms.h>
#include "interfaceGauche.h"
#include "load.h"
#include "mainwindow.h"


interfaceGaucheVars* interfaceGauche()
{
   GtkWidget *b_addBulle = open_ico("ico/addBulle.png");
   GtkWidget *b_addThread = open_ico("ico/addThread.png");
   GtkWidget *b_delete = open_ico("ico/delete.png");
   GtkWidget *b_deleteRec = open_ico("ico/deleteRec.png");
   GtkWidget *toolbar;
   GtkWidget *toolbar2;
   GtkWidget *toolbar3;
   
   GtkWidget *toolbar_item;
   GtkWidget *menu;
   GtkWidget *item1;
   GtkWidget *item2;
   GtkWidget *item3;
   GtkWidget *ev_box;
   interfaceGaucheVars *iGaucheVars;

   iGaucheVars = malloc(sizeof(interfaceGaucheVars));

   iGaucheVars->interfaceGauche = gtk_vbox_new(FALSE, 0);
   iGaucheVars->mode_auto = MODE_AUTO_ON;

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
                                             NULL, NULL, "Nouvelle bulle", NULL, b_addBulle, G_CALLBACK(addBulleAutoOnOff), iGaucheVars);
   toolbar_item = gtk_toolbar_append_element(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_CHILD_BUTTON,
                                             NULL, NULL, "Nouveau thread", NULL, b_addThread, G_CALLBACK(addThreadAutoOnOff), iGaucheVars);
   toolbar_item = gtk_toolbar_append_element(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_CHILD_BUTTON,
                                             NULL, NULL, "Supprimer l'element recursivement", NULL, b_deleteRec, G_CALLBACK(deleteRec), iGaucheVars);
   toolbar_item = gtk_toolbar_append_element(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_CHILD_BUTTON,
                                             NULL, NULL, "Sauvegarder les nouveaux paramètres", NULL, b_delete, G_CALLBACK(SaveNewData), iGaucheVars);

   /* Ajout des étiquettes charge-priorité-nom en vertical */
   
     toolbar2 = gtk_toolbar_new();
     gtk_toolbar_set_orientation(GTK_TOOLBAR(toolbar2),GTK_ORIENTATION_VERTICAL);
     gtk_toolbar_append_widget(GTK_TOOLBAR(toolbar2),gtk_label_new(""),NULL,NULL);
     gtk_toolbar_append_widget(GTK_TOOLBAR(toolbar2),gtk_label_new("Charge"),NULL,NULL);
     gtk_toolbar_append_widget(GTK_TOOLBAR(toolbar2),gtk_label_new(""),NULL,NULL);
     gtk_toolbar_append_widget(GTK_TOOLBAR(toolbar2),gtk_label_new("Priorité"),NULL,NULL);
     gtk_toolbar_append_widget(GTK_TOOLBAR(toolbar2),gtk_label_new(""),NULL,NULL);
     gtk_toolbar_append_widget(GTK_TOOLBAR(toolbar2),gtk_label_new("Nom"),NULL,NULL);

     gtk_toolbar_append_widget(GTK_TOOLBAR(toolbar),toolbar2,NULL,NULL);

   /* Ajout des ascenceurs plus la zone texte pour le nom */

     toolbar3 = gtk_toolbar_new();
     gtk_toolbar_set_orientation(GTK_TOOLBAR(toolbar3),GTK_ORIENTATION_VERTICAL);
     iGaucheVars->charge=gtk_hscale_new_with_range(0,100,10);
     gtk_range_set_value(GTK_RANGE(iGaucheVars->charge), DEF_CHARGE);
     iGaucheVars->priorite=gtk_hscale_new_with_range(0,10,1);
     gtk_range_set_value(GTK_RANGE(iGaucheVars->priorite), DEF_PRIO);
     iGaucheVars->nom=gtk_entry_new();
     gtk_toolbar_append_widget(GTK_TOOLBAR(toolbar3),iGaucheVars->charge,NULL,NULL);
     gtk_toolbar_append_widget(GTK_TOOLBAR(toolbar3),iGaucheVars->priorite,NULL,NULL);
     gtk_toolbar_append_widget(GTK_TOOLBAR(toolbar3),iGaucheVars->nom,NULL,NULL);

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
      iGaucheVars->idBulleMax =iGaucheVars->idBulleMax + 1;
      dataAddBulle->idBulle = iGaucheVars->idBulleMax;
      dataAddBulle->iGaucheVars = iGaucheVars;
      AddBulle(NULL, dataAddBulle);
   }

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
      iGaucheVars->idThreadMax =iGaucheVars->idThreadMax + 1;
      dataAddThread->idThread = iGaucheVars->idThreadMax;
      dataAddThread->idEntry = NULL;
      dataAddThread->popup = NULL;
      dataAddThread->iGaucheVars = iGaucheVars;
      AddThread(NULL, dataAddThread);
 

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
   GtkWidget *Adjust = (GtkWidget*)gtk_adjustment_new(20, 0, 100, 1, 10, 1);
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

   return;
}

void SaveNewData()
{
  if(iGaucheVars->ThreadSelect != NULL)
    {
  SetCharge(iGaucheVars->ThreadSelect,gtk_range_get_value(GTK_RANGE((DataAddThread*)iGaucheVars->charge)) );
  SetPrioriteThread(iGaucheVars->ThreadSelect,gtk_range_get_value(GTK_RANGE((DataAddThread*)iGaucheVars->priorite)) );
  SetNom(iGaucheVars->ThreadSelect,gtk_entry_get_text(GTK_ENTRY((DataAddThread*)iGaucheVars->nom)) );
    }
  return;
}

void deleteRec(GtkWidget* pWidget, gpointer data)
{
   //interfaceGaucheVars* iGaucheVars = (interfaceGaucheVars*)(data);
 
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


void deleteRec2(GtkWidget* pWidget, gpointer data)
{
   deleteRec(NULL,(gpointer)pWidget);

   return;
}


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
   
	    nouvelleBulle = CreateBulle(val, data->idBulle);

            AddElement(bulleParent, nouvelleBulle);

            AjouterSousZones(iGaucheVars->zoneSelectionnee, CreerZone(0,0,LARGEUR_B, HAUTEUR_B));

            Rearanger(iGaucheVars->zonePrincipale);

         }

         gtk_widget_destroy((GtkWidget*)(DataAddThread*)data->popup);

         free(data);
      }
   }
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

	    nouvelleBulle = CreateBulle(10, data->idBulle);

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
 * necéssaires à la création d'un thread et qui ferme la fenêtre */
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

	 printf("DEBUG : nom : %s\n",nom);
	 printf("DEBUG : priorite : %d\n",(int)val1);
	 printf("DEBUG : charge : %d\n",(int)val2);
	 printf("DEBUG : identifiant : %d\n",data->idThread);
         nouveauThread = CreateThread(val1, data->idThread, nom, val2);
   
         AddElement(bulleParent, nouveauThread);

         AjouterSousZones(iGaucheVars->zoneSelectionnee, CreerZone(0,0,LARGEUR_T, HAUTEUR_T));

         Rearanger(iGaucheVars->zonePrincipale);

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

   
