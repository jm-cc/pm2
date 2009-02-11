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

#include "zone.h"
#include "rearangement.h"


zone * CreerZone(int x, int y, int l, int h)
{
  zone * ptr;
  ptr = malloc(sizeof(zone));

  /* printf("CreerZone %p\n",ptr); */

  ptr->posX = x;
  ptr->posY = y;
  ptr->largeur = l;
  ptr->hauteur = h;
  ptr->sous_zones = CreerListeZones();
  return ptr;
}

int LireZoneX(zone * z)
{
  return z->posX;
}

int LireZoneY(zone * z)
{
  return z->posY;
}

int LireZoneLargeur(zone * z)
{
  return z->largeur;
}

int LireZoneHauteur(zone * z)
{
  return z->hauteur;
}

zone * LireSousZones(zone * z, int position)
{
  /*    printf("sous zone %d\n", position); */
  return LireListeZones(z->sous_zones, position);
}

void ChangerZoneX(zone * z, int x)
{
  z->posX = x;
}

void ChangerZoneY(zone * z, int y)
{
  z->posY = y;
}

void ChangerZoneLargeur(zone * z, int l)
{
  z->largeur = l;
}

void ChangerZoneHauteur(zone * z, int h)
{
  z->hauteur = h;
}

void AjouterSousZones(zone * conteneur, zone * contenu)
{
  /* printf("AjouterSousZones %p %p\n", conteneur, contenu); */
  AjouterListeZones(conteneur->sous_zones, contenu);
}

int EstDansZone(zone * z, int x, int y)
{
  return (x >= z->posX) &&
    (x <= z->posX + z->largeur) &&
    (y >= z->posY)&&(y <= z->posY + z->hauteur);
}

int EstDansSousZones(zone * z, int x, int y)
{
  return EstDansListeZones(z->sous_zones, x, y);
}

void TranslaterZone(zone * z, int v_x, int v_y)
{
  z->posX += v_x;
  z->posY += v_y;
  TranslaterListeZones(z->sous_zones, v_x, v_y);
}

void EnleverSousZones(zone * z, int position)
{
  EnleverListeZones(z->sous_zones, position);
}

void EffacerSousZones(zone * z, int position)
{
  EffacerListeZonesIeme(z->sous_zones, position);
}

void EffacerZone(zone * z)
{
  EffacerListeZones(z->sous_zones);
  free(z);
}

liste_zones * CreerListeZones()
{
  liste_zones * ptr;
  ptr = malloc(sizeof(liste_zones));
  ptr->debut = NULL;
  ptr->fin = NULL;
  return ptr;
}

void AjouterListeZones(liste_zones * l, zone * z)
{
  maillon_zone * ptr;
  ptr = malloc(sizeof(maillon_zone));
  ptr->tete = z;
  ptr->reste = NULL;
  if (l->fin != NULL)
    l->fin->reste = ptr;
  else
    l->debut = ptr;
  l->fin = ptr;
}

int EstVideListeZones(liste_zones * l)
{
  return l->debut == NULL;
}

zone * LireMaillon(maillon_zone * m, int position)
{
  if (position == 1)
    return m->tete;
  if (m->reste != NULL)
    return LireMaillon(m->reste, position - 1);
  return NULL;
}

zone * LireListeZones(liste_zones * l, int position)
{
  if ((l->debut == NULL)||(position <= 0))
    return NULL;
  return LireMaillon(l->debut, position);
}

int EstDansMaillon(maillon_zone * m, int x, int y)
{
  int i;
  if (EstDansZone(m->tete, x, y))
    return 1;
  if (m->reste == NULL)
    return 0;
  i = EstDansMaillon(m->reste, x, y);
  if (i == 0)
    return 0;
  return i + 1;
}

int EstDansListeZones(liste_zones * l, int x, int y)
{
  if (l->debut == NULL)
    return 0;
  return EstDansMaillon(l->debut, x, y);
}

void TranslaterMaillon(maillon_zone * m, int v_x, int v_y)
{
  TranslaterZone(m->tete, v_x, v_y);
  if (m->reste != NULL)
    TranslaterMaillon(m->reste, v_x, v_y);
}

