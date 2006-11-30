/**********************************************************************
 * File  : rightwindow.h
 * Author: Hardy
 * Date  : 02/03/2006
 **********************************************************************/

#ifndef RIGHT_WINDOW_H
#define RIGHT_WINDOW_H

#include <wchar.h>
#include <math.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include <gtk/gtkgl.h>
#include <GL/gl.h>
#include <GL/glu.h>

#include "timer.h"
#include "actions.h"
#include "animateur.h"
#include "polices.h"
#include "texture.h"

extern AnimElements* anim;

GtkWidget* make_right_window(GtkWidget* parent);
void       make_right_toolbar(GtkWidget* vbox, GtkWidget* parent);
void       make_drawable_zone(GtkWidget* vbox);
void       OnPositionChange(GtkWidget* pWidget, gpointer data);
gboolean   Redraw_dz(gpointer anim);
void       Realize_dz(GtkWidget* widget, gpointer data);
gboolean   Reshape_dz(GtkWidget* widget, GdkEventConfigure* ev, gpointer anim_parm);
gboolean   MouseMove_dz(GtkWidget* widget, GdkEventMotion* ev, gpointer anim_parm);

extern GtkWidget *right_scroll_bar;

#endif
