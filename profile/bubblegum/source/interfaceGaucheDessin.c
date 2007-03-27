#include "interfaceGaucheDessin.h"
#include "interfaceGauche.h"
#include "bulle.h"


#if 0
int i=0;
int j=0;
int k=0;
int POZX=0;
int POZY=0;

int selectionON = FALSE;
#endif

void couleurContour(zone* zoneADessiner, zone* zoneSelectionnee)
{
   if (zoneADessiner == zoneSelectionnee)
      glColor3f(0.1,0.8,0.1);
   else
      glColor3f(0.3,0.6,0.7);

   return;
}


void couleurInterieur(zone* zoneADessiner, zone* zoneSelectionnee)
{
   if (zoneADessiner == zoneSelectionnee)
      glColor3f(0.5,1,0.5);
   else
      glColor3f(0.5,1,1);

   return;
}


void DessinerTout(interfaceGaucheVars *iGaucheVars)
{

   Clear();
   if(iGaucheVars->zonePrincipale != NULL) 
      Dessiner(iGaucheVars, iGaucheVars->zonePrincipale);
}


void Dessiner(interfaceGaucheVars *iGaucheVars, zone * zonePrincipale)
{
   int i;

   zone* sousZone;

   TracerZone(iGaucheVars, zonePrincipale);

   /* et on fait pareil recursivement sur les sous-zones */
   for(i = 1;
       (sousZone = LireSousZones(zonePrincipale, i)) != NULL;
       i++)
      Dessiner(iGaucheVars, sousZone);
   
   return;
}


void Clear()
{
   glClearColor(1,1,1,1);
   glClear(GL_COLOR_BUFFER_BIT);
}

