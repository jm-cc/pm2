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

#include "bulle.h"


/*********************************************************************/

ListeElement* CreateListe(void)
{
  ListeElement* liste;

  liste = malloc(sizeof(ListeElement));

  liste->element = NULL;
  liste->suivant = NULL;

  return liste;
}



/*********************************************************************/
/* Crée une bulle en indiquant la priorité */
/*********************************************************************/
Element* CreateBulle(int priorite, int id)
{
  Element* element;
  Bulle bulle;
  element = malloc(sizeof(Element));
  /*    printf("create bulle : %p\n", element); */
  element->type = BULLE;

  bulle.liste = NULL;
  bulle.taille = 0;
  bulle.priorite = priorite;
  bulle.id = id;

  element->bulle = bulle;

  return element;
}

/*********************************************************************/

Element* CreateThread(int priorite, int id, char* nom, int charge)
{
  Element* element;
   
  element = malloc(sizeof(Element));
  /*    printf("create THREAD : %p\n", element); */

  element->type = THREAD;
  element->thread.priorite = priorite;
  element->thread.id = id;

  strcpy(element->thread.nom, nom);

  element->thread.charge = charge;
  return element;
}

/*********************************************************************/

TypeElement GetTypeElement(Element* element)
{
  return element->type;
}

/*********************************************************************/

int GetNbElement(Element* element)
{
  if (element->type == BULLE)
    return element->bulle.taille;
  return 0;
}

/*********************************************************************/

Element* GetElement(Element* conteneur, int position)
{
  int i;
  ListeElement* liste;
  liste = conteneur->bulle.liste;
  /*    printf("GetElement %p %d\n", conteneur, position); */

  if(position < 1 || position > conteneur->bulle.taille + 1)
    {
      /*       printf("dans getElement %d %d\n", position, conteneur->bulle.taille); */
      return NULL;
    }

  for(i = 1; i < position; i++)
    {
      if (liste == NULL)
	return NULL;
      liste = liste->suivant;
    }

  if (liste == NULL)
    return NULL;
  return liste->element;
}

/*********************************************************************/

ListeElement *FirstListeElement(Element *element)
{
  if (element->type == BULLE)
    return element->bulle.liste;
  return NULL;
}

ListeElement *NextListeElement(ListeElement *element)
{
  return element->suivant;
}

/*********************************************************************/

void AddElement(Element* conteneur, Element* contenu)
{
  ListeElement* liste;
  ListeElement* liste2=NULL;

  /*    printf("addElement : %p %p\n", conteneur, contenu); */

  if(conteneur->type == BULLE)
    {
      liste = conteneur->bulle.liste;

      if (conteneur->bulle.taille == 0)
	{
	  liste = CreateListe();
	  conteneur->bulle.liste = liste;
	}
      else
	{
	  /* Ajout de contenu à la fin, pourquoi ne pas ajouter au début ? */
	  while(liste != NULL)
	    {
	      liste2 = liste;
	      liste = liste->suivant;
	    }

	  liste = CreateListe();
	  liste2->suivant = liste;
	}


      liste->element = contenu;

      conteneur->bulle.taille++;
      /*   printf("taille du conteneur : %d\n",conteneur->bulle.taille); */

    }
  return;
}


/*********************************************************************
 * Deplace un élement de la position source à la position de destination
 */


 
#if 0
void MoveElement(Element * elmtSource,  Element * elmtDest,
		 int positionSource, int positionDest)
{
  int i;
  ListeElement* liste;
  ListeElement* liste2=NULL;

  /* on retrouve les peres */
  
  
  
  if (positionDest == positionSrc){
    printf("pas de déplacement...\n");
    return;
  }

  /* on cherche à retrouver le pere */

  
  if (elmtPere != NULL && elmtPere->type == BULLE
      && position >= 1 
      && position <= elmtPere->bulle.taille)
    {
      liste = elmtPere->bulle.liste;
      
      /* liste2 va pointer sur l'élément avant l'élément à supprimer
	 liste va pointer sur l'élément à supprimer
	 méthode du trainard
      */
      for(i = 1; i < position; i++)
	{
	  if (liste->suivant == NULL)
	    {
	      printf("erreur dans remove element");
	      return;
	    }
	 
	  liste2 = liste;
	  liste = liste->suivant;
	}
     
      if (liste->element->type == THREAD)
	free(liste->element);
      else
	{
	  /* faire des free sur les éléments appartenant à la bulle */
	  while (liste->element->bulle.taille)
	    {
	      /* AddElement(conteneur, GetElement(liste->element, 0)); */
	      liste->element->bulle.taille --;
	      
	    }
	  
	  liste->element->bulle.liste = NULL;		 
	  free(liste->element);
	}
     
      if (liste2 == NULL) /* impossible A CORRIGER */
	elmtPere->bulle.liste = liste->suivant;
      else
	liste2->suivant = liste->suivant;
     
      free(liste);
     
      elmtPere->bulle.taille --;      
    }
  else /* conteneur->type == THREAD */
    wprintf(L"mauvais paramêtre dans RemoveElement\n");
   
  return;
}
#endif 

