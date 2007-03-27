
#ifndef _SAVE_H_
#define _SAVE_H_

#include "bulle.h"
#include <libxml/tree.h>
#include <libxml/parser.h>

xmlNodePtr creerBulleXml(Element * element);
xmlNodePtr creerThreadXml(Element * element);
void parcourirBullesXml(Element * bulle, xmlNodePtr noeud);

void parcourirDocXml(Element * element, xmlNodePtr noeud);

void lectureNoeud (xmlNodePtr node);


#endif