void TracerZone(interfaceGaucheVars *iGaucheVars, zone* zoneADessiner)
{
   double i;
   Element * element;
   parcours *p;
   p = TrouverParcours(iGaucheVars->zonePrincipale, LireZoneX(zoneADessiner) + 1, LireZoneY(zoneADessiner) + 1);
   if(LireZoneParcours(iGaucheVars->zonePrincipale, p) != zoneADessiner)
     {
       printf("Zone Principale posX : %d, posY : %d, largeur : %d, hauteur %d\n", 
	      iGaucheVars->zonePrincipale->posX, iGaucheVars->zonePrincipale->posY, 
	      iGaucheVars->zonePrincipale->largeur, iGaucheVars->zonePrincipale->hauteur); 
       printf("Zone à dessiner posX : %d, posY : %d, largeur : %d, hauteur %d\n", 
	      zoneADessiner->posX, zoneADessiner->posY, 
	      zoneADessiner->largeur, zoneADessiner->hauteur); 
       printf("erreur dans Tracer zone!!!!!!!\n");
     }

   element = LireElementParcours(iGaucheVars->bullePrincipale, p);
   EffacerParcours(p);


   if(400 < iGaucheVars->echelle * LireZoneHauteur(iGaucheVars->zonePrincipale))
   {
      glScalef(0.95,0.95,0.95);
      iGaucheVars->echelle *= 0.95;
   }

   if(300 > iGaucheVars->echelle * LireZoneHauteur(iGaucheVars->zonePrincipale))
   {
      glScalef(1.05,1.05,1.05);
      iGaucheVars->echelle *= 1.05;
   }

   if(zoneADessiner != iGaucheVars->zonePrincipale && GetTypeElement(element) == BULLE)
   {
      /* gauche */
      glBegin(GL_QUADS);
      couleurContour(zoneADessiner, iGaucheVars->zoneSelectionnee);
      glVertex2f(LireZoneX(zoneADessiner),
                 LireZoneY(zoneADessiner) + MARGE);

      couleurInterieur(zoneADessiner, iGaucheVars->zoneSelectionnee);
      glVertex2f(LireZoneX(zoneADessiner) + MARGE,
                 LireZoneY(zoneADessiner) + MARGE);
      couleurInterieur(zoneADessiner, iGaucheVars->zoneSelectionnee);
      glVertex2f(LireZoneX(zoneADessiner) + MARGE,
                 LireZoneY(zoneADessiner) + LireZoneHauteur(zoneADessiner) - MARGE);
      couleurContour(zoneADessiner, iGaucheVars->zoneSelectionnee);
      glVertex2f(LireZoneX(zoneADessiner),
                 LireZoneY(zoneADessiner) + LireZoneHauteur(zoneADessiner) - MARGE);
      glEnd();


      /* coin en bas à gauche */
      glBegin(GL_POLYGON);
      couleurInterieur(zoneADessiner, iGaucheVars->zoneSelectionnee);
      glVertex2f(LireZoneX(zoneADessiner) + MARGE,
                 LireZoneY(zoneADessiner) + MARGE);

      couleurContour(zoneADessiner, iGaucheVars->zoneSelectionnee);
      for(i = 0; i <= M_PI/2; i += M_PI/30)
      {
         glVertex2f(LireZoneX(zoneADessiner) + MARGE - MARGE * cos(i),
                    LireZoneY(zoneADessiner) + MARGE - MARGE * sin(i));
      }
      glEnd();

      /* bas */
      glBegin(GL_QUADS);
      couleurContour(zoneADessiner, iGaucheVars->zoneSelectionnee);
      glVertex2f(LireZoneX(zoneADessiner) + MARGE,
                 LireZoneY(zoneADessiner));
      couleurInterieur(zoneADessiner, iGaucheVars->zoneSelectionnee);
      glVertex2f(LireZoneX(zoneADessiner) + MARGE,
                 LireZoneY(zoneADessiner) + MARGE);
      couleurInterieur(zoneADessiner, iGaucheVars->zoneSelectionnee);
      glVertex2f(LireZoneX(zoneADessiner) + LireZoneLargeur(zoneADessiner) - MARGE,
                 LireZoneY(zoneADessiner) + MARGE);
      couleurContour(zoneADessiner, iGaucheVars->zoneSelectionnee);
      glVertex2f(LireZoneX(zoneADessiner) + LireZoneLargeur(zoneADessiner) - MARGE,
                 LireZoneY(zoneADessiner));
      glEnd();


      /* coin en bas à droite */
      glBegin(GL_POLYGON);
      couleurInterieur(zoneADessiner, iGaucheVars->zoneSelectionnee);
      glVertex2f(LireZoneX(zoneADessiner) + LireZoneLargeur(zoneADessiner) - MARGE,
                 LireZoneY(zoneADessiner) + MARGE);

      couleurContour(zoneADessiner, iGaucheVars->zoneSelectionnee);
      for(i = 0; i <= M_PI/2; i += M_PI/30)
      {
         glVertex2f(LireZoneX(zoneADessiner) + LireZoneLargeur(zoneADessiner) - MARGE + MARGE * cos(i),
                    LireZoneY(zoneADessiner) + MARGE - MARGE * sin(i));
      }
      glEnd();

      /* droit */
      glBegin(GL_QUADS);
      couleurContour(zoneADessiner, iGaucheVars->zoneSelectionnee);
      glVertex2f(LireZoneX(zoneADessiner) + LireZoneLargeur(zoneADessiner),
                 LireZoneY(zoneADessiner) + MARGE);
      couleurInterieur(zoneADessiner, iGaucheVars->zoneSelectionnee);
      glVertex2f(LireZoneX(zoneADessiner) + LireZoneLargeur(zoneADessiner) - MARGE,
                 LireZoneY(zoneADessiner) + MARGE);
      couleurInterieur(zoneADessiner, iGaucheVars->zoneSelectionnee);
      glVertex2f(LireZoneX(zoneADessiner) + LireZoneLargeur(zoneADessiner) - MARGE,
                 LireZoneY(zoneADessiner) + LireZoneHauteur(zoneADessiner) - MARGE);
      couleurContour(zoneADessiner, iGaucheVars->zoneSelectionnee);
      glVertex2f(LireZoneX(zoneADessiner) + LireZoneLargeur(zoneADessiner),
                 LireZoneY(zoneADessiner) + LireZoneHauteur(zoneADessiner) - MARGE);
      glEnd();


      /* coin en haut à droite */
      glBegin(GL_POLYGON);
      couleurInterieur(zoneADessiner, iGaucheVars->zoneSelectionnee);
      glVertex2f(LireZoneX(zoneADessiner) + LireZoneLargeur(zoneADessiner) - MARGE,
                 LireZoneY(zoneADessiner) + LireZoneHauteur(zoneADessiner) - MARGE);

      couleurContour(zoneADessiner, iGaucheVars->zoneSelectionnee);
      for(i = 0; i <= M_PI/2; i += M_PI/30)
      {
         glVertex2f(LireZoneX(zoneADessiner) + LireZoneLargeur(zoneADessiner) - MARGE + MARGE * cos(i),
                    LireZoneY(zoneADessiner) + LireZoneHauteur(zoneADessiner) - MARGE + MARGE * sin(i));
      }
      glEnd();

      /* haut */
      glBegin(GL_QUADS);
      couleurContour(zoneADessiner, iGaucheVars->zoneSelectionnee);
      glVertex2f(LireZoneX(zoneADessiner) + MARGE,
                 LireZoneY(zoneADessiner) + LireZoneHauteur(zoneADessiner));
      couleurInterieur(zoneADessiner, iGaucheVars->zoneSelectionnee);
      glVertex2f(LireZoneX(zoneADessiner) + MARGE,
                 LireZoneY(zoneADessiner) + LireZoneHauteur(zoneADessiner) - MARGE);
      couleurInterieur(zoneADessiner, iGaucheVars->zoneSelectionnee);
      glVertex2f(LireZoneX(zoneADessiner) + LireZoneLargeur(zoneADessiner) - MARGE,
                 LireZoneY(zoneADessiner) + LireZoneHauteur(zoneADessiner) - MARGE);
      couleurContour(zoneADessiner, iGaucheVars->zoneSelectionnee);
      glVertex2f(LireZoneX(zoneADessiner) + LireZoneLargeur(zoneADessiner) - MARGE,
                 LireZoneY(zoneADessiner) + LireZoneHauteur(zoneADessiner));
      glEnd();

      /* coin en haut à gauche */
      glBegin(GL_POLYGON);
      couleurInterieur(zoneADessiner, iGaucheVars->zoneSelectionnee);
      glVertex2f(LireZoneX(zoneADessiner) + MARGE,
                 LireZoneY(zoneADessiner) + LireZoneHauteur(zoneADessiner) - MARGE);

      couleurContour(zoneADessiner, iGaucheVars->zoneSelectionnee);
      for(i = 0; i <= M_PI/2; i += M_PI/30)
      {
         glVertex2f(LireZoneX(zoneADessiner) + MARGE - MARGE * cos(i),
                    LireZoneY(zoneADessiner) + LireZoneHauteur(zoneADessiner) - MARGE + MARGE * sin(i));
      }
      glEnd();

      /* interieur */
      couleurInterieur(zoneADessiner, iGaucheVars->zoneSelectionnee);
      glBegin(GL_POLYGON);
      glVertex2f(LireZoneX(zoneADessiner) + MARGE,
                 LireZoneY(zoneADessiner) + MARGE);
      glVertex2f(LireZoneX(zoneADessiner) + MARGE,
                 LireZoneY(zoneADessiner) + LireZoneHauteur(zoneADessiner) - MARGE);
      glVertex2f(LireZoneX(zoneADessiner) + LireZoneLargeur(zoneADessiner) - MARGE,
                 LireZoneY(zoneADessiner) + LireZoneHauteur(zoneADessiner) - MARGE);
      glVertex2f(LireZoneX(zoneADessiner) + LireZoneLargeur(zoneADessiner) - MARGE,
                 LireZoneY(zoneADessiner) + MARGE);
      
      glEnd();


   }
   else if (GetTypeElement(element) == THREAD)
   {
      glBegin(GL_QUADS);
      for(i = -M_PI/4; i < M_PI/4; i += M_PI/20)
      {
         couleurInterieur(zoneADessiner, iGaucheVars->zoneSelectionnee);
         glVertex2f(LireZoneX(zoneADessiner) + M_SQRT2 * LARGEUR_T / 2 *cos(i) ,
                    LireZoneY(zoneADessiner) + HAUTEUR_T * 0.3 + M_SQRT2 * HAUTEUR_T / 5 *sin(i));
         glVertex2f(LireZoneX(zoneADessiner) + M_SQRT2 * LARGEUR_T / 2 *cos(i + M_PI/20) ,
                    LireZoneY(zoneADessiner) + HAUTEUR_T * 0.3 + M_SQRT2 * HAUTEUR_T / 5 *sin(i + M_PI/20));
         couleurContour(zoneADessiner, iGaucheVars->zoneSelectionnee);
         glVertex2f(LireZoneX(zoneADessiner) + M_SQRT2 * LARGEUR_T * 0.625 *cos(i + M_PI/20) ,
                    LireZoneY(zoneADessiner) + HAUTEUR_T * 0.3 + M_SQRT2 * HAUTEUR_T / 4 *sin(i + M_PI/20));
         glVertex2f(LireZoneX(zoneADessiner) + M_SQRT2 * LARGEUR_T * 0.625 *cos(i) ,
                    LireZoneY(zoneADessiner) + HAUTEUR_T * 0.3 + M_SQRT2 * HAUTEUR_T / 4 *sin(i));
      }
      for(i = -M_PI/4; i < M_PI/4; i += M_PI/20)
      {
         couleurContour(zoneADessiner, iGaucheVars->zoneSelectionnee);
         glVertex2f(LireZoneX(zoneADessiner) + M_SQRT2 * LARGEUR_T * 0.375 *cos(i) ,
                    LireZoneY(zoneADessiner) + HAUTEUR_T * 0.3 + M_SQRT2 * HAUTEUR_T * 0.15 *sin(i));
         glVertex2f(LireZoneX(zoneADessiner) + M_SQRT2 * LARGEUR_T * 0.375 *cos(i + M_PI/20) ,
                    LireZoneY(zoneADessiner) + HAUTEUR_T * 0.3 + M_SQRT2 * HAUTEUR_T * 0.15 *sin(i + M_PI/20));
         couleurInterieur(zoneADessiner, iGaucheVars->zoneSelectionnee);
         glVertex2f(LireZoneX(zoneADessiner) + M_SQRT2 * LARGEUR_T / 2 *cos(i + M_PI/20) ,
                    LireZoneY(zoneADessiner) + HAUTEUR_T * 0.3 + M_SQRT2 * HAUTEUR_T / 5 *sin(i + M_PI/20));
         glVertex2f(LireZoneX(zoneADessiner) + M_SQRT2 * LARGEUR_T / 2 *cos(i) ,
                    LireZoneY(zoneADessiner) + HAUTEUR_T * 0.3 + M_SQRT2 * HAUTEUR_T / 5 *sin(i));
      }

      for(i = 3*M_PI/4; i < 5*M_PI/4; i += M_PI/20)
      {
         couleurInterieur(zoneADessiner, iGaucheVars->zoneSelectionnee);
         glVertex2f(LireZoneX(zoneADessiner) + LARGEUR_T +M_SQRT2 * LARGEUR_T / 2 *cos(i) ,
                    LireZoneY(zoneADessiner) + HAUTEUR_T * 0.7 + M_SQRT2 * HAUTEUR_T / 5 *sin(i));
         glVertex2f(LireZoneX(zoneADessiner) + LARGEUR_T + M_SQRT2 * LARGEUR_T / 2 *cos(i + M_PI/20) ,
                    LireZoneY(zoneADessiner) + HAUTEUR_T * 0.7 + M_SQRT2 * HAUTEUR_T / 5 *sin(i + M_PI/20));
         couleurContour(zoneADessiner, iGaucheVars->zoneSelectionnee);
         glVertex2f(LireZoneX(zoneADessiner) + LARGEUR_T + M_SQRT2 * LARGEUR_T * 0.625 *cos(i + M_PI/20) ,
                    LireZoneY(zoneADessiner) + HAUTEUR_T * 0.7 + M_SQRT2 * HAUTEUR_T / 4 *sin(i + M_PI/20));
         glVertex2f(LireZoneX(zoneADessiner) + LARGEUR_T + M_SQRT2 * LARGEUR_T * 0.625 *cos(i) ,
                    LireZoneY(zoneADessiner) + HAUTEUR_T * 0.7 + M_SQRT2 * HAUTEUR_T / 4 *sin(i));
      }
      for(i = 3*M_PI/4; i < 5*M_PI/4; i += M_PI/20)
      {
         couleurContour(zoneADessiner, iGaucheVars->zoneSelectionnee);
         glVertex2f(LireZoneX(zoneADessiner) + LARGEUR_T + M_SQRT2 * LARGEUR_T * 0.375 *cos(i) ,
                    LireZoneY(zoneADessiner) + HAUTEUR_T * 0.7 + M_SQRT2 * HAUTEUR_T * 0.15 *sin(i));
         glVertex2f(LireZoneX(zoneADessiner) + LARGEUR_T + M_SQRT2 * LARGEUR_T * 0.375 *cos(i + M_PI/20) ,
                    LireZoneY(zoneADessiner) + HAUTEUR_T * 0.7 + M_SQRT2 * HAUTEUR_T * 0.15 *sin(i + M_PI/20));
         couleurInterieur(zoneADessiner, iGaucheVars->zoneSelectionnee);
         glVertex2f(LireZoneX(zoneADessiner) + LARGEUR_T + M_SQRT2 * LARGEUR_T / 2 *cos(i + M_PI/20) ,
                    LireZoneY(zoneADessiner) + HAUTEUR_T * 0.7 + M_SQRT2 * HAUTEUR_T / 5 *sin(i + M_PI/20));
         glVertex2f(LireZoneX(zoneADessiner) + LARGEUR_T + M_SQRT2 * LARGEUR_T / 2 *cos(i) ,
                    LireZoneY(zoneADessiner) + HAUTEUR_T * 0.7 + M_SQRT2 * HAUTEUR_T / 5 *sin(i));
      }
      glEnd();



      glBegin(GL_POLYGON);
      couleurInterieur(zoneADessiner, iGaucheVars->zoneSelectionnee);
      glVertex2f(LireZoneX(zoneADessiner) + LARGEUR_T / 2,
                 LireZoneY(zoneADessiner) + HAUTEUR_T * 0.9);

      couleurContour(zoneADessiner, iGaucheVars->zoneSelectionnee);
      for(i = -M_PI/4; i <= 3*M_PI/4; i += M_PI/30)
      {
         glVertex2f(LireZoneX(zoneADessiner) + LARGEUR_T / 2 + M_SQRT2 * LARGEUR_T * 0.125 * cos(i),
                    LireZoneY(zoneADessiner) + HAUTEUR_T * 0.9 + M_SQRT2 * HAUTEUR_T * 0.05 * sin(i));
      }
      glEnd();

      glBegin(GL_POLYGON);
      couleurInterieur(zoneADessiner, iGaucheVars->zoneSelectionnee);
      glVertex2f(LireZoneX(zoneADessiner) + LARGEUR_T / 2,
                 LireZoneY(zoneADessiner) + HAUTEUR_T / 10);

      couleurContour(zoneADessiner, iGaucheVars->zoneSelectionnee);
      for(i = 3*M_PI/4; i <= 7*M_PI/4; i += M_PI/30)
      {
         glVertex2f(LireZoneX(zoneADessiner) + LARGEUR_T / 2 + M_SQRT2 * LARGEUR_T * 0.125 * cos(i),
                    LireZoneY(zoneADessiner) + HAUTEUR_T / 10 + M_SQRT2 * HAUTEUR_T * 0.05 * sin(i));
      }
      glEnd();

   }

   return;
}







