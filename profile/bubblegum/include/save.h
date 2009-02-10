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

#ifndef _SAVE_H_
#define _SAVE_H_

#include "bulle.h"
#include "zone.h"

#include <libxml/tree.h>
#include <libxml/parser.h>

xmlNodePtr creerBulleXml(Element * element);
xmlNodePtr creerThreadXml(Element * element);

/* Sauvegarde */
void parcourirBullesXml(Element * bulle, xmlNodePtr noeud);
int enregistrerXml(const char* chemin, Element * racine);

/* Chargement */
void parcourirArbreXml(xmlNodePtr onode, Element * bulleAccueil, zone * z_conteneur);
int chargerXml(const char * Chemin);

#endif

