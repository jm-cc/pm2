#include "interfaceGaucheDessin.h"
#include "interfaceGauche.h"
#include "bulle.h"
#include<math.h>
/* Selection Multiple*/
#if 1
extern interfaceGaucheVars* iGaucheVars;
#endif
/*Fin*/

#define scale(v,charge) v*(1-0.8*(charge-DEF_CHARGE)/(MAX_CHARGE-DEF_CHARGE))
#define rgbchargeprio(r,g,b,charge,prio) scale(r,charge),scale(g,charge),scale(b,charge)

static void couleurContour(zone* zoneADessiner, zone* zoneSelectionnee, int charge, int prio)
{
  if (zoneADessiner == zoneSelectionnee)
    glColor3f(rgbchargeprio(0.1,0.8,0.1,charge,prio));
  else
    glColor3f(rgbchargeprio(0.3,0.6,0.7,charge,prio));

  return;
}


static void couleurInterieur(zone* zoneADessiner, zone* zoneSelectionnee, int charge, int prio)
{
  if (zoneADessiner == zoneSelectionnee)
    glColor3f(rgbchargeprio(0.5,1,0.5,charge,prio));
  else
    glColor3f(rgbchargeprio(0.5,1,1,charge,prio));

  return;
}


void DessinerTout(interfaceGaucheVars *iGaucheVars)
{

  Clear();
  if(iGaucheVars->zonePrincipale != NULL)
    Dessiner(iGaucheVars, iGaucheVars->zonePrincipale);
}


