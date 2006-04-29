#include "bulle.h"

ListeElement* CreateListe(void)
{
   ListeElement* liste;

   liste = malloc(sizeof(ListeElement));

   liste->element = NULL;
   liste->suivant = NULL;

   return liste;
}

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

TypeElement GetTypeElement(Element* element)
{
   return element->type;
}


int GetNbElement(Element* element)
{
   if (element->type == BULLE)
      return element->bulle.taille;
   return 0;
}

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

void RemoveElement(Element* conteneur, int position)
{

   int i;
   ListeElement* liste;
   ListeElement* liste2=NULL;
//   ListeElement* liste3;

   if (conteneur != NULL && conteneur->type == BULLE
       && position >= 1 && position <= conteneur->bulle.taille)
   {
      liste = conteneur->bulle.liste;

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
		 while (liste->element->bulle.taille)
		 {
/*             AddElement(conteneur, GetElement(liste->element, 0)); */
			liste->element->bulle.taille --;

/*  			liste3 = liste->element->bulle.liste; */
/* 			free(liste->element->bulle.liste); */

/*  			liste->element->bulle.liste = liste3; */
		 }

		 liste->element->bulle.liste = NULL;		 
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
      printf("mauvais paramêtre dans RemoveElement\n");

   return;
}


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
      printf("mauvais paramêtre dans RemoveElementOnCascade %d \n",conteneur->bulle.taille);

   return;
}


int GetCharge(Element* thread)
{
   if (thread->type == THREAD)
	  return thread->thread.charge;

   return -1;
}

int GetPrioriteThread(Element* thread)
{
   if (thread->type == THREAD)
	  return thread->thread.priorite;

   return -1;
}

int GetId(Element* thread)
{
   if (thread->type == THREAD)
	  return thread->thread.id;

   return -1;
}

char* GetNom(Element* thread)
{
   if (thread->type == THREAD)
	  return thread->thread.nom;

   return NULL;
}

int GetPrioriteBulle(Element* bulle)
{
   if (bulle->type == BULLE)
	  return bulle->bulle.priorite;

   return -1;
}



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