/***********************************************/
void make_left_drawable_zone(interfaceGaucheVars *iGaucheVars)
{
   GtkWidget* drawzone;

   zone* zonePrincipale;
   Element* bullePrincipale;

   drawzone = gtk_drawing_area_new();

   iGaucheVars->drawzone_left = drawzone;

   bullePrincipale = CreateBulle(1, 0);

   zonePrincipale = CreerZone(0, 0, 200,  100);

/*    zone* zone2; */
/*    zone* zone3; */
/*    zone* zone4; */
/*    zone* zone5; */
/*    zone* zone6; */
/*    zone* zone7; */

/*    Element * bulle2; */
/*    Element * bulle3; */
/*    Element * bulle4; */
/*    Element * bulle5; */
/*    Element * bulle6; */
/*    Element * bulle7; */
   
/*    bulle2 = CreateBulle(2); */
/*    bulle3 = CreateBulle(3); */
/*    bulle4 = CreateBulle(4); */
/*    bulle6 = CreateBulle(5); */
/*    bulle5 = CreateBulle(6); */
/*    bulle7 = CreateBulle(7); */


/*    zone2 = CreerZone(0, 0, 50, 50); */
/*    zone3 = CreerZone(0, 0, 50, 50); */
/*    zone4 = CreerZone(0, 0, 15, 50); */
/*    zone5 = CreerZone(0, 0, 15, 50); */
/*    zone6 = CreerZone(0, 0, 15, 50); */
/*    zone7 = CreerZone(0, 0, 15, 50); */

/*    AjouterSousZones(zonePrincipale, zone2); */
/*    AjouterSousZones(zonePrincipale, zone3); */
/*    AjouterSousZones(zonePrincipale, zone4); */
/*    AjouterSousZones(zonePrincipale, zone5); */
/*    AjouterSousZones(zone2, zone6); */
/*    AjouterSousZones(zone2, zone7); */

/*    AddElement(bullePrincipale, bulle2); */
/*    AddElement(bullePrincipale, bulle3); */
/*    AddElement(bullePrincipale, bulle4); */
/*    AddElement(bullePrincipale, bulle5); */
/*    AddElement(bulle2, bulle6); */
/*    AddElement(bulle2, bulle7); */


   Rearanger(zonePrincipale);

   iGaucheVars->bullePrincipale = bullePrincipale;
   
   iGaucheVars->zonePrincipale = zonePrincipale;

   iGaucheVars->zoneSelectionnee = zonePrincipale;


   // on enfiche la drawing area dans la place qui reste dans notre partie du hpane
   gtk_box_pack_start(GTK_BOX(iGaucheVars->interfaceGauche), drawzone, TRUE, TRUE, 0);

   // ceci a pour effet de fixer la taille MINIMUM définitive de la drawzone, en mode fill et expand
   // si on spécifie une taille plus petite que le widget n'a déjà, il sera étendu.
   // si on lui spécifie une taille plus grande, des barres de défilement sont créées.
   gtk_drawing_area_size(GTK_DRAWING_AREA(drawzone), 100, 100);

   // on commence a configurer les modes opengl
   GdkGLConfig* glconf = NULL;
   glconf = gdk_gl_config_new_by_mode(GDK_GL_MODE_RGBA | GDK_GL_MODE_DOUBLE | GDK_GL_MODE_ALPHA);
   if (glconf == NULL)
   { /* peut etre un probleme, double buffer inacceptable sur cette config ? */
      glconf = gdk_gl_config_new_by_mode(GDK_GL_MODE_RGBA | GDK_GL_MODE_ALPHA);
      printf("info : initiliasation en simple buffer (double impossible)\n");
   }
   if (glconf == NULL)
   { /* peut etre un probleme, alpha inacceptable sur cette config ? */
      glconf = gdk_gl_config_new_by_mode(GDK_GL_MODE_RGBA);
      printf("info : initiliasation sans alpha (ce sera moins joli)\n");
   }
   if (glconf == NULL)
   {
      wprintf(L"aie : problème de configuration opengl !\n");
      wprintf(L"Il n'y aura pas d'affichage\n");
   } else
   if (!gtk_widget_set_gl_capability(drawzone, glconf, NULL, TRUE, GDK_GL_RGBA_TYPE))
   { /* ok, ya un truc qui n'a pas marché */
      printf("aie : La promotion au rang de widget OpenGL a faillie lamentablement.\n");
   }



   // on va lui donner une autre promotion au widget (waw c'est un widget colonel maintenant)
   // il peut recevoir des événements bas niveau de mouvement de souris:
   gtk_widget_set_events(drawzone,
                         GDK_BUTTON_RELEASE_MASK |
                         GDK_BUTTON_PRESS_MASK |
                         GDK_POINTER_MOTION_MASK);

   /***************************** G_SIGNAL_CONNECT ***************************/
   
   // quand la souris bouge sur la zone on a des trucs a regarder:
     g_signal_connect(G_OBJECT(drawzone), "button_press_event", G_CALLBACK(MouseMove_left_dz), iGaucheVars);
   
#if 0
   // deplacement de bulles et de treads   
   g_signal_connect(G_OBJECT(drawzone), "motion_notify_event", G_CALLBACK(MouseMove_left_movebt), iGaucheVars);
#endif

   // relachement du click souris
   g_signal_connect(G_OBJECT(drawzone), "button_release_event", G_CALLBACK(MouseMove_left_release), iGaucheVars);

   // on lie l'evennement à occurence unique: realize
   g_signal_connect(G_OBJECT(drawzone), "realize", G_CALLBACK(Realize_left_dz), iGaucheVars);

   // on lie aussi le redimensionnement pour rester au courant de la taille de la zone:
   g_signal_connect(G_OBJECT(drawzone), "configure_event", G_CALLBACK(Reshape_left_dz), iGaucheVars);


    /***************************** /G_SIGNAL_CONNECT ***************************/

   // on rajoute un pti timer qui appelle une callback redraw toutes les 50 ms
   g_timeout_add(50,
                 Redraw_left_dz,
                 iGaucheVars);
   // c'est le seul moyen que j'ai trouvé pour faire de l'animation même en laissant
   // le flot d'execution s'engoufrer dans la fonction gtk_main dans laquelle nous
   // n'avons plus la main.
}


