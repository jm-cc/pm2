/* à ne pas utiliser,
   c'est pour l'implémentation,
   c'est "private"  */

#ifndef BULLE_H
#define BULLE_H

#include<stdlib.h>
#include<stdio.h>
#include <wchar.h>
#include"bulle.h"
#include <string.h>

typedef enum TypeElement_tag{
  BULLE,
  THREAD
} TypeElement;

typedef struct Bulle_tag
{
  struct ListeElement_tag* liste;
  int taille;
  int priorite;
} Bulle;


typedef struct Thread_tag
{
  int priorite;
  char nom[20];
  int id;
  int charge;
} Thread;


typedef struct Element_tag
{
  TypeElement type;
  union
  {
    Bulle bulle;
    Thread thread;
  };
} Element;


typedef struct ListeElement_tag
{
  Element* element;
  struct ListeElement_tag* suivant;
} ListeElement;


ListeElement* CreateListe(void);

void RemoveListe(ListeElement* liste);
void RemoveElement(Element* conteneur, int position);
void RemoveElementOnCascade(Element* conteneur, int position);

void PrintElement(Element* bulle, int level, int numero);

Element* CreateBulle(int priorite);
Element* CreateThread(int priorite, int id, char* nom, int charge);

int GetNbElement(Element* element);

Element* GetElement(Element* conteneur, int position);

void AddElement(Element* conteneur, Element* contenu);

TypeElement GetTypeElement(Element* element);
int GetCharge(Element* thread);
int GetPrioriteThread(Element* thread);
int GetPrioriteBulle(Element* bulle);
int GetId(Element* bulle);
char* GetNom(Element* bulle);

int GetElementPosition (Bulle * bulleParent, Element * elementRecherche);
ListeElement * GetListeElementPosition(Bulle * bulle, int position);
void MoveElement (Bulle * bulleParent, Element * elementADeplacer, Bulle * bulleDAccueil);

void RemoveElement2(Element* conteneur, int position);
void AddElement2(Element* conteneur, Element* contenu);

#endif
