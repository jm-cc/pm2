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

#ifndef MAIN_WINDOWS_H
#define MAIN_WINDOWS_H

#include <stdlib.h>
#include <gtk/gtk.h>
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

typedef struct BiWidget_tag
{
  GtkWidget* wg1;
  GtkWidget* wg2;
} BiWidget;

extern interfaceGaucheVars* iGaucheVars;

#endif
