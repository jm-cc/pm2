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


#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

typedef struct BiWidget_tag
{
      GtkWidget* wg1;
      GtkWidget* wg2;
} BiWidget;

#endif
