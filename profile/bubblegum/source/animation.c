/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2007 Raphaël BOIS <mailto:bois@enseirb.fr>
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

#include <stdlib.h>
#include <memory.h>
#include <GL/gl.h>
#include <GL/glu.h>

#ifdef WIN32
#  include <sys/timeb.h>
static int
gettimeofday (struct timeval *tp, void *tz)
{
   struct _timeb timebuffer;
   _ftime(&timebuffer);
   tp->tv_sec = timebuffer.time;
   tp->tv_usec = timebuffer.millitm * 1000;
   return 0;
}
#else

#  include <sys/time.h>
#endif /* WIN32 */

#include "animation.h"




/*! Animation Data structure.
 *  \todo complete structure.
 */
struct AnimationData_s {
    BubbleMovie    movie;      /*! < a pointer to a BGLMovie structure. */

    int            cframe;     /*! < current frame. */
    float          play_speed; /*! < playback speed (normal = 1.0) */
    AnimPlayType   play_type;  /*! < playback type. */
    int            play_seeking; /*! < is seeking in progress or not ? */

    struct timeval ctime;      /*! < frame start time. */

    float          width, height; /*! < Draw rea size. */

    float          g_scale;    /*! < Scaling value to fit in the drawing area */
    float          goff_x, goff_y; /*! < Axis Origin */
    float          rev_x, rev_y;    /*! < Axis orientation */

    AnimData_callback chframe_cb;  /*! < Frame change callback function */
    AnimData_callback newmovie_cb; /*! < new movie callback function */
    void *            cb_data;     /*! < User data for callback */
};



/*! Creates a new #AnimationData structure.
 *
 *  \return a new allocated and initialized #AnimationData structure.
 */
AnimationData *
newAnimationData () {
    AnimationData *anim_data = malloc (sizeof(*anim_data));

    anim_data->chframe_cb = NULL;
    anim_data->newmovie_cb= NULL;
    anim_data->cb_data    = NULL;

    resetAnimationData (anim_data, NULL);

    anim_data->play_speed =  1.0f;

    anim_data->height     =  1.0f;
    anim_data->width      =  1.0f;

    anim_data->goff_x     =  0.0f;
    anim_data->goff_y     =  0.0f;
    anim_data->rev_x      =  1.0f;
    anim_data->rev_y      = -1.0f;

    anim_data->g_scale    =  1.0f;

    return anim_data;
}


/*! Sets callback function on frame changes.
 *
 *  \param pdata    a pointer to an #AnimationData structure.
 *  \param callback a pointer to a function to call when current frame changes.
 *  \param udata    a pointer to user data to be passed to the callback
 *                  function.
 */
void
AnimationData_setCallback (AnimationData *pdata,
                           AnimData_callback frameChange,
                           AnimData_callback newMovie,
                           void *udata) {
    pdata->chframe_cb  = frameChange;
    pdata->newmovie_cb = newMovie;
    pdata->cb_data     = udata;
}



/*! Gets the current movie of an animation.
 *
 *  \param pdata    a pointer to an #AnimationData structure.
 *
 *  \return a pointer to a #BGLMovie structure or NULL.
 */
BubbleMovie
AnimationData_getMovie (AnimationData *pdata) {
    return pdata->movie;
}

/*! Resets an #AnimationData structure to default values.
 *
 *  \param pdata    a pointer to an #AnimationData structure.
 *  \param movie    a pointer to a #BGLMovie structure.
 *
 *  \warning Do not release previous movie resources.
 */
void
resetAnimationData (AnimationData *pdata, BubbleMovie movie) {
    pdata->movie        = movie;
    pdata->cframe       = -1;
    pdata->play_type    = ANIM_PLAY_NORMAL;
    pdata->play_seeking = 0;
    gettimeofday (&(pdata->ctime), NULL);

    if (movie) {
        if (pdata->newmovie_cb)
            pdata->newmovie_cb (movie->frames_count, pdata->cb_data);
        if (pdata->chframe_cb)
            pdata->chframe_cb (0, pdata->cb_data);
    }
}


#ifndef MIN
#  define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

/*! Adjusts scale parameters.
 *
 *  \param pdata    a pointer to an #Animationdata structure.
 *  \param pwidth   a float that represents the draw area width.
 *  \param pheight  a float that represents the draw area height.
 */
