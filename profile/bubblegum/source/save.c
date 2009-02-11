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
#include "save.h"
#include "bulle.h"
#include <stdio.h>

#include "interfaceGauche.h"

extern interfaceGaucheVars* iGaucheVars;


/*! Creer un noeud de type "bulle"
 *  Cette fonction récupère directement l'attribut priorité de la bulle
 *  \param element dans ce cas c'est une bulle.
 *  \return le noeud créer, NULL en cas d'erreur.
 */
xmlNodePtr creerBulleXml(Element * element) {
  xmlNodePtr noeud_bulle;

  /* récupération des attributs */
  /* unsigned */ char priorite[4];
  snprintf(priorite, sizeof(priorite), "%d", GetPrioriteBulle(element));

  /* Création du noeud "bulle" */
  noeud_bulle = xmlNewNode(NULL, (unsigned char*)"bulle");

  if(noeud_bulle == NULL) {
    printf("DEBUG : erreur dans la sauvegarde lors creation noeud bulle\n");
    return NULL;
  }

  /* Ajout de son attribut */
  if (xmlSetProp(noeud_bulle, (unsigned char*)"priorite", (unsigned char*)priorite) == NULL) {
    xmlFreeNode(noeud_bulle);
    printf("DEBUG : erreur dans la sauvegarde lors priorite bulle\n");
    return NULL;
  }
  return noeud_bulle;
}


/*! Creer un noeud de type "thread"
 *  Cette fonction récupère le nom, la priorité
 *  et la charge du thread
 *  \param  element c'est forcément du type THREAD.
 *  \return le noeud créer, NULL en cas d'erreur.
 */
xmlNodePtr creerThreadXml(Element * element) {
  xmlNodePtr noeud_thread;

  /* unsigned */ char priorite[4];
  /* unsigned */ char charge[4];
  /* unsigned */ char nom[32];

  snprintf(priorite, sizeof(priorite), "%d", GetPrioriteThread(element));
  snprintf(charge, sizeof(charge), "%d", GetCharge(element));

  strcpy(nom, GetNom(element));
  /*  Création du noeud "thread" */
  if ((noeud_thread = xmlNewNode(NULL, (unsigned char*)"thread")) == NULL) {
    printf("DEBUG : erreur dans la sauvegarde lors de noeud thread\n");
    return NULL;
  }
  /*  Ajout de son attribut "nom" */
  if (xmlSetProp(noeud_thread, (unsigned char*)"nom", (unsigned char*)nom) == NULL) {
    xmlFreeNode(noeud_thread);
    printf("DEBUG : erreur dans la sauvegarde lors nom thread\n");
    return NULL;
  }
  /*  Ajout de son attribut "priorite" */
  if (xmlSetProp(noeud_thread, (unsigned char*)"priorite", (unsigned char*)priorite) == NULL) {
    xmlFreeNode(noeud_thread);
    printf("DEBUG : erreur dans la sauvegarde lors priorite thread\n");
    return NULL;
  }

  /*  Ajout de son attribut "charge" */
  if (xmlSetProp(noeud_thread, (unsigned char*)"charge", (unsigned char*)charge) == NULL) {
    xmlFreeNode(noeud_thread);
    printf("DEBUG : erreur dans la sauvegarde lors charge thread\n");
    return NULL;
  }
  return noeud_thread;
}

/************************************************************************
 * Pour la sauvegarde
 */

/*! Fonction qui génère les balises XML associés à l'arbre
 *  de thread et de bulle parcouru dans l'interface de gauche.
 * \param Element * elementCourant c'est forcement de type bulle.
 * \param xmlNodePtr noeud est le noeud à partir du quel on lie ses fils.
 */
void parcourirBullesXml(Element * bulle, xmlNodePtr noeud) {
  ListeElement * liste;
  Element * elementFilsPtr;
  xmlNodePtr noeudFils;

  for(liste = FirstListeElement(bulle); liste; liste = NextListeElement(liste)) {
    elementFilsPtr = liste->element;

    if(GetTypeElement(elementFilsPtr) == BULLE) {
      noeudFils = creerBulleXml(elementFilsPtr);
      xmlAddChild(noeud, noeudFils);
      parcourirBullesXml(elementFilsPtr, noeudFils);
    }
    if(GetTypeElement(elementFilsPtr) == THREAD) {
      noeudFils = creerThreadXml(elementFilsPtr);
      xmlAddChild(noeud, noeudFils);
    }
  }
}