/*********************************************************************
 * Supprime l'élément à la position 'position'
 */
void RemoveElement(Element* conteneur, int position)
{

  int i;
  ListeElement* liste;
  ListeElement* liste2=NULL;
   
  if (conteneur != NULL && conteneur->type == BULLE
      && position >= 1 
      && position <= conteneur->bulle.taille)
    {
      liste = conteneur->bulle.liste;
     
      /* liste2 va pointer sur l'élément avant l'élément à supprimer
	 liste va pointer sur l'élément à supprimer
	 méthode du trainard
      */
      for(i = 1; i < position; i++)
	{
	  if (liste->suivant == NULL)
	    {
	      printf("erreur dans remove element");
	      return;
	    }
	 
	  liste2 = liste;
	  liste = liste->suivant;
	}
     
      if (liste->element->type == THREAD)
	free(liste->element);
      else
	{
	  /* faire des free sur les éléments appartenant à la bulle */
	  while (liste->element->bulle.taille)
	    {
	      /* AddElement(conteneur, GetElement(liste->element, 0)); */
	      liste->element->bulle.taille --;
	      
	      /* liste3 = liste->element->bulle.liste; */
	      /* free(liste->element->bulle.liste); */
	      
	      /* liste->element->bulle.liste = liste3; */
	    }
	  
	  liste->element->bulle.liste = NULL;		 
	  free(liste->element);
	}
     
      if (liste2 == NULL) /* impossible A CORRIGER */
	conteneur->bulle.liste = liste->suivant;
      else
	liste2->suivant = liste->suivant;
     
      free(liste);
     
      conteneur->bulle.taille --;      
    }
  else /* conteneur->type == THREAD */
    wprintf(L"mauvais paramêtre dans RemoveElement\n");
   
  return;
}

/*********************************************************************/

void RemoveElementOnCascade(Element* conteneur, int position)
{
  
  int i;
  ListeElement* liste;
  ListeElement* liste2=NULL;
  
  if (conteneur != NULL && conteneur->type == BULLE
      && position >= 1 && position <= conteneur->bulle.taille)
    {
      liste = conteneur->bulle.liste;
      
      for(i = 1; i < position; i++)
	{
	  if (liste->suivant == NULL)
            return;
	  
	  liste2 = liste;
	  liste = liste->suivant;
	}
      
      if (liste->element->type == THREAD)
	{
	  free(liste->element);
	}
      else
	{
	  while (liste->element->bulle.taille)
            RemoveElementOnCascade(liste->element, 1);
	  
	  free(liste->element);
	}
      
      if (liste2 == NULL)
	conteneur->bulle.liste = liste->suivant;
      else
	liste2->suivant = liste->suivant;
      
      free(liste);
      
      conteneur->bulle.taille --;      
    }
  else
    wprintf(L"mauvais paramêtre dans RemoveElementOnCascade %d \n",conteneur->bulle.taille);
  
  return;
}

/*********************************************************************/

int GetCharge(Element* thread)
{
  if (thread->type == THREAD)
    return thread->thread.charge;

  return -1;
}
void SetCharge(Element* thread, int charge)
{
  if (thread->type == THREAD)
    thread->thread.charge=charge;

  return ;
}

/*********************************************************************/

int GetPrioriteThread(Element* thread)
{
  if (thread->type == THREAD)
    return thread->thread.priorite;

  return -1;
}
void SetPrioriteThread(Element* thread, int priorite)
{
  if (thread->type == THREAD)
    thread->thread.priorite=priorite;

  return ;
}

/*********************************************************************/

int GetId(Element* element)
{
  if (element->type == THREAD)
    return element->thread.id;

  else
    return element->bulle.id;
}


/*********************************************************************/

char* GetNom(Element* thread)
{
  if (thread->type == THREAD)
    return thread->thread.nom;

  return NULL;
}
void SetNom(Element* thread, const char* nom)
{
  if (thread->type == THREAD)
    strcpy(thread->thread.nom,nom);
  return;
}


