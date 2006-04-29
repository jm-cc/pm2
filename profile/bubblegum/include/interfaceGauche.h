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

void addBulle(GtkWidget* pWidget, gpointer data);
void addThread(GtkWidget* pWidget, gpointer data);
void delete(GtkWidget* pWidget, gpointer data);
void deleteRec(GtkWidget* pWidget, gpointer data);

void OnScrollbarChange(GtkWidget *pWidget, gpointer data);

void traceAddBulle(GtkWidget* widget, DataAddBulle* data);
void traceAddThread(GtkWidget* widget, DataAddThread* data);

int menuitem_rep(GtkWidget *widget,GdkEvent *event);

void imprimer_rep (gchar *string);

#endif
