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

/* à ne pas utiliser,
   c'est pour l'implémentation,
   c'est "private"  */

#ifndef BULLE_H
#define BULLE_H

#include<stdlib.h>
#include<stdio.h>
#include <wchar.h>
#include <string.h>

typedef enum TypeElement_tag{
  BULLE,
  THREAD
} TypeElement;

typedef struct Bulle_tag
{
  struct ListeElement_tag* liste;
  int taille;
  int id;
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

Element* CreateBulle(int priorite, int id);
Element* CreateThread(int priorite, int id, char* nom, int charge);

int GetNbElement(Element* element);

Element* GetElement(Element* conteneur, int position);

ListeElement *FirstListeElement(Element *element);
ListeElement *NextListeElement(ListeElement *element);

void AddElement(Element* conteneur, Element* contenu);

TypeElement GetTypeElement(Element* element);
int GetCharge(Element* thread);
int GetPrioriteThread(Element* thread);
int GetPrioriteBulle(Element* bulle);
int GetId(Element* element);
char* GetNom(Element* thread);

void SetCharge(Element* thread, int charge);
void SetPrioriteThread(Element* thread, int priorite);
void SetPrioriteBulle(Element* bulle, int priorite);

void SetNom(Element* thread, const char* nom);

int GetElementPosition (Bulle * bulleParent, Element * elementRecherche);
ListeElement * GetListeElementPosition(Bulle * bulle, int position);
void MoveElement (Bulle * bulleParent, Element * elementADeplacer, Bulle * bulleDAccueil);

void RemoveElement2(Element* conteneur, int position);
void AddElement2(Element* conteneur, Element* contenu);
int appartientElementParent(Element * parent, Element * fils);


#endif
