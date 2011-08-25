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
  guint mousePosClic_state;
  int clic_handled;
  int mousePos_left_x;
  int mousePos_left_y;

  int idgeneral; // id pour les bulles et les threads, correspond à l'id maximum dans la configuration actuelle

  char chemin[STRING_BUFFER_SIZE]; // chemin du fichier de sauvegarde

  GtkWidget *interfaceGauche;
  GtkWidget *drawzone_left;

  GtkWidget *charge; // Champ correspondant à l'ascenseur charge
  GtkWidget *priorite;// Champ correspondant à l ascenseur priorite
  GtkWidget *nom;// Champ correspondant à la zone texte nom
  GtkWidget *prioritebulle;// Champ correspondant à l'ascenseur priorité bulle

  int defcharge; // Champ correspondant à la valeur de charge d'un thread par défaut
  int defpriorite;// Champ correspondant à la valeur de priorite d'un thread par défaut
  const char *defnom;// Champ correspondant à la zone texte nom par défaut
  int defprioritebulle;// Champ correspondant à la priorité d'une bulle par défaut

  Element *ThreadSelect; // Thread selectionné
  Element *BulleSelect; // Bulle selectionnée

  Element *bullePrincipale;
  zone *zonePrincipale;
  zone *zoneSelectionnee;
  zone * head;/* la liste des elements selectionnes*/
  zone * last;/* le dernier element selectionne*/
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
gboolean TestCopieDeplacement(interfaceGaucheVars * iGaucheVars);

void       DessinerTout(interfaceGaucheVars *iGaucheVars);

void       Dessiner(interfaceGaucheVars *iGaucheVars, zone * zonePrincipale);

void       TracerZone(interfaceGaucheVars *iGaucheVars, zone* zoneADessiner);

void       Clear();

int SetId(gpointer data);
/*Selection Simple */
#if 0
void Copier(interfaceGaucheVars * iGaucheVars);
void Deplacer(interfaceGaucheVars * iGaucheVars);
#endif
/*Fin */


/*Selection Multiple */
#if 1
void Copier(Element * bulleprinc, zone * zoneprinc, int srcX, int srcY, zone * zdest, Element *bdest, interfaceGaucheVars * iGaucheVarsPar);
void Deplacer(interfaceGaucheVars * iGaucheVars, int srcX, int srcY, zone * zdest, Element *bdest);
#endif
/*Fin*/
zone *insertZoneSelected(zone *headZoneSelectionnee,zone *zoneSelected);
zone* delete(zone *headZoneSelectionnee,zone *deletedZone);
gboolean isInside(zone *headZoneSelectionnee,zone *search);
#endif
