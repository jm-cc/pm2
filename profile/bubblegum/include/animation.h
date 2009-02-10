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

#ifndef BUBBLEGUM_ANIMATION_H
#define BUBBLEGUM_ANIMATION_H

#include "bubble_gl_anim.h"

typedef enum {
    ANIM_PLAY_UNCHANGED = -1,
    ANIM_PLAY_PAUSE = 0,
    ANIM_PLAY_REVERSE,
    ANIM_PLAY_NORMAL,
} AnimPlayType;


typedef void (*AnimData_callback)(int, void *);

/*! Animation Data type. Only one instance should be created. */
#ifndef ANIMATION_DATA_STRUCT
#  define ANIMATION_DATA_STRUCT
typedef struct AnimationData_s AnimationData;
#endif

AnimationData *
newAnimationData ();

void
AnimationData_setCallback (AnimationData *pdata,
                           AnimData_callback frameChange,
                           AnimData_callback animReset,
                           void *udata);

BubbleMovie
AnimationData_getMovie (AnimationData *pdata);

void
resetAnimationData (AnimationData *pdata, BubbleMovie movie);

void
AnimationData_setPlayStatus (AnimationData *pdata, AnimPlayType type,
                             float speed, int seeking);

void
AnimationData_gotoFrame (AnimationData *pdata, int frame);

void
AnimationData_setGlobalScale (AnimationData *pdata, float scale);

void
AnimationData_AdjustScale (AnimationData *pdata);

void
AnimationData_SetViewSize (AnimationData *pdata, float width, float height);

void
AnimationData_setPosition (AnimationData *pdata,
                           float goff_x, float goff_y,
                           float rev_x, float rev_y);

void
AnimationData_display (AnimationData *pdata);

#endif /* BUBBLEGUM_ANIMATION_H */