// réglages des parametres opengl définitifs
void Realize_left_dz(GtkWidget *widget, gpointer data)
{
  interfaceGaucheVars *iGaucheVars = (interfaceGaucheVars*)data;
  
  GdkGLContext* glcontext = gtk_widget_get_gl_context(widget);
  GdkGLDrawable* gldrawable = gtk_widget_get_gl_drawable(widget);
  
  
  if (gdk_gl_drawable_gl_begin(gldrawable, glcontext))
    {
      // zone d'initialisation des états opengl
      
      glClearColor(0.0, 0.0, 0.0, 1.0);  // fond noir
      glDisable(GL_DEPTH);       // pas de zbuffer
      
      glLoadIdentity();
      glEnable(GL_BLEND);   // transparence
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      glDisable(GL_CULL_FACE);   // pas de culling
      
      PushScreenCoordinateMatrix();   // coordonnées en pixels
      
      int i;
      for (i = 0; i < 2; ++i)  // on nettoie une premiere fois les deux buffers
	{
	  glClear(GL_COLOR_BUFFER_BIT);
	  /* Swap buffers. */
	  if (gdk_gl_drawable_is_double_buffered(gldrawable))
            gdk_gl_drawable_swap_buffers(gldrawable);
         else
	   glFlush ();
	}
      
      glScalef( 0.5, 0.5, 0.5);
      iGaucheVars->echelle = 0.5;
      
      gdk_gl_drawable_gl_end(gldrawable);
    }
  else
    {
      printf("attention : opengl inactif\n");
    }
}


