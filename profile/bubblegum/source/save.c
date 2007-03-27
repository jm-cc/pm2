/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2007 Rigann LUN <mailto:lun@enseirb.fr>
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
#if 1
#include "save.h"
#include "bulle.h"
#include <stdio.h>

#define TAILLE_DU_BUFFER 8


/*! Creer un noeud de type "bulle"
 *  Cette fonction récupère directement l'attribut priorité de la bulle
 *  \param Bulle * bulle
 */
xmlNodePtr creerBulleXml(Element * element) {
  //Bulle bulle = element->bulle;
  xmlNodePtr noeud_bulle;
  //  xmlChar buffer[TAILLE_DU_BUFFER];

  /* récupération des attributs */
  char priorite[16]; 
  snprintf(priorite, sizeof(priorite), "%d", GetPrioriteBulle(element));
  
  
  printf("DEBUG : priorite %s ->*<-\n", priorite);

  /* Création du noeud "bulle" */
  noeud_bulle = xmlNewNode(NULL, "bulle");

  if(noeud_bulle == NULL) {
    printf("DEBUG : erreur dans la sauvegarde lors creation n b\n");
    return NULL;
  }
  
  /* Ajout de son attribut */
 #if 1
  if (xmlSetProp(noeud_bulle, "priorite", &priorite) == NULL) {
    xmlFreeNode(noeud_bulle);
    printf("DEBUG : erreur dans la sauvegarde lors priorite bulle\n");
    return NULL;
  }
 #endif 
  return noeud_bulle;
}


/*! Creer un noeud de type "thread"
 *  Cette fonction récupère le nom, la priorité 
 *  et la charge du thread
 *  \param Thread thread
 */
xmlNodePtr creerThreadXml(Element * element){
  //Thread thread = element->thread;
  xmlNodePtr noeud_thread;

  char priorite[16];
  char charge[4];
  char nom[25]; 

  snprintf(priorite, sizeof(priorite), "%d", GetPrioriteThread(element)); 
  snprintf(charge, sizeof(charge), "%d", GetCharge(element));
  strcpy(nom, GetNom(element));
  // Création du noeud "thread"
  if ((noeud_thread = xmlNewNode(NULL, "thread")) == NULL) {
    printf("DEBUG : erreur dans la sauvegarde lors de n t\n");
    return NULL;
  }
    // Ajout de son attribut "nom"
  if (xmlSetProp(noeud_thread, "nom", nom) == NULL) {
    xmlFreeNode(noeud_thread); 
    printf("DEBUG : erreur dans la sauvegarde lors nom t\n");   
    return NULL;
  }
 // Ajout de son attribut "priorite"
  if (xmlSetProp(noeud_thread, "priorite", priorite) == NULL) {
    xmlFreeNode(noeud_thread);
    printf("DEBUG : erreur dans la sauvegarde lors prio t\n");
    return NULL;
  }
 
  // Ajout de son attribut "charge"
  if (xmlSetProp(noeud_thread, "charge", charge) == NULL) {
    xmlFreeNode(noeud_thread);
     printf("DEBUG : erreur dans la sauvegarde lors charge t\n");
 return NULL;
  }
  return noeud_thread;
}
/************************************************************************
 * Pour la sauvegarde
 */

#if 1
/*! Fonction qui génère les balises XML associés à l'arbre parcouru 
 * \param Element * elementCourant c'est forcement de type bulle 
 * \param xmlNodePtr noeud est le noeud à partir du quel on lie ses fils
 */
void parcourirBullesXml(Element * bulle, xmlNodePtr noeud) {
  int i;
  Element * elementFilsPtr;
  xmlNodePtr noeudFils;
  int taillebulle = GetNbElement(bulle);
  fprintf(stderr,"Taille de la bulle : %d\n", taillebulle);
  for(i=1; i<= taillebulle; i++){
    fprintf(stderr, "Traitement de l'element %d\n", i);
    elementFilsPtr=GetElement(bulle, i);
    if(GetTypeElement(elementFilsPtr) == BULLE){
      fprintf(stderr, "bulle en traitement\n");
      noeudFils = creerBulleXml(elementFilsPtr);
      xmlAddChild(noeud, noeudFils);
      parcourirBullesXml(elementFilsPtr, noeudFils);
    }
    if(GetTypeElement(elementFilsPtr) == THREAD){
      fprintf(stderr, "thread en traitement\n");
     noeudFils = creerThreadXml(elementFilsPtr);
     xmlAddChild(noeud, noeudFils);
    }
  }
}
#endif
/* test*/

void parcourirDocXml(Element * elementCourant, xmlNodePtr noeud) {
  xmlNodePtr noeudFils;
  
  printf("DEBUG :debut xml\n");
  printf("DEBUG : Type d'élément %d\n", GetTypeElement(elementCourant));
  
  if(GetTypeElement(elementCourant) == BULLE){
    noeudFils = creerBulleXml(elementCourant);
    xmlAddChild(noeud, noeudFils);  
    printf("DEBUG : creation bulle xml\n");
  }
  if(GetTypeElement(elementCourant) == THREAD){
    noeudFils = creerThreadXml(elementCourant);
    xmlAddChild(noeud, noeudFils);  
    printf("DEBUG : creation thread xml\n");
  }
  printf("DEBUG : fin xml\n");
}

/***************************************************************************
 * Pour le chargement
 */
void lectureNoeud (xmlNodePtr node){
  if (node->type == XML_ELEMENT_NODE){
    xmlNodePtr n;
    for (n = node; n; n = n->next){
      if (n->children){
	lectureNoeud(n->children);
      }
    }
  }
  else if ((node->type == XML_CDATA_SECTION_NODE)
 	   || (node->type == XML_TEXT_NODE)){
 
    xmlChar *path = xmlGetNodePath (node);
    printf ("%s -> '%s'\n", path,
	    node->content ? (char *) node->content : "(null)");
    xmlFree (path);
    return;
  }
}

/* on utilise principalement des fonctions de dessins dans "zone" et 
 * des fonctions d'objets dans "bulle"
 */

/***************************************************************************
 * Pour les tests
 */
#if 0
int main(){
  
  xmlNodePtr root, nouv_thread, nouv_bulle;
  
  xmlDocPtr xmldoc = xmlNewDoc ("1.0"); 
  xmlChar *str;
  int size;
  
  root = xmlNewNode(NULL, "bulle");
  xmlDocSetRootElement(xmldoc, root);
  // Ajout d'un nouveau produit en fin/queue
  nouv_bulle = creerBulleXml("m", "4.00");
  nouv_thread = creerThreadXml("m", "caca", "4.00","42");

  if (nouv_bulle){ 
    xmlAddChild(root, nouv_bulle); 
    printf("creation ok\n");
  }
  
  if (nouv_thread){ 
    xmlAddChild( nouv_bulle, nouv_thread); 
    printf("creation ok!\n");
  }
  
  // Affichage de l'arbre DOM tel qu'il est en mémoire
  xmlDocFormatDump(stdout, xmldoc, 1);
  
  /*   FILE * fs = fopen("toto","w"); */
  /*   xmlDocDump(fs, xmldoc); */
  
  xmlSaveFile("popo", xmldoc);
  // Libération de la mémoire
  xmlFreeDoc(xmldoc);
  
  return EXIT_SUCCESS;
}
#endif

#endif


