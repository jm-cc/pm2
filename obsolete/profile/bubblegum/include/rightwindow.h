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

#ifndef RIGHT_WINDOW_H
#define RIGHT_WINDOW_H

#include <gtk/gtk.h>

/* #include "timer.h" */
/* #include "animateur.h" */
#include "animation.h"
#include "polices.h"
#include "texture.h"
#include "actions.h"

/*! \todo remove global variables and AnimElements structure. */

#ifndef ANIMATION_DATA_STRUCT
#  define ANIMATION_DATA_STRUCT
typedef AnimElements AnimationData;
#endif

extern AnimationData *anim;
extern GtkWidget     *right_scroll_bar;
extern GtkWidget     *right_status;
extern guint         right_status_context_id;


GtkWidget* right_window_init(AnimationData *p_anim);
#endif