/* callback du timer de la glib */
gboolean Redraw_left_dz(gpointer data)
{
  interfaceGaucheVars* iGaucheVars = (interfaceGaucheVars*)data;
  
  GdkGLContext* glcontext = gtk_widget_get_gl_context(iGaucheVars->drawzone_left);
  GdkGLDrawable* gldrawable = gtk_widget_get_gl_drawable(iGaucheVars->drawzone_left);
  
  if (gdk_gl_drawable_gl_begin(gldrawable, glcontext))
    {
      DessinerTout(iGaucheVars);
      
      if (gdk_gl_drawable_is_double_buffered(gldrawable))
         gdk_gl_drawable_swap_buffers(gldrawable);
      else
	glFlush ();
      
      gdk_gl_drawable_gl_end(gldrawable);
    }
  
  /*    Rearanger(iGaucheVars->zonePrincipale); */
  
  return TRUE;
}


/* callback de "configure_event" */
gboolean Reshape_left_dz(GtkWidget* widget, GdkEventConfigure* ev, gpointer data)
{
  widget = NULL;  // inutilisé
  interfaceGaucheVars* iGaucheVars = (interfaceGaucheVars*)data;
  
  GdkGLContext* glcontext = gtk_widget_get_gl_context(iGaucheVars->drawzone_left);
  GdkGLDrawable* gldrawable = gtk_widget_get_gl_drawable(iGaucheVars->drawzone_left);

  // lorsque la fenêtre est redimensionnée, il faut redimensionner la zone opengl et adapter la projection.
  iGaucheVars->area_left_x = ev->width;
  iGaucheVars->area_left_y = ev->height;
  
  if (gdk_gl_drawable_gl_begin(gldrawable, glcontext))
    {
      glViewport(0, 0, ev->width, ev->height);
      PopProjectionMatrix();
      PushScreenCoordinateMatrix();
      
      gdk_gl_drawable_gl_end(gldrawable);
    }
   return TRUE;
}