void Dessiner(interfaceGaucheVars *iGaucheVarsPar, zone * zonePrincipale)
{
  int i;

  zone* sousZone;

  TracerZone(iGaucheVarsPar, zonePrincipale);

  /* et on fait pareil recursivement sur les sous-zones */
  for(i = 1;
      (sousZone = LireSousZones(zonePrincipale, i)) != NULL;
      i++)
    /*Selection Multiple*/
   #if 1
   {

     if(isInside(iGaucheVars->head,sousZone) && iGaucheVars->head != zonePrincipale)
   {
   iGaucheVarsPar->zoneSelectionnee = sousZone;
    Dessiner(iGaucheVarsPar, sousZone);
   }
   else
    Dessiner(iGaucheVarsPar, sousZone);
   }
   #endif
  /*Fin*/

  /*Slection Simple */
  #if 0
    Dessiner(iGaucheVarsPar, sousZone);
  #endif
   /*Fin*/
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
      wprintf(L"Zone à dessiner posX : %d, posY : %d, largeur : %d, hauteur %d\n",
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
      couleurContour(zoneADessiner, iGaucheVars->zoneSelectionnee, DEF_CHARGE, element->bulle.priorite);
      glVertex2f(LireZoneX(zoneADessiner),
                 LireZoneY(zoneADessiner) + MARGE);

      couleurInterieur(zoneADessiner, iGaucheVars->zoneSelectionnee, DEF_CHARGE, element->bulle.priorite);
      glVertex2f(LireZoneX(zoneADessiner) + MARGE,
                 LireZoneY(zoneADessiner) + MARGE);
      couleurInterieur(zoneADessiner, iGaucheVars->zoneSelectionnee, DEF_CHARGE, element->bulle.priorite);
      glVertex2f(LireZoneX(zoneADessiner) + MARGE,
                 LireZoneY(zoneADessiner) + LireZoneHauteur(zoneADessiner) - MARGE);
      couleurContour(zoneADessiner, iGaucheVars->zoneSelectionnee, DEF_CHARGE, element->bulle.priorite);
      glVertex2f(LireZoneX(zoneADessiner),
                 LireZoneY(zoneADessiner) + LireZoneHauteur(zoneADessiner) - MARGE);
      glEnd();


      /* coin en bas à gauche */
      glBegin(GL_POLYGON);
      couleurInterieur(zoneADessiner, iGaucheVars->zoneSelectionnee, DEF_CHARGE, element->bulle.priorite);
      glVertex2f(LireZoneX(zoneADessiner) + MARGE,
                 LireZoneY(zoneADessiner) + MARGE);

      couleurContour(zoneADessiner, iGaucheVars->zoneSelectionnee, DEF_CHARGE, element->bulle.priorite);
      for(i = 0; i <= M_PI/2; i += M_PI/30)
	{
	  glVertex2f(LireZoneX(zoneADessiner) + MARGE - MARGE * cos(i),
		     LireZoneY(zoneADessiner) + MARGE - MARGE * sin(i));
	}
      glEnd();

      /* bas */
      glBegin(GL_QUADS);
      couleurContour(zoneADessiner, iGaucheVars->zoneSelectionnee, DEF_CHARGE, element->bulle.priorite);
      glVertex2f(LireZoneX(zoneADessiner) + MARGE,
                 LireZoneY(zoneADessiner));
      couleurInterieur(zoneADessiner, iGaucheVars->zoneSelectionnee, DEF_CHARGE, element->bulle.priorite);
      glVertex2f(LireZoneX(zoneADessiner) + MARGE,
                 LireZoneY(zoneADessiner) + MARGE);
      couleurInterieur(zoneADessiner, iGaucheVars->zoneSelectionnee, DEF_CHARGE, element->bulle.priorite);
      glVertex2f(LireZoneX(zoneADessiner) + LireZoneLargeur(zoneADessiner) - MARGE,
                 LireZoneY(zoneADessiner) + MARGE);
      couleurContour(zoneADessiner, iGaucheVars->zoneSelectionnee, DEF_CHARGE, element->bulle.priorite);
      glVertex2f(LireZoneX(zoneADessiner) + LireZoneLargeur(zoneADessiner) - MARGE,
                 LireZoneY(zoneADessiner));
      glEnd();


      /* coin en bas à droite */
      glBegin(GL_POLYGON);
      couleurInterieur(zoneADessiner, iGaucheVars->zoneSelectionnee, DEF_CHARGE, element->bulle.priorite);
      glVertex2f(LireZoneX(zoneADessiner) + LireZoneLargeur(zoneADessiner) - MARGE,
                 LireZoneY(zoneADessiner) + MARGE);

      couleurContour(zoneADessiner, iGaucheVars->zoneSelectionnee, DEF_CHARGE, element->bulle.priorite);
      for(i = 0; i <= M_PI/2; i += M_PI/30)
	{
	  glVertex2f(LireZoneX(zoneADessiner) + LireZoneLargeur(zoneADessiner) - MARGE + MARGE * cos(i),
		     LireZoneY(zoneADessiner) + MARGE - MARGE * sin(i));
	}
      glEnd();

      /* droit */
      glBegin(GL_QUADS);
      couleurContour(zoneADessiner, iGaucheVars->zoneSelectionnee, DEF_CHARGE, element->bulle.priorite);
      glVertex2f(LireZoneX(zoneADessiner) + LireZoneLargeur(zoneADessiner),
                 LireZoneY(zoneADessiner) + MARGE);
      couleurInterieur(zoneADessiner, iGaucheVars->zoneSelectionnee, DEF_CHARGE, element->bulle.priorite);
      glVertex2f(LireZoneX(zoneADessiner) + LireZoneLargeur(zoneADessiner) - MARGE,
                 LireZoneY(zoneADessiner) + MARGE);
      couleurInterieur(zoneADessiner, iGaucheVars->zoneSelectionnee, DEF_CHARGE, element->bulle.priorite);
      glVertex2f(LireZoneX(zoneADessiner) + LireZoneLargeur(zoneADessiner) - MARGE,
                 LireZoneY(zoneADessiner) + LireZoneHauteur(zoneADessiner) - MARGE);
      couleurContour(zoneADessiner, iGaucheVars->zoneSelectionnee, DEF_CHARGE, element->bulle.priorite);
      glVertex2f(LireZoneX(zoneADessiner) + LireZoneLargeur(zoneADessiner),
                 LireZoneY(zoneADessiner) + LireZoneHauteur(zoneADessiner) - MARGE);
      glEnd();


      /* coin en haut à droite */
      glBegin(GL_POLYGON);
      couleurInterieur(zoneADessiner, iGaucheVars->zoneSelectionnee, DEF_CHARGE, element->bulle.priorite);
      glVertex2f(LireZoneX(zoneADessiner) + LireZoneLargeur(zoneADessiner) - MARGE,
                 LireZoneY(zoneADessiner) + LireZoneHauteur(zoneADessiner) - MARGE);

      couleurContour(zoneADessiner, iGaucheVars->zoneSelectionnee, DEF_CHARGE, element->bulle.priorite);
      for(i = 0; i <= M_PI/2; i += M_PI/30)
	{
	  glVertex2f(LireZoneX(zoneADessiner) + LireZoneLargeur(zoneADessiner) - MARGE + MARGE * cos(i),
		     LireZoneY(zoneADessiner) + LireZoneHauteur(zoneADessiner) - MARGE + MARGE * sin(i));
	}
      glEnd();

      /* haut */
      glBegin(GL_QUADS);
      couleurContour(zoneADessiner, iGaucheVars->zoneSelectionnee, DEF_CHARGE, element->bulle.priorite);
      glVertex2f(LireZoneX(zoneADessiner) + MARGE,
                 LireZoneY(zoneADessiner) + LireZoneHauteur(zoneADessiner));
      couleurInterieur(zoneADessiner, iGaucheVars->zoneSelectionnee, DEF_CHARGE, element->bulle.priorite);
      glVertex2f(LireZoneX(zoneADessiner) + MARGE,
                 LireZoneY(zoneADessiner) + LireZoneHauteur(zoneADessiner) - MARGE);
      couleurInterieur(zoneADessiner, iGaucheVars->zoneSelectionnee, DEF_CHARGE, element->bulle.priorite);
      glVertex2f(LireZoneX(zoneADessiner) + LireZoneLargeur(zoneADessiner) - MARGE,
                 LireZoneY(zoneADessiner) + LireZoneHauteur(zoneADessiner) - MARGE);
      couleurContour(zoneADessiner, iGaucheVars->zoneSelectionnee, DEF_CHARGE, element->bulle.priorite);
      glVertex2f(LireZoneX(zoneADessiner) + LireZoneLargeur(zoneADessiner) - MARGE,
                 LireZoneY(zoneADessiner) + LireZoneHauteur(zoneADessiner));
      glEnd();

      /* coin en haut à gauche */
      glBegin(GL_POLYGON);
      couleurInterieur(zoneADessiner, iGaucheVars->zoneSelectionnee, DEF_CHARGE, element->bulle.priorite);
      glVertex2f(LireZoneX(zoneADessiner) + MARGE,
                 LireZoneY(zoneADessiner) + LireZoneHauteur(zoneADessiner) - MARGE);

      couleurContour(zoneADessiner, iGaucheVars->zoneSelectionnee, DEF_CHARGE, element->bulle.priorite);
      for(i = 0; i <= M_PI/2; i += M_PI/30)
	{
	  glVertex2f(LireZoneX(zoneADessiner) + MARGE - MARGE * cos(i),
		     LireZoneY(zoneADessiner) + LireZoneHauteur(zoneADessiner) - MARGE + MARGE * sin(i));
	}
      glEnd();

      /* interieur */
      couleurInterieur(zoneADessiner, iGaucheVars->zoneSelectionnee, DEF_CHARGE, element->bulle.priorite);
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
	  couleurInterieur(zoneADessiner, iGaucheVars->zoneSelectionnee, element->thread.charge, element->thread.priorite);
	  glVertex2f(LireZoneX(zoneADessiner) + M_SQRT2 * LARGEUR_T / 2 *cos(i) ,
		     LireZoneY(zoneADessiner) + HAUTEUR_T * 0.3 + M_SQRT2 * HAUTEUR_T / 5 *sin(i));
	  glVertex2f(LireZoneX(zoneADessiner) + M_SQRT2 * LARGEUR_T / 2 *cos(i + M_PI/20) ,
		     LireZoneY(zoneADessiner) + HAUTEUR_T * 0.3 + M_SQRT2 * HAUTEUR_T / 5 *sin(i + M_PI/20));
	  couleurContour(zoneADessiner, iGaucheVars->zoneSelectionnee, element->thread.charge, element->thread.priorite);
	  glVertex2f(LireZoneX(zoneADessiner) + M_SQRT2 * LARGEUR_T * 0.625 *cos(i + M_PI/20) ,
		     LireZoneY(zoneADessiner) + HAUTEUR_T * 0.3 + M_SQRT2 * HAUTEUR_T / 4 *sin(i + M_PI/20));
	  glVertex2f(LireZoneX(zoneADessiner) + M_SQRT2 * LARGEUR_T * 0.625 *cos(i) ,
		     LireZoneY(zoneADessiner) + HAUTEUR_T * 0.3 + M_SQRT2 * HAUTEUR_T / 4 *sin(i));
	}
      for(i = -M_PI/4; i < M_PI/4; i += M_PI/20)
	{
	  couleurContour(zoneADessiner, iGaucheVars->zoneSelectionnee, element->thread.charge, element->thread.priorite);
	  glVertex2f(LireZoneX(zoneADessiner) + M_SQRT2 * LARGEUR_T * 0.375 *cos(i) ,
		     LireZoneY(zoneADessiner) + HAUTEUR_T * 0.3 + M_SQRT2 * HAUTEUR_T * 0.15 *sin(i));
	  glVertex2f(LireZoneX(zoneADessiner) + M_SQRT2 * LARGEUR_T * 0.375 *cos(i + M_PI/20) ,
		     LireZoneY(zoneADessiner) + HAUTEUR_T * 0.3 + M_SQRT2 * HAUTEUR_T * 0.15 *sin(i + M_PI/20));
	  couleurInterieur(zoneADessiner, iGaucheVars->zoneSelectionnee, element->thread.charge, element->thread.priorite);
	  glVertex2f(LireZoneX(zoneADessiner) + M_SQRT2 * LARGEUR_T / 2 *cos(i + M_PI/20) ,
		     LireZoneY(zoneADessiner) + HAUTEUR_T * 0.3 + M_SQRT2 * HAUTEUR_T / 5 *sin(i + M_PI/20));
	  glVertex2f(LireZoneX(zoneADessiner) + M_SQRT2 * LARGEUR_T / 2 *cos(i) ,
		     LireZoneY(zoneADessiner) + HAUTEUR_T * 0.3 + M_SQRT2 * HAUTEUR_T / 5 *sin(i));
	}

      for(i = 3*M_PI/4; i < 5*M_PI/4; i += M_PI/20)
	{
	  couleurInterieur(zoneADessiner, iGaucheVars->zoneSelectionnee, element->thread.charge, element->thread.priorite);
	  glVertex2f(LireZoneX(zoneADessiner) + LARGEUR_T +M_SQRT2 * LARGEUR_T / 2 *cos(i) ,
		     LireZoneY(zoneADessiner) + HAUTEUR_T * 0.7 + M_SQRT2 * HAUTEUR_T / 5 *sin(i));
	  glVertex2f(LireZoneX(zoneADessiner) + LARGEUR_T + M_SQRT2 * LARGEUR_T / 2 *cos(i + M_PI/20) ,
		     LireZoneY(zoneADessiner) + HAUTEUR_T * 0.7 + M_SQRT2 * HAUTEUR_T / 5 *sin(i + M_PI/20));
	  couleurContour(zoneADessiner, iGaucheVars->zoneSelectionnee, element->thread.charge, element->thread.priorite);
	  glVertex2f(LireZoneX(zoneADessiner) + LARGEUR_T + M_SQRT2 * LARGEUR_T * 0.625 *cos(i + M_PI/20) ,
		     LireZoneY(zoneADessiner) + HAUTEUR_T * 0.7 + M_SQRT2 * HAUTEUR_T / 4 *sin(i + M_PI/20));
	  glVertex2f(LireZoneX(zoneADessiner) + LARGEUR_T + M_SQRT2 * LARGEUR_T * 0.625 *cos(i) ,
		     LireZoneY(zoneADessiner) + HAUTEUR_T * 0.7 + M_SQRT2 * HAUTEUR_T / 4 *sin(i));
	}
      for(i = 3*M_PI/4; i < 5*M_PI/4; i += M_PI/20)
	{
	  couleurContour(zoneADessiner, iGaucheVars->zoneSelectionnee, element->thread.charge, element->thread.priorite);
	  glVertex2f(LireZoneX(zoneADessiner) + LARGEUR_T + M_SQRT2 * LARGEUR_T * 0.375 *cos(i) ,
		     LireZoneY(zoneADessiner) + HAUTEUR_T * 0.7 + M_SQRT2 * HAUTEUR_T * 0.15 *sin(i));
	  glVertex2f(LireZoneX(zoneADessiner) + LARGEUR_T + M_SQRT2 * LARGEUR_T * 0.375 *cos(i + M_PI/20) ,
		     LireZoneY(zoneADessiner) + HAUTEUR_T * 0.7 + M_SQRT2 * HAUTEUR_T * 0.15 *sin(i + M_PI/20));
	  couleurInterieur(zoneADessiner, iGaucheVars->zoneSelectionnee, element->thread.charge, element->thread.priorite);
	  glVertex2f(LireZoneX(zoneADessiner) + LARGEUR_T + M_SQRT2 * LARGEUR_T / 2 *cos(i + M_PI/20) ,
		     LireZoneY(zoneADessiner) + HAUTEUR_T * 0.7 + M_SQRT2 * HAUTEUR_T / 5 *sin(i + M_PI/20));
	  glVertex2f(LireZoneX(zoneADessiner) + LARGEUR_T + M_SQRT2 * LARGEUR_T / 2 *cos(i) ,
		     LireZoneY(zoneADessiner) + HAUTEUR_T * 0.7 + M_SQRT2 * HAUTEUR_T / 5 *sin(i));
	}
      glEnd();



      glBegin(GL_POLYGON);
      couleurInterieur(zoneADessiner, iGaucheVars->zoneSelectionnee, element->thread.charge, element->thread.priorite);
      glVertex2f(LireZoneX(zoneADessiner) + LARGEUR_T / 2,
                 LireZoneY(zoneADessiner) + HAUTEUR_T * 0.9);

      couleurContour(zoneADessiner, iGaucheVars->zoneSelectionnee, element->thread.charge, element->thread.priorite);
      for(i = -M_PI/4; i <= 3*M_PI/4; i += M_PI/30)
	{
	  glVertex2f(LireZoneX(zoneADessiner) + LARGEUR_T / 2 + M_SQRT2 * LARGEUR_T * 0.125 * cos(i),
		     LireZoneY(zoneADessiner) + HAUTEUR_T * 0.9 + M_SQRT2 * HAUTEUR_T * 0.05 * sin(i));
	}
      glEnd();

      glBegin(GL_POLYGON);
      couleurInterieur(zoneADessiner, iGaucheVars->zoneSelectionnee, element->thread.charge, element->thread.priorite);
      glVertex2f(LireZoneX(zoneADessiner) + LARGEUR_T / 2,
                 LireZoneY(zoneADessiner) + HAUTEUR_T / 10);

      couleurContour(zoneADessiner, iGaucheVars->zoneSelectionnee, element->thread.charge, element->thread.priorite);
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

  iGaucheVars->zonePrincipale->next = NULL;
  iGaucheVars->head =  iGaucheVars->zonePrincipale;
  iGaucheVars->last =  iGaucheVars->zonePrincipale;
  iGaucheVars->zoneSelectionnee =  iGaucheVars->zonePrincipale;

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
      printf("info : initialisation sans alpha (ce sera moins joli)\n");
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

/*Selection Multiple*/
#if 1
zone *insertZoneSelected(zone *headZoneSelectionnee,zone *zoneSelected)
{
	//printf("DEBUG:Insertion d'un elemnt selectionne\n");
	zone *cur;
        if(zoneSelected->posX == 0 || zoneSelected->posY == 0 || abs(zoneSelected->posX) >= 1500 || abs(zoneSelected->posY) >= 1500 ) {
             //deleteRec2(NULL,NULL);
           //printf("DEBUG:pas de zone principale dans l'insertion\n");
		return headZoneSelectionnee;
        }

	if(zoneSelected == NULL ) {
           //printf("DEBUG:zoneselected ???NULL\n");
		return headZoneSelectionnee;
        }
	else{
        if(headZoneSelectionnee == NULL){
		//printf("DEBUG:NULL INSERTED\n");
		headZoneSelectionnee = zoneSelected;
                headZoneSelectionnee->next = NULL;
		return headZoneSelectionnee;
	}
	else{
              //if(headZoneSelectionnee != NULL){
		cur = headZoneSelectionnee;
		while(cur->next!=NULL){
                //printf("%d %d \n",cur->posX,cur->posY);
			cur=cur->next;
		}
                //zoneSelected->next = NULL;
		cur->next = zoneSelected;
                //cur->next->next = NULL;
		zoneSelected->next = NULL;
                //printf("DEBUG:INSERTED\n");
	    //}
            //else return NULL;
         }
       }
         //return cur;
	return headZoneSelectionnee;
}


zone* delete(zone *headZoneSelectionnee,zone *deletedZone)
{
	zone *cur,*prev;
	cur = headZoneSelectionnee;
	prev = NULL;
	if(deletedZone == NULL || headZoneSelectionnee == NULL)
	{
		//printf("DEBUG:Delete NULL\n");
		return NULL;
	}
	while((cur!=NULL) && (cur->posX != deletedZone->posX ||  cur->posY != deletedZone->posY))
	{
		//printf("DEBUG:I search\n");
		prev = cur;
		cur=cur->next;
	}
       //if(iGaucheVars->last == cur)
         //return NULL;
	if((cur != NULL) && (cur->posX == deletedZone->posX &&  cur->posY == deletedZone->posY))
	{
		iGaucheVars->last = cur;
 		//printf("DEBUG:Deleted\n");
		if(prev == NULL){

                //printf("DEBUG:prev NULL\n");
                        if(cur->next != NULL){
                          //printf("DEBUG:supp premier\n");
                          //cur = cur->next;
   			headZoneSelectionnee = cur->next;
                        //cur->next = NULL;
                        //cur->next = NULL;
                        }else{
                        headZoneSelectionnee = iGaucheVars->zonePrincipale;
                        headZoneSelectionnee->next = NULL;
                        //printf("DEBUG:supp seul\n");
                        }
                }
		else{
                        //printf("DEBUG:Action\n");
			prev->next = cur->next;
                        //return prev;
               }
	//free(cur);
        }
	//printf("DEBUG:Nothing was deleted\n");
	return headZoneSelectionnee;
}


gboolean isInside(zone *headZoneSelectionnee,zone *search)
{
	zone *cur;
	cur = headZoneSelectionnee;
	if(search == NULL)
	{
		//printf("DEBUG:Search NULL\n");
		return FALSE;
	}
        if(headZoneSelectionnee == NULL)
	{
		//printf("DEBUG:head NULL\n");
		return FALSE;
	}
	while(cur!=NULL)
	{
		//printf("DEBUG: I search\n");
		if(cur->posX == search->posX &&  cur->posY == search->posY)
		{
 			///printf("DEBUG:I was found\n");
  			return TRUE;
		}
		cur=cur->next;
	}
	//printf("DEBUG:Nothing\n");
	return FALSE;
}

/*********************************************/

static void handleClic(interfaceGaucheVars* iGaucheVarsTmp, int clicX, int clicY) {
  parcours *p;
  int i = 0;
  zone * ZoneCliquee, *tempo ;

  Element* elementSelectionne;
  char *tmp = malloc(20);

  p = TrouverParcours(iGaucheVarsTmp->zonePrincipale, clicX, clicY);
  ZoneCliquee = LireZoneParcours(iGaucheVarsTmp->zonePrincipale, p);
  /* D�placement court -> s�lection */
  if (iGaucheVarsTmp->mousePosClic_state & GDK_CONTROL_MASK )
  {

        //if(iGaucheVarsTmp->last != iGaucheVarsTmp->zonePrincipale && iGaucheVarsTmp->last!= NULL )
           //iGaucheVarsTmp->zoneSelectionnee = insertZoneSelected(iGaucheVarsTmp->zoneSelectionnee, iGaucheVarsTmp->last);

	if( ZoneCliquee == iGaucheVarsTmp->zonePrincipale ){
            printf("DEBUG : Selection de la zone principale\n");
	    return;
        }
	if(!isInside(iGaucheVarsTmp->head, ZoneCliquee)){
	   iGaucheVarsTmp->head = insertZoneSelected(iGaucheVarsTmp->head, ZoneCliquee);
           if(isInside(iGaucheVarsTmp->head, iGaucheVarsTmp->zonePrincipale))
              iGaucheVarsTmp->head  = delete(iGaucheVarsTmp->head , iGaucheVarsTmp->zonePrincipale);
        }else
	   iGaucheVarsTmp->head  = delete(iGaucheVarsTmp->head , ZoneCliquee);

     iGaucheVarsTmp->last = ZoneCliquee;
     iGaucheVarsTmp->zoneSelectionnee = iGaucheVarsTmp->head;
    elementSelectionne = LireElementParcours(iGaucheVarsTmp->bullePrincipale, p);
  }
  else{
  if( ZoneCliquee == iGaucheVarsTmp->zonePrincipale){
    printf("DEBUG : Selection de la zone principale\n");
    iGaucheVarsTmp->zonePrincipale->next = NULL;
    iGaucheVarsTmp->zoneSelectionnee = iGaucheVarsTmp->zonePrincipale;
    iGaucheVarsTmp->head = iGaucheVarsTmp->zonePrincipale;
    iGaucheVarsTmp->last = iGaucheVarsTmp->zonePrincipale;
    return;
  } else {
   ZoneCliquee->next = NULL;
   iGaucheVarsTmp->zoneSelectionnee = ZoneCliquee;
   iGaucheVarsTmp->head = ZoneCliquee;
   iGaucheVarsTmp->last = ZoneCliquee;
   elementSelectionne = LireElementParcours(iGaucheVarsTmp->bullePrincipale, p);
  }
 }//fin de else

  if (elementSelectionne->type==THREAD)
    {
      printf("zone thread\n");
      printf("id du thread : %d\n", GetId(elementSelectionne));
      iGaucheVarsTmp->BulleSelect=NULL;
      iGaucheVarsTmp->ThreadSelect=elementSelectionne;
      // On repositionne les paramétres des ascenseurs et de la zone texte correspondant au données du thread
      gtk_range_set_value (GTK_RANGE(iGaucheVarsTmp->charge),GetCharge(elementSelectionne));
      gtk_range_set_value (GTK_RANGE(iGaucheVarsTmp->priorite),GetPrioriteThread(elementSelectionne));
      gtk_range_set_value (GTK_RANGE(iGaucheVarsTmp->prioritebulle),iGaucheVarsTmp->defprioritebulle);
      printf("debug elementselecTHREAD 1 %s\n",iGaucheVarsTmp->defnom);
      strcpy(tmp, iGaucheVarsTmp->defnom);
      gtk_entry_set_text (GTK_ENTRY(iGaucheVarsTmp->nom),GetNom(elementSelectionne));
      printf("debug elementselecTHREAD 2 %s\n",tmp);
      iGaucheVarsTmp->defnom=tmp;
      // On sauve le thread selectionné en cas de changement des ses valeurs.
      // iGaucheVarsTmp->ThreadSelect=elementSelectionne;
      printf("debug elementselecTHREAD 3 %s\n",iGaucheVarsTmp->defnom);
    }

  if(elementSelectionne==iGaucheVarsTmp->bullePrincipale){

    wprintf(L"Zone principale cliquée\n");
    iGaucheVarsTmp->ThreadSelect=NULL;
    iGaucheVarsTmp->BulleSelect=NULL;
    gtk_range_set_value (GTK_RANGE(iGaucheVarsTmp->charge),iGaucheVarsTmp->defcharge);
    gtk_range_set_value (GTK_RANGE(iGaucheVarsTmp->priorite),iGaucheVarsTmp->defpriorite);
    printf("Debug eleselc 1 %s\n",iGaucheVarsTmp->defnom);
    gtk_entry_set_text (GTK_ENTRY(iGaucheVarsTmp->nom),iGaucheVarsTmp->defnom);
    printf("Debug eleselc 2 %s\n",iGaucheVarsTmp->defnom);
    gtk_range_set_value (GTK_RANGE(iGaucheVarsTmp->prioritebulle),iGaucheVarsTmp->defprioritebulle);
  }

  if(elementSelectionne!=iGaucheVarsTmp->bullePrincipale && elementSelectionne->type==BULLE){

    wprintf(L"Zone bulle cliquée\n");
    printf("id de la bulle : %d\n", GetId(elementSelectionne));
    iGaucheVarsTmp->ThreadSelect=NULL;
    iGaucheVarsTmp->BulleSelect=elementSelectionne;

    gtk_range_set_value (GTK_RANGE(iGaucheVarsTmp->charge),iGaucheVarsTmp->defcharge);
    gtk_range_set_value (GTK_RANGE(iGaucheVarsTmp->priorite),iGaucheVarsTmp->defpriorite);
    gtk_entry_set_text (GTK_ENTRY(iGaucheVarsTmp->nom),iGaucheVarsTmp->defnom);

    gtk_range_set_value (GTK_RANGE(iGaucheVarsTmp->prioritebulle),GetPrioriteBulle(elementSelectionne));
  }


#if 1
tempo = iGaucheVarsTmp->head;
while(tempo != NULL){
 printf("%d %d \n",tempo->posX,tempo->posY);
 tempo = tempo->next;

 i++;
 }

 printf("%d \n",i);
#endif

  EffacerParcours(p);
}

gboolean MouseMove_left_dz(GtkWidget* widget, GdkEventMotion* ev, gpointer data)
{

  parcours *p;
  zone * ZoneCliquee;
  interfaceGaucheVars* iGaucheVarsTmp = (interfaceGaucheVars*)data;
  int clicX, clicY;

  iGaucheVarsTmp->mousePos_left_x = clicX = (ev->x) / iGaucheVarsTmp->echelle;
  iGaucheVarsTmp->mousePos_left_y = clicY = (iGaucheVarsTmp->area_left_y - ev->y) / iGaucheVarsTmp->echelle;

  // Pour test
  iGaucheVarsTmp->mousePosClic_left_x =  clicX;
  iGaucheVarsTmp->mousePosClic_left_y =  clicY;
  iGaucheVarsTmp->mousePosClic_state = ev->state;

  p = TrouverParcours(iGaucheVarsTmp->zonePrincipale, clicX, clicY);
  ZoneCliquee = LireZoneParcours(iGaucheVarsTmp->zonePrincipale, p);
  EffacerParcours(p);
  if(!isInside(iGaucheVarsTmp->head, ZoneCliquee)) {
     handleClic(iGaucheVarsTmp, iGaucheVarsTmp->mousePosClic_left_x, iGaucheVarsTmp->mousePosClic_left_y);
     iGaucheVarsTmp->clic_handled = 1;
  } else {
     iGaucheVarsTmp->clic_handled = 0;
  }

return TRUE;
}





/*!
 *
 *
 * pour cette fonction, le mieux serait de sauvegarder
 *  dans une structure les elements selectionnées
 */
gboolean MouseMove_left_release(GtkWidget* widget, GdkEventMotion* ev, gpointer data)
{
  parcours *p, *pParent, *pAccueil;
  widget = NULL;  // inutilisé
  interfaceGaucheVars* iGaucheVarsTmp = (interfaceGaucheVars*)data;
  zone * ZoneSelectionnee, *ZoneParent, * ZoneAccueil;
  Element* elementSelectionne, * elementParent, * elementAccueil;
  Bulle* bulleParent, * bulleAccueil;
  int clicX, clicY, lacheX, lacheY; /*position du clic et du relachement du clic */

  /* Mise à jour de iGaucheVarsTmp pour connaître les coordonnées du relâchement du clic */
  iGaucheVarsTmp->mousePos_left_x = (ev->x) / iGaucheVarsTmp->echelle;
  iGaucheVarsTmp->mousePos_left_y = (iGaucheVarsTmp->area_left_y - ev->y) / iGaucheVarsTmp->echelle;
  clicX = iGaucheVarsTmp->mousePosClic_left_x;
  clicY = iGaucheVarsTmp->mousePosClic_left_y;
  lacheX = iGaucheVarsTmp->mousePos_left_x;
  lacheY = iGaucheVarsTmp->mousePos_left_y;

  if (abs(clicX - lacheX) < 3 && abs(clicY - lacheY) < 3) {
     if (!iGaucheVarsTmp->clic_handled)
       handleClic(iGaucheVarsTmp, clicX, clicY);
     return TRUE;
   }

/* D�placement long -> d�placement/copie */
  if(!isInside(iGaucheVarsTmp->head, iGaucheVarsTmp->last))
   iGaucheVarsTmp->head = insertZoneSelected(iGaucheVarsTmp->head, iGaucheVarsTmp->last);
  /* CALCUL DES ELEMENTS SELECTIONNES, PARENT ET ACCUEIL */

  /* Voilà l'élément d'accueil */
  pAccueil = TrouverParcours(iGaucheVarsTmp->zonePrincipale, lacheX, lacheY);
  /* wprintf(L"Trace parcours élément Accueil\n");
     traceParcours(pAccueil);*/
  ZoneAccueil = LireZoneParcours(iGaucheVarsTmp->zonePrincipale, pAccueil);
  elementAccueil = LireElementParcours(iGaucheVarsTmp->bullePrincipale, pAccueil);
  bulleAccueil = &elementAccueil->bulle;
  EffacerParcours(pAccueil);

  /* Voilà l'élément sélectionné qu'on va déplacer */
  zone *tmp = iGaucheVarsTmp->head;
 while(tmp != NULL && tmp != iGaucheVarsTmp->zonePrincipale){
  p = TrouverParcours(iGaucheVarsTmp->zonePrincipale, tmp->posX + 1, tmp->posY + 1 );
   //p = TrouverParcours(iGaucheVarsTmp->zonePrincipale, iGaucheVarsTmp->mousePosClic_left_x, iGaucheVarsTmp->mousePosClic_left_y);
  /* wprintf(L"Trace parcours élément à déplacer\n");
     traceParcours(p); */
  ZoneSelectionnee = LireZoneParcours(iGaucheVarsTmp->zonePrincipale, p);
  /* après tests, il s'avère que la ZoneSelectionnee n'est jamais vide */
  if(ZoneSelectionnee == iGaucheVarsTmp->zonePrincipale)
    {
      iGaucheVarsTmp->head = iGaucheVarsTmp->zonePrincipale;
      wprintf(L"Zone sélectionnée bullePrincipale\n");
      return FALSE;
    }
  //iGaucheVarsTmp->zoneSelectionnee =  tmp ;//tmp et ZoneSelectionnee sont les memes
  elementSelectionne = LireElementParcours(iGaucheVarsTmp->bullePrincipale, p);
  EffacerParcours(p);

  /* Voilà l'élément parent de l'élément sélectionné */
  pParent = TrouverParcours(iGaucheVarsTmp->zonePrincipale, LireZoneX(tmp) + 1, LireZoneY(tmp)+1);
  ZoneParent = LireZoneParcours(iGaucheVarsTmp->zonePrincipale, pParent);
  elementParent = LireElementParcours(iGaucheVarsTmp->bullePrincipale, pParent);
  bulleParent = &elementParent->bulle;
  EffacerParcours(pParent);

  wprintf(L"Eléments sélectionné, parent et d'accueil calculés\n");


  /* Ici les tests pour voir si on peut effectuer les déplacements */
  /* Pour test, on regarde si la ZoneSelectionnee est vide */
  if(ZoneSelectionnee == NULL)
    {
      wprintf(L"Zone sélectionnée vide\n");
      return FALSE;
    }

  if (ZoneAccueil == NULL)
    {
      printf("Zone d'accueil vide\n");
      return FALSE;
    }
  if(elementAccueil->type == THREAD)
    {
      wprintf(L"Accueil de type thread, pas de déplacement.\n");
      return FALSE;
    }
  /* On teste pour voir si la Zone de relâchement du clic et celle du clic initial sont identiques */
  /*if(ZoneSelectionnee == ZoneAccueil || ZoneParent == ZoneAccueil)
    {
      wprintf(L"Zone identique, pas de déplacement.\n");
      return FALSE;
    }
*/
  if (elementSelectionne == elementAccueil) {
      wprintf(L"La destination est égale à la source, pas de déplacement.\n");
      return FALSE;
  }
  /* Si l'élément d'accueil est un fils de l'élément sélectionné, on ne fait rien */
  if(appartientElementParent(elementSelectionne, elementAccueil))
    {
      wprintf(L"Zone d'accueil comprise dans la zone sélectionnée, pas de déplacement.\n");
      return FALSE;
    }
  /* Finalement on fait le déplacement */
   /* On teste si le déplacement ou la copie est possible */
  //if (TestCopieDeplacement(iGaucheVarsTmp,clicX, clicY, lacheX, lacheY) == FALSE)
  //return FALSE;


   wprintf(L"Zones différentes, déplacement possible.\n");
   /* SI on appuie sur Control, ça fait une copie, sinon un déplacement */
  if (ev->state & GDK_CONTROL_MASK)
    {
      Copier(iGaucheVarsTmp->bullePrincipale, iGaucheVarsTmp->zonePrincipale,
	     tmp->posX + 1, tmp->posY + 1,
	     ZoneAccueil, elementAccueil, iGaucheVarsTmp);
      printf("DEBUG : Mouse_move_left_release_copy, Copier fait\n");

    }
  else
    {
      Deplacer(iGaucheVarsTmp,
	        tmp->posX + 1, tmp->posY + 1,
	       ZoneAccueil, elementAccueil);
      //enregistrerTmp(); /* pour l'historique */
    }
   printf("MouseP X : %d   MouseP Y : %d\n", tmp->posX + 1, tmp->posY + 1);
   printf("Mouse X : %d   Mouse Y : %d\n",  iGaucheVarsTmp->mousePos_left_x, iGaucheVarsTmp->mousePos_left_y);

  tmp=tmp->next;
 }
  enregistrerTmp(); /* pour l'historique */
  return TRUE;
}


/***************************************************************************/
/*! Copie la sélection vers une autre destination
 *
 *
 *
 */
void Copier(Element * bulleprinc, zone * zoneprinc, int srcX, int srcY, zone * zdest, Element *bdest, interfaceGaucheVars * iGaucheVarsPar)
{
  parcours * psrc;
  zone * zsrc, * zdep;
  Element * bsrc, * bdep;
  /* dest : accueil / src : parent / dep : selectionnee */

  psrc = TrouverParcours(zoneprinc, srcX, srcY);
  zsrc = LireZoneParcoursPartiel(zoneprinc, psrc, LireParcoursTaille(psrc) - 1);
  bsrc = LireElementParcoursPartiel(bulleprinc, psrc, LireParcoursTaille(psrc) - 1);
  /* printf("DEBUG : Copier, Trace parcours élément Parent "); */
  traceParcours(psrc);


  zdep = LireSousZones(zsrc, LirePosition(psrc, LireParcoursTaille(psrc)));
  bdep = GetElement(bsrc, LirePosition(psrc, LireParcoursTaille(psrc)));
  /* wprintf(L"Trace parcours élément à déplacer\n");
     traceParcours(LirePosition(psrc, LireParcoursTaille(psrc))); */

  /* Donc on ajoute l'élément courant (bdep) dans la liste de l'élément de destination (bdest) */
  CopyElement(bdest, zdest, bdep, iGaucheVarsPar, 1);
  /* printf("DEBUG : Copier, CopyElement fait\n"); */

  EffacerParcours(psrc);

  Rearanger(zoneprinc);
  /* printf("DEBUG : Copier, Rearanger fait.\n\n"); */
}

/********************************************************************/
/*! Déplace la sélection vers une autre destination
 * \param iGaucheVars les variables de l'interface gauche
 */
void Deplacer(interfaceGaucheVars * iGaucheVars, int srcX, int srcY, zone * zdest, Element *bdest)
{
  parcours * psrc;
  zone *zoneprinc, * zsrc, * zdep;
  Element * bulleprinc, * bsrc, * bdep;
  /* src : parent / dep : selectionnee / dest : accueil  */

  bulleprinc = iGaucheVars->bullePrincipale;
  zoneprinc = iGaucheVars->zonePrincipale;


  psrc = TrouverParcours(zoneprinc, srcX, srcY);
  zsrc = LireZoneParcoursPartiel(zoneprinc, psrc, LireParcoursTaille(psrc) - 1);
  bsrc = LireElementParcoursPartiel(bulleprinc, psrc, LireParcoursTaille(psrc) - 1);
  /* wprintf(L"DEBUG : Deplacer, Trace parcours élément Parent ");
     traceParcours(psrc); */

  zdep = LireSousZones(zsrc, LirePosition(psrc, LireParcoursTaille(psrc)));
  bdep = GetElement(bsrc, LirePosition(psrc, LireParcoursTaille(psrc)));
  /* wprintf(L"Trace parcours élément à déplacer\n");
     traceParcours(LirePosition(psrc, LireParcoursTaille(psrc))); */


  EnleverSousZones(zsrc, LirePosition(psrc, LireParcoursTaille(psrc)));
  //TranslaterZone(zdep, destX - srcX, destY - srcY);

  /* Donc on ajoute l'élément courant (bdep) dans la liste de l'élément de destination (bdest) */
  AddElement2(bdest, bdep);
  /* printf("DEBUG : Deplacer, AddElement2 fait\n"); */
  /* et on n'oublie pas de supprimer l'ancien élément contenu dans l'élément parent (bsrc) */
  RemoveElement2(bsrc, LirePosition(psrc,LireParcoursTaille(psrc)));
  /* printf("DEBUG : Deplacer, RemoveElement2 fait\n"); */

  AjouterSousZones(zdest, zdep);
  /* printf("DEBUG : Deplacer, AjouterSousZones fait.\n"); */

  EffacerParcours(psrc);

  Rearanger(zoneprinc);
  /* printf("DEBUG : Deplacer, Rearanger fait.\n\n"); */
}

#endif

/*Fin*/



/***************************************************************************/
/*! Met à jour la variable id general en l'incrémentant de 1 à chaque fois
 * qu'une bulle ou un thread est créé et renvoie la valeur de idgeneral.
 *
 *
 */
int SetId(gpointer data)
{
  interfaceGaucheVars* iGaucheVars = (interfaceGaucheVars*)data;
  iGaucheVars->idgeneral++;

  return iGaucheVars->idgeneral;
}



/*Selection Simple*/
#if 0

/* callback de "button_press_event" */
gboolean MouseMove_left_dz(GtkWidget* widget, GdkEventMotion* ev, gpointer data)
{
  parcours *p;
  widget = NULL;  // inutilisé
  interfaceGaucheVars* iGaucheVars = (interfaceGaucheVars*)data;
  zone * nouvelleZoneSelectionnee;
  Element* elementSelectionne;
  char *tmp=malloc(20);

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
  printf("ZoneSelectionnee X : %d Y : %d\n", nouvelleZoneSelectionnee->posX, nouvelleZoneSelectionnee->posY);

  if (elementSelectionne->type==THREAD)
    {
      printf("zone thread\n");
      printf("id du thread : %d\n", GetId(elementSelectionne));
      iGaucheVars->BulleSelect=NULL;
      iGaucheVars->ThreadSelect=elementSelectionne;
      // On repositionne les paramétres des ascenseurs et de la zone texte correspondant au données du thread
      gtk_range_set_value (iGaucheVars->charge,GetCharge(elementSelectionne));
      gtk_range_set_value (iGaucheVars->priorite,GetPrioriteThread(elementSelectionne));
      gtk_range_set_value (iGaucheVars->prioritebulle,iGaucheVars->defprioritebulle);
      printf("debug elementselecTHREAD 1 %s\n",iGaucheVars->defnom);
      strcpy(tmp, iGaucheVars->defnom);
      gtk_entry_set_text (iGaucheVars->nom,GetNom(elementSelectionne));
      printf("debug elementselecTHREAD 2 %s\n",tmp);
      iGaucheVars->defnom=tmp;
      // On sauve le thread selectionné en cas de changement des ses valeurs.
      // iGaucheVars->ThreadSelect=elementSelectionne;
      printf("debug elementselecTHREAD 3 %s\n",iGaucheVars->defnom);
    }

  if(elementSelectionne==iGaucheVars->bullePrincipale){

    wprintf(L"Zone principale cliquée\n");
    iGaucheVars->ThreadSelect=NULL;
    iGaucheVars->BulleSelect=NULL;
    gtk_range_set_value (iGaucheVars->charge,iGaucheVars->defcharge);
    gtk_range_set_value (iGaucheVars->priorite,iGaucheVars->defpriorite);
    printf("Debug eleselc 1 %s\n",iGaucheVars->defnom);
    gtk_entry_set_text (iGaucheVars->nom,iGaucheVars->defnom);
    printf("Debug eleselc 2 %s\n",iGaucheVars->defnom);
    gtk_range_set_value (iGaucheVars->prioritebulle,iGaucheVars->defprioritebulle);
  }

  if(elementSelectionne!=iGaucheVars->bullePrincipale && elementSelectionne->type==BULLE){

    wprintf(L"Zone bulle cliquée\n");
    printf("id de la bulle : %d\n", GetId(elementSelectionne));
    iGaucheVars->ThreadSelect=NULL;
    iGaucheVars->BulleSelect=elementSelectionne;

    gtk_range_set_value (iGaucheVars->charge,iGaucheVars->defcharge);
    gtk_range_set_value (iGaucheVars->priorite,iGaucheVars->defpriorite);
    gtk_entry_set_text (iGaucheVars->nom,(const gchar*)iGaucheVars->defnom);

    gtk_range_set_value (iGaucheVars->prioritebulle,GetPrioriteBulle(elementSelectionne));
  }


  /* pour test
     k++; */
  /* variable globale pour test */
  /*
    wprintf(L"Clic souris détecté %d\n",k);  */

  return TRUE;
}

/*! On vérifie que la sélection n'est pas vide
 *  Le clic gauche doit être pressé sur la sélection
 *  et relaché au dessus d'une
 *  bulle ne faisant pas partie de la sélection
 */


gboolean MouseMove_left_movebt(GtkWidget* widget, GdkEventMotion* ev, gpointer data)
{
  parcours *p;
  widget = NULL;  // inutilisé
  interfaceGaucheVars* iGaucheVars = (interfaceGaucheVars*)data;
  zone * nouvelleZoneSelectionnee;
  Element* elementSelectionne;

  // wprintf(L"déplacement détecté %d\n",i);
  i++;
  return TRUE;
}


/*! Gère les événements liés au relâchement de la souris (copie, déplacement)
 * \param widget
 * \param ev
 * \param data
 * \todo doc
 */
gboolean MouseMove_left_release(GtkWidget* widget, GdkEventMotion* ev, gpointer data)
{

  widget = NULL;  // inutilisé
  interfaceGaucheVars* iGaucheVars = (interfaceGaucheVars*)data;

  int clicX, clicY, lacheX, lacheY; /*position du clic et du relachement du clic */

  /* Mise à jour de iGaucheVars pour connaître les coordonnées du relâchement du clic */
  iGaucheVars->mousePos_left_x = (ev->x) / iGaucheVars->echelle;
  iGaucheVars->mousePos_left_y = (iGaucheVars->area_left_y - ev->y) / iGaucheVars->echelle;
  clicX = iGaucheVars->mousePosClic_left_x; /* inutilisé en fait */
  clicY = iGaucheVars->mousePosClic_left_y; /* inutilisé en fait */
  lacheX = iGaucheVars->mousePos_left_x; /* inutilisé en fait */
  lacheY = iGaucheVars->mousePos_left_y; /* inutilisé en fait */

  /* On teste si le déplacement ou la copie est possible */
  if (TestCopieDeplacement(iGaucheVars) == FALSE)
    return FALSE;

  /* SI on appuie sur Control, ça fait une copie, sinon un déplacement */
  if (ev->state & GDK_CONTROL_MASK)
    {
      Copier(iGaucheVars);
      /* printf("DEBUG : Mouse_move_left_release, Copier fait\n"); */
    }
  else
    {
      Deplacer(iGaucheVars);
      /* wprintf(L"DEBUG : Mouse_move_left_release, Déplacer fait\n"); */
    }
  enregistrerTmp(); /* pour l'historique */
  return TRUE;
}




/***************************************************************************/
/*! Copie la sélection vers une autre destination
 * \param iGaucheVars les variables de l'interface gauche
 */
void Copier(interfaceGaucheVars * iGaucheVars)
{
  parcours * psrc, * pdest;
  zone * zoneprinc, * zsrc, * zdep, * zdest;
  Element * bulleprinc, * bsrc, * bdep, * bdest;
  int srcX, srcY, destX, destY;
  /* dest : accueil / src : parent / dep : selectionnee */

  bulleprinc = iGaucheVars->bullePrincipale;
  zoneprinc = iGaucheVars->zonePrincipale;
  srcX = iGaucheVars->mousePosClic_left_x;
  srcY = iGaucheVars->mousePosClic_left_y;
  destX = iGaucheVars->mousePos_left_x;
  destY = iGaucheVars->mousePos_left_y;

  psrc = TrouverParcours(zoneprinc, srcX, srcY);
  zsrc = LireZoneParcoursPartiel(zoneprinc, psrc, LireParcoursTaille(psrc) - 1);
  bsrc = LireElementParcoursPartiel(bulleprinc, psrc, LireParcoursTaille(psrc) - 1);
  /* wprintf(L"DEBUG : Copier, Trace parcours élément Parent ");
     traceParcours(psrc); */


  zdep = LireSousZones(zsrc, LirePosition(psrc, LireParcoursTaille(psrc)));
  bdep = GetElement(bsrc, LirePosition(psrc, LireParcoursTaille(psrc)));
  /* wprintf(L"Trace parcours élément à déplacer\n");
     traceParcours(LirePosition(psrc, LireParcoursTaille(psrc))); */

  pdest = TrouverParcoursBulle(bulleprinc, zoneprinc, destX, destY);
  zdest = LireZoneParcours(zoneprinc, pdest);
  bdest = LireElementParcours(bulleprinc, pdest);
  /* wprintf(L"DEBUG : Copier, Trace parcours élément Accueil ");
     traceParcours(pdest); */

  /* Donc on ajoute l'élément courant (bdep) dans la liste de l'élément de destination (bdest) */
  CopyElement(bdest, zdest, bdep, iGaucheVars, 1);
  /* printf("DEBUG : Copier, CopyElement fait\n"); */

  EffacerParcours(psrc);
  EffacerParcours(pdest);

  Rearanger(zoneprinc);
  /* printf("DEBUG : Copier, Rearanger fait.\n\n"); */
}

/********************************************************************/
/*! Déplace la sélection vers une autre destination
 * \param iGaucheVars les variables de l'interface gauche
 */
void Deplacer(interfaceGaucheVars * iGaucheVars)
{
  parcours * psrc, * pdest;
  zone *zoneprinc, * zsrc, * zdep, * zdest;
  Element * bulleprinc, * bsrc, * bdep, * bdest;
  int srcX, srcY, destX, destY;
  /* src : parent / dep : selectionnee / dest : accueil  */

  bulleprinc = iGaucheVars->bullePrincipale;
  zoneprinc = iGaucheVars->zonePrincipale;
  srcX = iGaucheVars->mousePosClic_left_x;
  srcY = iGaucheVars->mousePosClic_left_y;
  destX = iGaucheVars->mousePos_left_x;
  destY = iGaucheVars->mousePos_left_y;

  psrc = TrouverParcours(zoneprinc, srcX, srcY);
  zsrc = LireZoneParcoursPartiel(zoneprinc, psrc, LireParcoursTaille(psrc) - 1);
  bsrc = LireElementParcoursPartiel(bulleprinc, psrc, LireParcoursTaille(psrc) - 1);
  /* wprintf(L"DEBUG : Deplacer, Trace parcours élément Parent ");
     traceParcours(psrc); */

  zdep = LireSousZones(zsrc, LirePosition(psrc, LireParcoursTaille(psrc)));
  bdep = GetElement(bsrc, LirePosition(psrc, LireParcoursTaille(psrc)));
  /* wprintf(L"Trace parcours élément à déplacer\n");
     traceParcours(LirePosition(psrc, LireParcoursTaille(psrc))); */


  pdest = TrouverParcoursBulle(bulleprinc, zoneprinc, destX, destY);
  zdest = LireZoneParcours(zoneprinc, pdest);
  bdest = LireElementParcours(bulleprinc, pdest);
  /* wprintf(L"DEBUG : Deplacer, Trace parcours élément Accueil ");
     traceParcours(pdest); */

  EnleverSousZones(zsrc, LirePosition(psrc, LireParcoursTaille(psrc)));
  TranslaterZone(zdep, destX - srcX, destY - srcY);

  /* Donc on ajoute l'élément courant (bdep) dans la liste de l'élément de destination (bdest) */
  AddElement2(bdest, bdep);
  /* printf("DEBUG : Deplacer, AddElement2 fait\n"); */
  /* et on n'oublie pas de supprimer l'ancien élément contenu dans l'élément parent (bsrc) */
  RemoveElement2(bsrc, LirePosition(psrc,LireParcoursTaille(psrc)));
  /* printf("DEBUG : Deplacer, RemoveElement2 fait\n"); */

  AjouterSousZones(zdest, zdep);
  /* printf("DEBUG : Deplacer, AjouterSousZones fait.\n"); */

  EffacerParcours(psrc);
  EffacerParcours(pdest);

  Rearanger(zoneprinc);
  /* printf("DEBUG : Deplacer, Rearanger fait.\n\n"); */
}


/*! Vérifie la possibilité d'une copie ou d'un déplacement
 * Retourne TRUE si la copie ou le déplacement est possible, FALSE sinon.
 * \param iGaucheVars les variables de l'interface gauche, nécessaires pour connaître les coordonnes du clic.
 */
gboolean TestCopieDeplacement(interfaceGaucheVars * iGaucheVars)
{
  parcours *p, *pParent, *pAccueil;
  zone * ZoneSelectionnee, *ZoneParent, * ZoneAccueil;
  Element* elementSelectionne, * elementParent, * elementAccueil;
  Bulle* bulleParent, * bulleAccueil;

  /* CALCUL DES ELEMENTS SELECTIONNES, PARENT ET ACCUEIL */
  /* Voilà l'élément sélectionné qu'on va déplacer */
  p = TrouverParcours(iGaucheVars->zonePrincipale, iGaucheVars->mousePosClic_left_x, iGaucheVars->mousePosClic_left_y);
  /* wprintf(L"Trace parcours élément à déplacer\n");
     traceParcours(p); */
  ZoneSelectionnee = LireZoneParcours(iGaucheVars->zonePrincipale, p);
  iGaucheVars->zoneSelectionnee = ZoneSelectionnee;
  elementSelectionne = LireElementParcours(iGaucheVars->bullePrincipale, p);
  EffacerParcours(p);

  /* Voilà l'élément parent de l'élément sélectionné */
  pParent = TrouverParcours(iGaucheVars->zonePrincipale, LireZoneX(iGaucheVars->zoneSelectionnee) + 1, LireZoneY(iGaucheVars->zoneSelectionnee)+1);
  /* wprintf(L"Trace parcours élément Parent\n");
     traceParcours(pParent); */
  ZoneParent = LireZoneParcours(iGaucheVars->zonePrincipale, pParent);
  elementParent = LireElementParcours(iGaucheVars->bullePrincipale, pParent);
  bulleParent = &elementParent->bulle;
  EffacerParcours(pParent);

  /* Voilà l'élément d'accueil */
  pAccueil = TrouverParcours(iGaucheVars->zonePrincipale, iGaucheVars->mousePos_left_x, iGaucheVars->mousePos_left_y);
  /* wprintf(L"Trace parcours élément Accueil\n");
     traceParcours(pAccueil); */
  ZoneAccueil = LireZoneParcours(iGaucheVars->zonePrincipale, pAccueil);
  elementAccueil = LireElementParcours(iGaucheVars->bullePrincipale, pAccueil);
  bulleAccueil = &elementAccueil->bulle;
  EffacerParcours(pAccueil);

  /* wprintf(L"Eléments sélectionné, parent et d'accueil calculés\n"); */


  /* Ici les tests pour voir si on peut effectuer les déplacements */
  /* Pour test, on regarde si la ZoneSelectionnee est vide */
  if(ZoneSelectionnee == NULL)
    {
      /* wprintf(L"Zone sélectionnée vide\n"); */
      return FALSE;
    }
  /* après tests, il s'avère que la ZoneSelectionnee n'est jamais vide */
  if(ZoneSelectionnee == iGaucheVars->zonePrincipale)
    {
      /* Quand on est dans la bulle principale, on peut changer les valeurs par défaut pour les bulles et les threads */
      gtk_range_set_value (iGaucheVars->charge,iGaucheVars->defcharge);
      gtk_range_set_value (iGaucheVars->priorite,iGaucheVars->defpriorite);
      gtk_entry_set_text (iGaucheVars->nom,iGaucheVars->defnom);
      gtk_range_set_value (iGaucheVars->prioritebulle,iGaucheVars->defprioritebulle);
      /* wprintf(L"Zone sélectionnée bullePrincipale\n"); */
      return FALSE;
    }
  if (ZoneAccueil == NULL)
    {
      /* printf("Zone d'accueil vide\n"); */
      return FALSE;
    }
  if(elementAccueil->type == THREAD)
    {
      /* wprintf(L"Accueil de type thread, pas de déplacement.\n"); */
      return FALSE;
    }
  /* On teste pour voir si la Zone de relâchement du clic et celle du clic initial sont identiques */
  if(ZoneSelectionnee == ZoneAccueil || ZoneParent == ZoneAccueil)
    {
      /* wprintf(L"Zone identique, pas de déplacement.\n"); */
      return FALSE;
    }
  /* Si l'élément d'accueil est un fils de l'élément sélectionné, on ne fait rien */
  if(appartientElementParent(elementSelectionne, elementAccueil))
    {
      /* wprintf(L"Zone d'accueil comprise dans la zone sélectionnée, pas de déplacement.\n"); */
      return FALSE;
    }
  /* Finalement on fait le déplacement */
  /*  wprintf(L"Zones différentes, déplacement possible.\n"); */

  return TRUE;
}
#endif
/*Fin*/