void
AnimationData_AdjustScale (AnimationData *pdata) {
    float scale = 1.0f;
    float goff_x = 0.0f, goff_y = pdata->height;
    float rev_x = 1.0f, rev_y = -1.0f;
    BubbleMovie movie = pdata->movie;

    /* Compute animation location */
    
    if (movie) {
        scale = MIN (pdata->height / movie->height, pdata->width / movie->width);
        goff_x = (pdata->width - scale * movie->width) / 2;
        goff_y = pdata->height - (pdata->height - scale * movie->height) / 2;
    }

    AnimationData_setGlobalScale (pdata, scale);
    AnimationData_setPosition (pdata, goff_x, goff_y, rev_x, rev_y);
}

/*! Sets view size.
 *
 *  \param pdata    a pointer to an #Animationdata structure.
 *  \param width    a float that represents view width.
 *  \param height   a float that represents view height.
 */
void
AnimationData_SetViewSize (AnimationData *pdata, float width, float height) {
    pdata->width = width;
    pdata->height = height;
}


/*! Sets play status.
 *
 *  \param pdata    a pointer to an #AnimationData structure.
 *  \param type     Playback type. Must be one of #AnimPlayType enumeration
 *                  values.
 *  \param speed    a float that represents the playback velocity.
 */
void
AnimationData_setPlayStatus (AnimationData *pdata, AnimPlayType type,
                             float speed, int seeking) {
    if (type != ANIM_PLAY_UNCHANGED)
        pdata->play_type = type;
    if (speed >= 0)
        pdata->play_speed = speed;
    pdata->play_seeking = seeking;
}


/*! Go to Frame. Callback function for frame changes is not called.
 *
 *  \param pdata    a pointer to an #AnimationData structure.
 *  \param frame    an integer that represents the frame to go to.
 */
void
AnimationData_gotoFrame (AnimationData *pdata, int frame) {
    pdata->cframe = frame;
    if (frame < 0 && pdata->movie)
        pdata->cframe = pdata->movie->frames_count - 1;
    gettimeofday (&(pdata->ctime), NULL);
}


/*! Sets global scaling.
 *
 *  \param pdata    a pointer to an #AnimationData structure.
 *  \param scale    a float that represents the global scaling value.
 */
void
AnimationData_setGlobalScale (AnimationData *pdata, float scale) {
    pdata->g_scale = scale;
}


/*! Sets global position and axis orientation.
 *
 *  \param pdata    a pointer to an #AnimationData structure.
 *  \param goff_x   global abscissa origin.
 *  \param goff_y   global ordinate origin.
 *  \param rev_x    abscissa axis orientation.
 *  \param rev_y    ordinate axis orientation.
 */
void
AnimationData_setPosition (AnimationData *pdata,
                           float goff_x, float goff_y,
                           float rev_x, float rev_y) {
    pdata->goff_x = goff_x;
    pdata->goff_y = goff_y;
    pdata->rev_x  = rev_x;
    pdata->rev_y  = rev_y;
}



/*! Plays a frame of the movie. It takes care of frame timing and move to the
 *  next frame when needed. In that case, calls the callback function.
 *
 *  \param pdata    a pointer to an #AnimationData structure.
 */
void
AnimationData_display (AnimationData *pdata) {
    struct timeval tv;
    float elapsed_time;

    float frm_time = 0;
    int   chframe  = 0;
    
    if (!pdata || !pdata->movie)
        return; /* Nothing to display. */
    
    AnimationData_AdjustScale (pdata);
    gettimeofday (&tv, NULL);
    
    elapsed_time = tv.tv_sec - pdata->ctime.tv_sec
        + 1e-6f * (tv.tv_usec - pdata->ctime.tv_usec);
    
    if (pdata->cframe != -1) {
        frm_time = pdata->movie->frames_array[pdata->cframe]->duration
            / pdata->play_speed;
    } else
        pdata->cframe = 0;
    
    /* In any case, we move by one frame max. */
    if (frm_time < elapsed_time) {
        if (!pdata->play_seeking) { /* If seeking, no automatic frame change. */
            chframe = 1;
            if (pdata->play_type == ANIM_PLAY_NORMAL)
                pdata->cframe++;
            else if (pdata->play_type == ANIM_PLAY_REVERSE)
                pdata->cframe--;
        }
        memcpy (&(pdata->ctime), &tv, sizeof (tv));
    }
    
    if (pdata->cframe < 0) {
        pdata->cframe = 0;
        chframe = 0;
    }
    else if (pdata->cframe >= pdata->movie->frames_count) {
        pdata->cframe = pdata->movie->frames_count - 1;
        chframe = 0;
    }

    if (pdata->chframe_cb && chframe)
        pdata->chframe_cb (pdata->cframe, pdata->cb_data);

    bgl_anim_DisplayFrame (pdata->movie, pdata->cframe, pdata->g_scale,
                           pdata->goff_x, pdata->goff_y,
                           pdata->rev_x, pdata->rev_y);
}
