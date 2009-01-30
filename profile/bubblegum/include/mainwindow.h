#ifndef MAIN_WINDOWS_H
#define MAIN_WINDOWS_H

#include <stdlib.h>
#include <gtk/gtk.h>
#include "mainwindow.h"
#include "menus.h"
#include "actions.h"
#include "toolbars.h"
#include "raccourcis.h"
#include "rightwindow.h"
#include "interfaceGauche.h"
#include "rearangement.h"


#define WINDOW_WIDTH 1100
#define WINDOW_HEIGHT 600

#define DEFLOAD 10
#define _tostr(s) #s
#define tostr(s) _tostr(s)

typedef struct BiWidget_tag
{
  GtkWidget* wg1;
  GtkWidget* wg2;
} BiWidget;

extern interfaceGaucheVars* iGaucheVars;

#endif
