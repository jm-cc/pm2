#ifndef INTERFACEGAUCHEDESSIN_H
#define INTERFACEGAUCHEDESSIN_H

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

typedef struct interfaceGaucheVars_tag
{
/*       Chrono* time; */

      int area_left_x;
      int area_left_y;
      int mousePos_left_x;
      int mousePos_left_y;
      GtkWidget *interfaceGauche;
      GtkWidget *drawzone_left;
      Element *bullePrincipale;
      zone *zonePrincipale;
      zone *zoneSelectionnee;
      float echelle;
}interfaceGaucheVars;

void       make_left_drawable_zone(interfaceGaucheVars *iGaucheVars);

void       OnPositionChange(GtkWidget* pWidget, gpointer data);
gboolean   Redraw_left_dz(gpointer iGaucheVars);
void       Realize_left_dz(GtkWidget* widget, gpointer data);
gboolean   Reshape_left_dz(GtkWidget* widget, GdkEventConfigure* ev, gpointer anim_parm);
gboolean   MouseMove_left_dz(GtkWidget* widget, GdkEventMotion* ev, gpointer anim_parm);


void couleurContour(zone* zoneADessiner, zone* zoneSelectionnee);
void couleurInterieur(zone* zoneADessiner, zone* zoneSelectionnee);

void DessinerTout(interfaceGaucheVars *iGaucheVars);

void Dessiner(interfaceGaucheVars *iGaucheVars, zone * zonePrincipale);

void TracerZone(interfaceGaucheVars *iGaucheVars, zone* zoneADessiner);

void Clear();


#endif
