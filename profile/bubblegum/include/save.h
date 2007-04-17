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
int enregistrerXml(const char* chemin);//, Element * racine);

/* Chargement */
void parcourirArbreXml(xmlNodePtr onode, Element * bulleAccueil, zone * z_conteneur);
int chargerXml(const char * Chemin);

#endif