/*! Fonction qui va faire la sauvegarde de l'arbre XML
 *  à l'adresse indiquée.
 * \param chemin endroit où va être sauvegarder le fichier généré.
 * \return 0 en fonctionnement correct et -1 en cas d'erreur.
*/
int enregistrerXml(const char* chemin, Element *racine)
{
  /* initialisation de la structure */
  xmlDocPtr xmldoc;
  xmldoc = xmlNewDoc ((unsigned char*)"1.0");
  xmlNodePtr root;

  root = xmlNewNode(NULL, (unsigned char*)"BubbleGum");
  xmlDocSetRootElement(xmldoc, root);

  /* parcours */
  parcourirBullesXml(racine, root);

  /* sauvegarde de l'arbre, 1 pour l'indentation */
  if(xmlSaveFormatFile(chemin, xmldoc, 1) == -1){
    xmlFreeDoc(xmldoc);
    return -1;
  }
  xmlFreeDoc(xmldoc);

  return 0;
}


/***************************************************************************
 * Pour le chargement
 */


/*! Fonction qui lance la creation des element rencontres
 *  à partir du noeud donné en paramètre
 *
 */
void parcourirArbreXml(xmlNodePtr onode, Element * bulleAccueil, zone * z_conteneur) {
  xmlNodePtr tree, node;
  //wprintf(L"DEBUG : Début de lecture\n");

  tree = onode->children;

  for (node = tree; node; node = node->next){
    if ((node->type == XML_ELEMENT_NODE)){
      /* Si l'élément est une bulle */
      if(strcmp((char *)node->name, "bulle") == 0){
	char * attr_priorite;
	int attrI_priorite;

	/* On verifie son attribut */
	attr_priorite = (char*)xmlGetProp (node, (unsigned char*)"priorite");
	attrI_priorite = atoi(attr_priorite);

	/* On creer la bulle correspondante */
	Element * nouvelleBulle;
	zone * z_bulle;

	nouvelleBulle = CreateBulle(attrI_priorite, SetId(iGaucheVars));
        /* on l'insère dans la bulle de destination */
        AddElement(bulleAccueil, nouvelleBulle);
        /* on crée la zone qui va contenir cette nouvelle bulle */
        z_bulle = CreerZone(0,0,LARGEUR_B, HAUTEUR_B);
        /* et on l'insère dans la zone conteneur */
        AjouterSousZones(z_conteneur, z_bulle);
        Rearanger(iGaucheVars->zonePrincipale);

	/* on lance la récurrence */
	parcourirArbreXml(node, nouvelleBulle, z_bulle);

	/* on libère */
	xmlFree (attr_priorite);
      }

      /* cas du thread */
      if(strcmp((char *)node->name, "thread") == 0){
	/* unsigned */ char *attr_nom;
	/* unsigned */ char *attr_priorite;
	/* unsigned */ char *attr_charge;
	int attrI_priorite;
	int attrI_charge;
	Element * nouveauThread;

	/* On verifie et on récupere ses attributs */
	attr_nom = (char *)xmlGetProp (node, (unsigned char*)"nom");
	attr_priorite = (char *)xmlGetProp (node, (unsigned char*)"priorite");
	attr_charge = (char *)xmlGetProp (node, (unsigned char*)"charge");

	attrI_priorite = atoi(attr_priorite);
	attrI_charge = atoi(attr_charge);


	/* On creer le thread correspondant */
	nouveauThread = CreateThread(attrI_priorite, SetId(iGaucheVars),
				     attr_nom, attrI_charge);
	AddElement(bulleAccueil, nouveauThread);
	AjouterSousZones(z_conteneur, CreerZone(0,0,LARGEUR_T, HAUTEUR_T));
	Rearanger(iGaucheVars->zonePrincipale);

	xmlFree(attr_nom);
	xmlFree(attr_priorite);
	xmlFree(attr_charge);
      }
    }
  }
  //  printf("DEBUG : Fin de lecture\n");
}

/*! Charge un fichier xml dans bubblegum
 *  \param Chemin endroit ou s'effectue la lecture.
 *  \return 0 en fonctionnement correct et -1 en cas d'erreur.
 */
int chargerXml(const char * Chemin){

  xmlDocPtr xmldoc = NULL;
  xmlNodePtr racine = NULL;

  xmldoc = xmlParseFile(Chemin);
  if (!xmldoc){
      fprintf (stderr, "%s:%d: Erreur d'ouverture du fichier \n",
	       __FILE__, __LINE__);
      return -1;
  }
  else {
    racine = xmlDocGetRootElement(xmldoc);
    if (racine == NULL) {
      fprintf(stderr, "Document XML vierge\n");
      xmlFreeDoc(xmldoc);
      return -1;
    }

    /* On cree les bulles et les threads selon le fichier */
    else {
      parcourirArbreXml(racine, iGaucheVars->bullePrincipale,
			iGaucheVars->zonePrincipale);
    }

    /* on libere */
    xmlFreeDoc(xmldoc);
  }
  return 0;
}

