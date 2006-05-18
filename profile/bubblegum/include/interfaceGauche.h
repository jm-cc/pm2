#ifndef INTERFACEGAUCHE_H
#define INTERFACEGAUCHE_H

#include "interfaceGaucheDessin.h"


typedef struct DataAddThread_tag
{
      interfaceGaucheVars * iGaucheVars;
      GtkWidget *popup;
      GtkWidget *nomEntry;
      GtkWidget *idEntry;
      GtkWidget *prioriteScrollbar;
      GtkWidget *chargeScrollbar;
}DataAddThread;

typedef struct DataAddBulle_tag
{
      interfaceGaucheVars * iGaucheVars;
      GtkWidget *popup;
      GtkWidget *pScrollbar;
}DataAddBulle;

interfaceGaucheVars* interfaceGauche();

void addBulleAutoOnOff(GtkWidget* pWidget, gpointer data);
void addBulleAutoOff(interfaceGaucheVars* data);
void addThreadAutoOnOff(GtkWidget* pWidget, gpointer data);
void addThreadAutoOff(interfaceGaucheVars* data);
void switchModeAuto(GtkWidget* pWidget, gpointer data);
void deleteRec(GtkWidget* pWidget, gpointer data);

void OnScrollbarChange(GtkWidget *pWidget, gpointer data);

void AddBulle(GtkWidget* widget, DataAddBulle* data);
void AddThread(GtkWidget* widget, DataAddThread* data);

int menuitem_rep(GtkWidget *widget,GdkEvent *event);

void imprimer_rep (gchar *string);

#endif
