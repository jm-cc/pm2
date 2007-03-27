#ifndef INTERFACEGAUCHEDESSIN_H
#define INTERFACEGAUCHEDESSIN_H

#include <wchar.h>
#include <math.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include <gtk/gtkgl.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <stdio.h>

#include "actions.h"
#include "polices.h"
#include "zone.h"
#include "rearangement.h"

#define MODE_AUTO_ON 1
#define MODE_AUTO_OFF 0

typedef struct interfaceGaucheVars_tag
{
  int mode_auto;
  int area_left_x;
  int area_left_y;
  int mousePosClic_left_x;
  int mousePosClic_left_y;
  int mousePos_left_x;
  int mousePos_left_y;
  int idThreadMax;  // Cette valeur est incrémentée à chaque fois qu'un thread est créé
  int idBulleMax; // Cette valeur est incrémentée à chaque fois qu'une bulle est créée
  GtkWidget *interfaceGauche;
  GtkWidget *drawzone_left;

  GtkWidget *charge; // Champs correspondant a l ascenseur charge
  GtkWidget *priorite;// Champs correspondant a l ascenseur priorite
  GtkWidget *nom;// Champs correspondant a la zone texte nom

  GtkWidget *ThreadSelect;

  Element *bullePrincipale;
  zone *zonePrincipale;
  zone *zoneSelectionnee;
  float echelle;
} interfaceGaucheVars;

void       make_left_drawable_zone(interfaceGaucheVars *iGaucheVars);

void       OnPositionChange(GtkWidget* pWidget, gpointer data);
gboolean   Redraw_left_dz(gpointer iGaucheVars);
void       Realize_left_dz(GtkWidget* widget, gpointer data);
gboolean   Reshape_left_dz(GtkWidget* widget, GdkEventConfigure* ev, gpointer anim_parm);
gboolean   MouseMove_left_dz(GtkWidget* widget, GdkEventMotion* ev, gpointer anim_parm);
gboolean   MouseMove_left_movebt(GtkWidget* widget, GdkEventMotion* ev, gpointer data);
gboolean   MouseMove_left_release(GtkWidget* widget, GdkEventMotion* ev, gpointer data);

void       couleurContour(zone* zoneADessiner, zone* zoneSelectionnee);
void       couleurInterieur(zone* zoneADessiner, zone* zoneSelectionnee);

void       DessinerTout(interfaceGaucheVars *iGaucheVars);

void       Dessiner(interfaceGaucheVars *iGaucheVars, zone * zonePrincipale);

void       TracerZone(interfaceGaucheVars *iGaucheVars, zone* zoneADessiner);

void       Clear();



#endif
