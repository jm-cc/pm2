/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2008 "the PM2 team" (see AUTHORS file)
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

#ifndef ZONE_H
#define ZONE_H

#include <stdlib.h>
#include "bulle.h"

#define CHARGE 50
#if 1
#define LARGEUR_T 20
#define HAUTEUR_T 50
#define LARGEUR_B 50
#define HAUTEUR_B 50
#define MARGE 25
#else
#define LARGEUR_T 40
#define HAUTEUR_T 100
#define LARGEUR_B 100
#define HAUTEUR_B 100
#define MARGE 50
#endif

/*!
 *
 *
 */
typedef struct _zone
{
  int posX;
  int posY;
  int largeur;
  int hauteur;
  struct _zone * next;
  struct _liste_zones * sous_zones;
} zone;

/*!
 *
 *
 */
typedef struct _maillon_zone
{
  struct _zone * tete;
  struct _maillon_zone * reste;
} maillon_zone;

/*!
 *
 *
 */
typedef struct _liste_zones
{
  struct _maillon_zone * debut;
  struct _maillon_zone * fin;
} liste_zones;

/*!
 *
 *
 */
typedef struct _parcours
{
  int taille;
  int nb;
  int * tab;
} parcours;

zone * CreerZone(int x, int y, int l, int h);
int LireZoneX(zone * z);
int LireZoneY(zone * z);
int LireZoneLargeur(zone * z);
int LireZoneHauteur(zone * z);
zone * LireSousZones(zone * z, int position);
void ChangerZoneX(zone * z, int x);
void ChangerZoneY(zone * z, int y);
void ChangerZoneLargeur(zone * z, int l);
void ChangerZoneHauteur(zone * z, int h);
void AjouterSousZones(zone * conteneur, zone * contenu);
int EstDansZone(zone * z, int x, int y);
int EstDansSousZones(zone * z, int x, int y);
void TranslaterZone(zone * z, int v_x, int v_y);
void EnleverSousZones(zone * z, int position);
void EffacerSousZones(zone * z, int position);
void EffacerZone(zone * z);

liste_zones * CreerListeZones();
void AjouterListeZones(liste_zones * l, zone * z);
int EstVideListeZones(liste_zones * l);
zone * LireListeZones(liste_zones * l, int position);
int EstDansListeZones(liste_zones * l, int x, int y);
void TranslaterListeZones(liste_zones * l, int v_x, int v_y);
void EnleverListeZones(liste_zones * l, int position);
void EffacerListeZonesIeme(liste_zones * l, int position);
void EffacerListeZones(liste_zones * l);

parcours * CreerParcours();
void AjouterParcoursPosition(parcours * p, int position);
int LireParcoursTaille(parcours * p); 
int LirePosition(parcours * p, int place);
void EffacerParcours(parcours * p);

parcours * TrouverParcours(zone * z, int x, int y);
parcours * TrouverParcoursBulle(Element * b, zone * z, int x, int y);
zone * LireZoneParcoursPartiel(zone * z, parcours * p, int i);
Element * LireElementParcoursPartiel(Element * bulle, parcours * p, int i);
zone * LireZoneParcours(zone * z, parcours * p);
Element * LireElementParcours(Element * bulle, parcours * p);

void CreerZoneThread(Element * bulleprinc, zone * zoneprinc, int x, int y);
void CreerZoneBulle(Element * bulleprinc, zone * zoneprinc, int x, int y);
void ReDimensionner(Element * bulleprinc, zone * zoneprinc, int x, int y, int newx, int newy);
void Effacer(Element * bulleprinc, zone * zoneprinc, int x, int y);

void traceZone(zone * zone, int level, int numero);
void traceParcours(parcours *p);



#endif
