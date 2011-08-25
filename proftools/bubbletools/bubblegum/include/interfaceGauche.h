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

#ifndef INTERFACEGAUCHE_H
#define INTERFACEGAUCHE_H

#include "interfaceGaucheDessin.h"

#define MAX_CHARGE 100
#define DEF_CHARGE 10
#define MAX_PRIO 10
#define DEF_PRIO 0

/* extern unsigned int NumTmp; */
/* extern unsigned int NumTmpMax; */

typedef struct DataAddThread_tag
{
  interfaceGaucheVars * iGaucheVars;
  int idThread;
  GtkWidget *popup;
  GtkWidget *nomEntry;
  GtkWidget *idEntry;
  GtkWidget *prioriteScrollbar;
  GtkWidget *chargeScrollbar;
}DataAddThread;

typedef struct DataAddBulle_tag
{
  interfaceGaucheVars * iGaucheVars;
  int idBulle;
  GtkWidget *popup;
  GtkWidget *pScrollbar;
}DataAddBulle;

interfaceGaucheVars* interfaceGauche();

void addBulleAutoOnOff(GtkWidget* pWidget, gpointer data);
void addBulleAutoOff(interfaceGaucheVars* data);
void addThreadAutoOnOff(GtkWidget* pWidget, gpointer data);
void addThreadAutoOff(interfaceGaucheVars* data);
void encapsuler();
void deleteRec(GtkWidget* pWidget, gpointer data);
void deleteRec2(GtkWidget* pWidget, gpointer data);

void OnScrollbarChange(GtkWidget *pWidget, gpointer data);

void AddBulle(GtkWidget* widget, DataAddBulle* data);
void AddThread(GtkWidget* widget, DataAddThread* data);

int menuitem_rep(GtkWidget *widget,GdkEvent *event);

void imprimer_rep (gchar *string);

void CopyElement(Element* conteneur, zone* z_conteneur, Element* contenu, interfaceGaucheVars * iGaucheVars, int newid);

void enregistrerTmp(void);

#endif
