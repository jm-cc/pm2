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
Element* CreateBulle(int priorite)
{
   Element* element;
   Bulle bulle;
   element = malloc(sizeof(Element));
/*    printf("create bulle : %p\n", element); */
   element->type = BULLE;

   bulle.liste = NULL;
   bulle.taille = 0;
   bulle.priorite = priorite;

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
/*       printf("%d\n",conteneur->bulle.taille); */

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

/*********************************************************************/

int GetPrioriteThread(Element* thread)
{
   if (thread->type == THREAD)
	  return thread->thread.priorite;

   return -1;
}

/*********************************************************************/

int GetId(Element* thread)
{
   if (thread->type == THREAD)
	  return thread->thread.id;

   /* TODO: BUBBLE */
   return -1;
}

/*********************************************************************/

char* GetNom(Element* thread)
{
   if (thread->type == THREAD)
	  return thread->thread.nom;

   return NULL;
}

/*********************************************************************/

int GetPrioriteBulle(Element* bulle)
{
   if (bulle->type == BULLE)
	  return bulle->bulle.priorite;

   return -1;
}

/*********************************************************************/
/*! Donne la position d'un élément dans ListeElement
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
     printf("Erreur GetListeElementPosition, type attendu BULLE, reçu THREAD\n");
     return NULL;
   }
   return listeRetour;
}

/*********************************************************************/   
/*! Effectue un déplacement d'un élément d'une liste vers une autre liste
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




/* int main(void) */
/* { */

/* // On créé des bulles */
/*    Element* bulle1 = CreateBulle(3); */
/*    Element* bulle2 = CreateBulle(2); */
/*    Element* bulle3 = CreateBulle(1); */
/*    Element* bulle4 = CreateBulle(1); */
/*    Element* bulle5 = CreateBulle(5); */
/*    Element* bulle6 = CreateBulle(2); */

/* // on créé des threads */
/*    Element* thread1 = CreateThread(1,1,"jacky",110); */
/*    Element* thread2 = CreateThread(2,2,"john doe",200); */
/*    Element* thread3 = CreateThread(1,3,"mickey",30); */
/*    Element* thread4 = CreateThread(3,4,"obi wan",140); */
/*    Element* thread5 = CreateThread(1,5,"gordon freeman",50); */

/* /\* // on mets les threads dans les bulles *\/ */
/*    AddElement(bulle1, thread1); */
/*    AddElement(bulle1, thread2); */
/*    AddElement(bulle2, thread3); */
/*    AddElement(bulle2, thread4); */
/*    AddElement(bulle3, thread5); */

/* /\* // et les bulles dans les threads *\/ */
/*    AddElement(bulle1, bulle2); */
/*    AddElement(bulle2, bulle3); */
/*    AddElement(bulle3, bulle4); */
/*    AddElement(bulle4, bulle5); */
/*    AddElement(bulle5, bulle6); */

/* /\* // on affiche la structure *\/ */
/*    PrintElement(bulle1, 0); */

/* /\*    RemoveElement(bulle1, 2); *\/ */

/* /\*    PrintElement(bulle1, 1); *\/ */

/*    return EXIT_SUCCESS; */
/* } */






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
/*! Copie le contenu dans la liste du conteneur en tenant compte
 * de ce que contient déjà le contenu
 *
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

      liste->element = contenu;
      printf("DEBUG : AddElement2, contenu->bulle.taille : %d\n", contenu->bulle.taille);

      conteneur->bulle.taille++;
      printf("DEBUG : AddElement2, conteneur->bulle.taille : %d\n",conteneur->bulle.taille);

   }
   return;
}