/*********************************************************************/

int GetPrioriteBulle(Element* bulle)
{
  if (bulle->type == BULLE)
    return bulle->bulle.priorite;

  return -1;
}
void SetPrioriteBulle(Element* bulle, int priorite)
{
  if (bulle->type == BULLE)
    bulle->bulle.priorite=priorite;

  return ;
}

/*********************************************************************/
/*! Donne la position d'un élément dans ListeElement, fonction non testée et inutilisée
 *  \param bulleParent     bulleParent est la bulle parent de l'élément 'elementRecherche'
 *  \param elementRecherche    elementRecherche est un élément (bulle ou thread)
 * 
 *  \return La fonction retourne la position de l'élément dans ListeElement.
 */
int GetElementPosition (Bulle * bulleParent, Element * elementRecherche)
{
  ListeElement* liste;
  int position = 1;
  liste = bulleParent->liste;
  /* parcours de la liste chainée simple à la recherche de element */
  while(liste->element != elementRecherche){
    position++;
    liste=liste->suivant;
  }
  return position;
}

/*********************************************************************/
/*! Donne la ListeElement de 'bulle' à la position 'position'. Par exemple, on veut la ListeElement 
 * dont element pointe sur le 3ème élément de ListeElement, on exécute pour cela
 * GetListeElementPosition(bulle, 3);
 * Fonction non testée et inutilisée.
 * \param bulle une bulle contenant des éléments
 * \param position la position à laquelle on souhaite se déplacer dans la liste
 *
 */
ListeElement * GetListeElementPosition(Bulle * bulle, int position)
{
  int taille = bulle->taille;
  int i;
  ListeElement * listeRetour=bulle->liste;
  if (taille < 1)
    {
      printf("Erreur, taille < 1");
      return NULL;
    }
  if (position > taille)
    printf("Erreur, position > taille");
  return NULL;
  for(i=1;i<position;i++)
    {
      listeRetour=listeRetour->suivant;
    }
  if (listeRetour->element->type == THREAD)
    {
      wprintf(L"Erreur GetListeElementPosition, type attendu BULLE, reçu THREAD\n");
      return NULL;
    }
  return listeRetour;
}

/*********************************************************************/   
/*! Effectue un déplacement d'un élément d'une liste vers une autre liste
 * Fonction non testée et inutilisée.
 * \param bulleParent la bulle parent de elementADeplacer
 * \param elementADeplacer bulle ou thread à supprimer de la liste à laquelle elle appartient
 * \param bulleDAccueil la bulle qui va accueillir elementADeplacer
 */
void MoveElement (Bulle * bulleParent, Element * elementADeplacer, Bulle * bulleDAccueil)
{
  /* La bulleParent de elementADeplacer */
  ListeElement * liste1, * liste2;
  liste1 = NULL;
  liste2 = NULL;
  int i, position;
  
  liste1 = bulleParent->liste;
  /* on obtient la position de elementADeplacer */
  position = GetElementPosition(bulleParent, elementADeplacer);
   
  /* méthode du trainard */   
  for(i = 1; i < position; i++)
    {
      if (liste1->suivant == NULL)
	return;
	  
      liste2 = liste1;
      liste1 = liste1->suivant;
    }
   
  if (liste2 == NULL)
    bulleParent->liste = NULL;
  else
    liste2->suivant = liste1->suivant;
  /* dans tous les cas */
  bulleParent->taille --;
      
   
  /* On insère elementADeplacer en tête de la ListeElement de bulleDAccueil */
  bulleDAccueil->liste->suivant = bulleDAccueil->liste;
  bulleDAccueil->liste->element = elementADeplacer;
  /* ne pas oublier d'augmenter la taille de la bulle */
  bulleDAccueil->taille ++;
}

/*********************************************************************/
/* fonction qui affiche la structure */
void PrintElement(Element* element, int level, int numero)
{
  int i;

  //on affiche des espaces pour l'indentation (suivant le niveau de
  //récursion)
  for (i = 1; i <= level; i++)
    printf("  ");

  if (GetTypeElement(element) == BULLE) // si c'est une bulle
    {
      // on l'affiche
      printf("- bulle %p - taille = %d  priorite = %d\n", element, GetNbElement(element), GetPrioriteBulle(element));

      // et on affiche récursivement tout les élément contenus dans
      // cette bulle
      for(i = 1; i <= GetNbElement(element); i++) 
	PrintElement(GetElement(element, i), level + 1, i); 

    }
  else
    {
      // et si c'est un thread on l'affiche
      printf("- thread - priorite = %d  id = %d  nom : %s  charge = %d\n",
	     GetPrioriteThread(element),
	     GetId(element),
	     GetNom(element),
	     GetCharge(element));
    }
  return;
}