/* callback de "button_press_event" */
gboolean MouseMove_left_dz(GtkWidget* widget, GdkEventMotion* ev, gpointer data)
{
  parcours *p;
  widget = NULL;  // inutilisé
  interfaceGaucheVars* iGaucheVars = (interfaceGaucheVars*)data;
  zone * nouvelleZoneSelectionnee;
  Element* elementSelectionne;
  
  iGaucheVars->mousePos_left_x = (ev->x) / iGaucheVars->echelle;
  iGaucheVars->mousePos_left_y = (iGaucheVars->area_left_y - ev->y) / iGaucheVars->echelle;

  /* Pour test */
  iGaucheVars->mousePosClic_left_x =  iGaucheVars->mousePos_left_x;
  iGaucheVars->mousePosClic_left_y =  iGaucheVars->mousePos_left_y;
  
  p = TrouverParcours(iGaucheVars->zonePrincipale, iGaucheVars->mousePos_left_x, iGaucheVars->mousePos_left_y);
  nouvelleZoneSelectionnee = LireZoneParcours(iGaucheVars->zonePrincipale, p);
  iGaucheVars->zoneSelectionnee = nouvelleZoneSelectionnee;
  elementSelectionne = LireElementParcours(iGaucheVars->bullePrincipale, p);
  EffacerParcours(p);
   if (elementSelectionne->type==THREAD)
    {
  // On repositionne les paramétres des ascenseurs et de la zone texte correspondant au données du thread
  gtk_range_set_value (GTK_RANGE(iGaucheVars->charge),(int)GetCharge(elementSelectionne));
  gtk_range_set_value (GTK_RANGE(iGaucheVars->priorite),(int)GetPrioriteThread(elementSelectionne));
  gtk_entry_set_text (GTK_ENTRY(iGaucheVars->nom),GetNom(elementSelectionne));
  // On sauve le thread selectionné en cas de changement des ses valeurs.
  iGaucheVars->ThreadSelect=elementSelectionne;
    }
  else
    {
      printf("deselection\n");
       iGaucheVars->ThreadSelect=NULL; 
       /* TODO: remettre plut�t les valeurs qu'on avait avant */
       gtk_range_set_value (GTK_RANGE(iGaucheVars->charge),DEF_CHARGE);
       gtk_range_set_value (GTK_RANGE(iGaucheVars->priorite),DEF_PRIO);
  gtk_entry_set_text (GTK_ENTRY(iGaucheVars->nom),"");
    }

  /* pour test
  k++; */
  /* variable globale pour test */
  /*
  printf("Clic souris détecté %d\n",k);  */

  return TRUE;
}