void TranslaterListeZones(liste_zones * l, int v_x, int v_y)
{
  if (l->debut == NULL)
    return;
  TranslaterMaillon(l->debut, v_x, v_y);
}

maillon_zone * AtteindreMaillon(maillon_zone * m, int position)
{
  if (m->reste != NULL)
    {
      if (position == 1)
	return m;
      return AtteindreMaillon(m->reste, position - 1);
    }
  return NULL;
}

void EnleverListeZones(liste_zones * l, int position)
{
  maillon_zone * m, * m1;
  if ((l->debut == NULL)||(position <= 0))
    return;
  switch (position){
  case 1:
    m = l->debut;
    l->debut = m->reste;
    if (l->fin == m)
      l->fin = NULL;
    free(m);
    break;
  case 2:
    m = l->debut->reste;
    if (m != NULL)
      {
	l->debut->reste = m->reste;
	if (l->fin == m)
	  l->fin = l->debut;
	free(m);
      }
    break;
  default:
    m = AtteindreMaillon(l->debut, position - 1);
    if (m != NULL)
      {
	m1 = m->reste;
	if(m1 != NULL)
	  {
	    m->reste = m1->reste;
	    if (l->fin == m1)
	      l->fin = m;
	    free(m1);
	  }
      }
    break;
  }
}

void EffacerListeZonesIeme(liste_zones * l, int position)
{
  maillon_zone * m, * m1;
  if ((l->debut == NULL)||(position <= 0))
    return;
  switch (position){
  case 1:
    m = l->debut;
    l->debut = m->reste;
    EffacerZone(m->tete);
    if (l->fin == m)
      l->fin = NULL;
    free(m);
    break;
  case 2:
    m = l->debut->reste;
    if (m != NULL)
      {
	l->debut->reste = m->reste;
	EffacerZone(m->tete);
	if (l->fin == m)
	  l->fin = l->debut;
	free(m);
      }
    break;
  default:
    m = AtteindreMaillon(l->debut, position - 1);
    if (m != NULL)
      {
	m1 = m->reste;
	if(m1 != NULL)
	  {
	    m->reste = m1->reste;
	    EffacerZone(m1->tete);
	    if (l->fin == m1)
	      l->fin = m;
	    free(m1);
	  }
      }
    break;
  }
}

void EffacerMaillon(maillon_zone * m)
{
  EffacerZone(m->tete);
  if (m->reste != NULL)
    EffacerMaillon(m->reste);
  free(m);
}

void EffacerListeZones(liste_zones * l)
{
  if (l->debut != NULL)
    EffacerMaillon(l->debut);
  free(l);
}

parcours * CreerParcours()
{
  parcours * ptr;
  ptr = malloc(sizeof(parcours));
  ptr->taille = 5;
  ptr->tab = malloc(ptr->taille * sizeof(int));
  ptr->nb = 0;
  return ptr;
}

void AjouterParcoursPosition(parcours * p, int position)
{
  if (p->taille <= p->nb)
    {
      p->taille *= 2;
      p->tab = realloc(p->tab,p->taille *sizeof(int));
    }
  p->tab[p->nb] = position;
  ++p->nb;
}

int LireParcoursTaille(parcours * p)
{
  return p->nb;
}

int LirePosition(parcours * p, int place)
{
  if ((place <= 0)||(place > p->nb))
    return 0;
  return p->tab[place-1];
}

void EffacerParcours(parcours * p)
{
  free(p->tab);
  free(p);
}

/*! A partir d'une position d'une zone, la fonction retourne un pointeur de parcours.
 * \param zone *
 * \param x
 * \param y
 * \return parcours *
 *
 */
parcours * TrouverParcours(zone * z, int x, int y)
{
  parcours * p;
  int i;
  p = CreerParcours();
  for(i = EstDansSousZones(z, x, y); i != 0; i = EstDansSousZones(z, x, y))
    {
      AjouterParcoursPosition(p, i);
      z = LireSousZones(z, i);
    }
  return p;
}

parcours * TrouverParcoursBulle(Element * b, zone * z, int x, int y)
{
  parcours * p;
  int i;
  p = CreerParcours();
  for(i = EstDansSousZones(z, x, y), b = GetElement(b, i);
      ((i != 0)&&(GetTypeElement(b) == BULLE));
      i = EstDansSousZones(z, x, y), b = GetElement(b, i))
    {
      AjouterParcoursPosition(p, i);
      z = LireSousZones(z, i);
    }
  return p;
}

