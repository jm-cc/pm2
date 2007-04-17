#ifndef INTERFACEGAUCHE_H
#define INTERFACEGAUCHE_H

#include "interfaceGaucheDessin.h"

#define DEF_CHARGE 10
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