/*********************************************************************/
/*! Supprime l'élément à la position 'position' de l'élément 'conteneur'
 * mais ne supprime pas ses fils
 * \param conteneur contient le fils qui va être supprimé
 * \param position la position du fils à supprimer dans la liste de conteneur
 */
void RemoveElement2(Element* conteneur, int position)
{
  int i;
  ListeElement* liste;
  ListeElement* liste2=NULL;
   
  if (conteneur != NULL && conteneur->type == BULLE
      && position >= 1 
      && position <= conteneur->bulle.taille)
    {
      liste = conteneur->bulle.liste;
     
      /* liste2 va pointer sur l'élément avant l'élément à supprimer
	 liste va pointer sur l'élément à supprimer
	 méthode du trainard
      */
      for(i = 1; i < position; i++)
	{
	  if (liste->suivant == NULL)
	    {
	      printf("erreur dans remove element");
	      return;
	    }
	 
	  liste2 = liste;
	  liste = liste->suivant;
	}
      
      if (liste2 == NULL) /* impossible A CORRIGER */
	conteneur->bulle.liste = liste->suivant;
      else
	liste2->suivant = liste->suivant;
     
      /* free(liste); */
     
      conteneur->bulle.taille --;
      printf("DEBUG : RemoveElement2, conteneur->bulle.taille : %d\n",conteneur->bulle.taille);      
    }
  else /* conteneur->type == THREAD */
    wprintf(L"mauvais paramètre dans RemoveElement2\n");
   
  return;
}

/*********************************************************************/
/*! Copie le 'contenu' dans la liste du 'conteneur' en tenant compte
 * de ce que contient déjà le 'contenu' (copie le pointeur de la liste de contenu,
 * on ne recrée pas les éléments fils de 'contenu').
 * \param conteneur l'élément qui va recevoir les nouveaux éléments créés
 * \param contenu l'élément qui va être copié dans conteneur, avec également ses fils
 */
void AddElement2(Element* conteneur, Element* contenu)
{
  ListeElement* liste;
  ListeElement* liste2=NULL;

  /*    printf("addElement : %p %p\n", conteneur, contenu); */

  if(conteneur->type == BULLE)
    {
      liste = conteneur->bulle.liste;

      if (conteneur->bulle.taille == 0)
	{
	  liste = CreateListe();
	  conteneur->bulle.liste = liste;
	}
      else
	{
	  /* Ajout de contenu à la fin, pourquoi ne pas ajouter au début ? */
	  while(liste != NULL)
	    {
	      liste2 = liste;
	      liste = liste->suivant;
	    }
	  liste = CreateListe();
	  liste2->suivant = liste;
	}
      /* là on ajoute le contenu à la fin de la liste de conteneur */
      liste->element = contenu;
      printf("DEBUG : AddElement2, contenu->bulle.taille : %d\n", contenu->bulle.taille);
      /* on n'oublie pas de mettre à jour la taille de l'élément conteneur */
      conteneur->bulle.taille++;
      printf("DEBUG : AddElement2, conteneur->bulle.taille : %d\n",conteneur->bulle.taille);

    }
  return;
}


/**********************************************************************************/
/*! Vérifie si l'élément fils est un fils de l'élément parent
 * \param parent a pour type BULLE
 * \param fils est de type Element
 * \return 0 si le fils n'est pas fils de parent, 1 dans le cas contraire
 */
int appartientElementParent(Element * parent, Element * fils)
{
  ListeElement* liste;
  ListeElement* liste2=NULL;

  if(parent->type == BULLE)
    {
      liste = parent->bulle.liste;
    }
  else
    {
      /* fils non fils de parent */
      return 0;
    }
  /* parcours de toute la liste des fils de parent */
  while(liste != NULL)
    {
      if(liste->element == fils)
	{
	  /* dans le cas où fils est fils de parent */
	  return 1;
	}
      else
	{
	  /* si on tombe sur une bulle, on parcourt ses fils */
	  if(liste->element->type == BULLE)
	    {  
	      return appartientElementParent(liste->element, fils);    
	    }
	}
      liste2 = liste;
      liste = liste->suivant;
    }
  /* dans le cas où fils n'est pas fils de parent */
  return 0; 
}