zone * LireZoneParcoursPartiel(zone * z, parcours * p, int i)
{
  int j;
  for(j = 1; j <= i; ++j)
    z = LireSousZones(z, LirePosition(p, j));
  return z;
}

Element * LireElementParcoursPartiel(Element * bulle, parcours * p, int i)
{
  int j;
  for(j = 1; j <= i; ++j)
    bulle = GetElement(bulle, LirePosition(p, j));
  return bulle;
}

zone * LireZoneParcours(zone * z, parcours * p)
{
  return LireZoneParcoursPartiel(z, p, LireParcoursTaille(p));
}

Element * LireElementParcours(Element * bulle, parcours * p)
{
  return LireElementParcoursPartiel(bulle, p, LireParcoursTaille(p));
}

void CreerZoneThread(Element * bulleprinc, zone * zoneprinc, int x, int y)
{
  parcours * p;
  zone * z1, * z2;
  Element * b, * t;

  p = TrouverParcoursBulle(bulleprinc, zoneprinc, x, y);
  z1 = LireZoneParcours(zoneprinc, p);
  b = LireElementParcours(bulleprinc, p);

  t = CreateThread(0,
		   0,
		   "nomdethread",
		   CHARGE);
  AddElement(b, t);
  z2 = CreerZone(x, y, LARGEUR_T, HAUTEUR_T);
  AjouterSousZones(z1, z2);

  EffacerParcours(p);
}


void CreerZoneBulle(Element * bulleprinc, zone * zoneprinc, int x, int y)
{
  parcours * p;
  zone * z1, * z2;
  Element * b1, * b2;

  p = TrouverParcoursBulle(bulleprinc, zoneprinc, x, y);
  z1 = LireZoneParcours(zoneprinc, p);
  b1 = LireElementParcours(bulleprinc, p);

  b2 = CreateBulle(0, 0);
  AddElement(b1, b2);
  z2 = CreerZone(x, y, LARGEUR_B, HAUTEUR_B);
  AjouterSousZones(z1, z2);

  EffacerParcours(p);
}



void ReDimensionner(Element * bulleprinc, zone * zoneprinc, int x, int y, int newx, int newy)
{
  parcours * p;
  zone * z;

  p = TrouverParcoursBulle(bulleprinc, zoneprinc, x, y);
  z = LireZoneParcours(zoneprinc, p);
  ChangerZoneLargeur(z, newx - LireZoneX(z));
  ChangerZoneHauteur(z, newy - LireZoneY(z));

  EffacerParcours(p);
}


/*! Effacer
 *
 *
 */
void Effacer(Element * bulleprinc, zone * zoneprinc, int x, int y)
{
  parcours * p;
  zone * z;
  Element * b;
  /* printf("Effacer\n"); */
  p = TrouverParcours(zoneprinc, x, y);
  z = LireZoneParcoursPartiel(zoneprinc, p , LireParcoursTaille(p) - 1);
  b = LireElementParcoursPartiel(bulleprinc, p , LireParcoursTaille(p) - 1);
  EffacerSousZones(z, LirePosition(p, LireParcoursTaille(p)));
  RemoveElementOnCascade(b, LirePosition(p, LireParcoursTaille(p)));
  EffacerParcours(p);
}

void traceZone(zone * zone, int level, int numero)
{
  int i;
  maillon_zone * ptr = zone->sous_zones->debut;

  for (i = 0; i < level; i++)
    printf("  ");

  printf("%d ", numero);

  printf("- zone %p - l %d - h %d - x %d -y %d\n",zone,
	 LireZoneLargeur(zone),
	 LireZoneHauteur(zone),
	 LireZoneX(zone),
	 LireZoneY(zone));

  i =1;

  while (ptr != NULL)
    {
      traceZone(ptr->tete,level + 1, i);
      ptr = ptr->reste;
      i ++;
    }
  return;
}

/*! Affiche la trace du parcours
 *
 */
void traceParcours(parcours *p)
{
  int i;

  for(i = 0; i< p->nb; i++)
    {
      printf("%d ",p->tab[i]);
    }
  printf("\n");

  return;
}