/*! On vérifie que la sélection n'est pas vide
 *  Le clic gauche doit être pressé sur la sélection 
 *  et relaché au dessus d'une
 *  bulle ne faisant pas partie de la sélection
 */

#if 0
gboolean MouseMove_left_movebt(GtkWidget* widget, GdkEventMotion* ev, gpointer data)
{
  parcours *p;
  widget = NULL;  // inutilisé
  interfaceGaucheVars* iGaucheVars = (interfaceGaucheVars*)data;
  zone * nouvelleZoneSelectionnee;
  Element* elementSelectionne;
  
  // printf("déplacement détecté %d\n",i);
  i++;
  return TRUE;
}
#endif
  
/*!
 *
 * 
 * pour cette fonction, le mieux serait de sauvegarder 
 *  dans une structure les elements selectionnées 
 */
gboolean MouseMove_left_release(GtkWidget* widget, GdkEventMotion* ev, gpointer data)
{
  parcours *p, *pParent, *pAccueil, *pParentAccueil;
  widget = NULL;  // inutilisé
  interfaceGaucheVars* iGaucheVars = (interfaceGaucheVars*)data;
  zone * ZoneSelectionnee, *ZoneParent, * ZoneAccueil, *ZoneParentAccueil;
  Element* elementSelectionne, * elementParent, * elementAccueil, *elementParentAccueil;
  Bulle* bulleParent, * bulleAccueil, * bulleParentAccueil;
  int clicX, clicY, lacheX, lacheY; /*position du clic et du relachement du clic */ 
  
  /* Mise à jour de iGaucheVars pour connaître les coordonnées du relâchement du clic */
  iGaucheVars->mousePos_left_x = (ev->x) / iGaucheVars->echelle;
  iGaucheVars->mousePos_left_y = (iGaucheVars->area_left_y - ev->y) / iGaucheVars->echelle;
  clicX = iGaucheVars->mousePosClic_left_x;
  clicY = iGaucheVars->mousePosClic_left_y;
  lacheX = iGaucheVars->mousePos_left_x;
  lacheY = iGaucheVars->mousePos_left_y;

  /* CALCUL DES ELEMENTS SELECTIONNES, PARENT ET ACCUEIL */
  /* Voilà l'élément sélectionné qu'on va déplacer */
  p = TrouverParcours(iGaucheVars->zonePrincipale, iGaucheVars->mousePosClic_left_x, iGaucheVars->mousePosClic_left_y);
  /* printf("Trace parcours élément à déplacer\n");
  traceParcours(p); */
  ZoneSelectionnee = LireZoneParcours(iGaucheVars->zonePrincipale, p);
  iGaucheVars->zoneSelectionnee = ZoneSelectionnee;
  elementSelectionne = LireElementParcours(iGaucheVars->bullePrincipale, p);
  EffacerParcours(p);

  /* Voilà l'élément parent de l'élément sélectionné */
  pParent = TrouverParcours(iGaucheVars->zonePrincipale, LireZoneX(iGaucheVars->zoneSelectionnee) + 1, LireZoneY(iGaucheVars->zoneSelectionnee)+1);
  /*  printf("Trace parcours élément Parent\n");
  traceParcours(pParent);*/
  ZoneParent = LireZoneParcours(iGaucheVars->zonePrincipale, pParent);
  elementParent = LireElementParcours(iGaucheVars->bullePrincipale, pParent);
  bulleParent = &elementParent->bulle;
  EffacerParcours(pParent);
  
  /* Voilà l'élément d'accueil */
  pAccueil = TrouverParcours(iGaucheVars->zonePrincipale, iGaucheVars->mousePos_left_x, iGaucheVars->mousePos_left_y);
  /* printf("Trace parcours élément Accueil\n");
  traceParcours(pAccueil);*/
  ZoneAccueil = LireZoneParcours(iGaucheVars->zonePrincipale, pAccueil);
  elementAccueil = LireElementParcours(iGaucheVars->bullePrincipale, pAccueil);
  bulleAccueil = &elementAccueil->bulle;
  EffacerParcours(pAccueil);
#if 0
  /* Voilà l'élément parent de l'élément d'accueil */
  pParentAccueil = TrouverParcours(iGaucheVars->zonePrincipale, iGaucheVars->mousePos_left_x + 1, iGaucheVars->mousePos_left_y + 1);
  /* printf("Trace parcours élément ParentAccueil\n");
  traceParcours(pParentAccueil);*/
  ZoneParentAccueil = LireZoneParcours(iGaucheVars->zonePrincipale, pParentAccueil);
  elementParentAccueil = LireElementParcours(iGaucheVars->bullePrincipale, pParentAccueil);
  bulleParentAccueil = &elementParentAccueil->bulle;
  EffacerParcours(pParentAccueil);
#endif    
  printf("Eléments sélectionné, parent et d'accueil calculés\n");


  
  /* Ici les tests pour voir si on peut effectuer les déplacements */
  /* Pour test, on regarde si la ZoneSelectionnee est vide */
  if(ZoneSelectionnee == NULL)
  {
    printf("Zone sélectionnée vide\n");
    return FALSE;
  }
   /* après tests, il s'avère que la ZoneSelectionnee n'est jamais vide */
  if(ZoneSelectionnee == iGaucheVars->zonePrincipale)
  {
    printf("Zone sélectionnée bullePrincipale\n");
    return FALSE;
  }
  if (ZoneAccueil == NULL)
  {
    printf("Zone d'accueil vide\n");
    return FALSE;
   }
  if(elementAccueil->type == THREAD)
  {
    printf("Accueil de type thread, pas de déplacement.\n");
    return FALSE;
  }
  /* On teste pour voir si la Zone de relâchement du clic et celle du clic initial sont identiques */
  if(ZoneSelectionnee == ZoneAccueil || ZoneParent == ZoneAccueil)
  {
    printf("Zone identique, pas de déplacement.\n");
    return FALSE;
  }
  /* Si l'élément d'accueil est un fils de l'élément sélectionné, on ne fait rien */
  if(appartientElementParent(elementSelectionne, elementAccueil))
  {
     printf("Zone d'accueil comprise dans la zone sélectionnée, pas de déplacement.\n");
     return FALSE;
   } 
  /* Finalement on fait le déplacement */

  printf("Zones différentes, déplacement possible.\n");
 
  Deplacer(iGaucheVars->bullePrincipale, iGaucheVars->zonePrincipale,
		    iGaucheVars->mousePosClic_left_x, iGaucheVars->mousePosClic_left_y,
		    iGaucheVars->mousePos_left_x, iGaucheVars->mousePos_left_y);

  
  return TRUE;
}
