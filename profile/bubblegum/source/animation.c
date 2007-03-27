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


#include <memory.h>

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

    struct timeval ctime;      /*! < frame start time. */

    float          g_scale;    /*! < Scaling value to fit in the drawing area */

    AnimData_frameChanged_cb chframe_cb; /*! < Pointer to a callback function */
    void *                   cb_data;    /*! < User data for callback */

    
};



/*! Creates a new #AnimationData structure.
 *
 *  \return a new allocated and initialized #AnimationData structure.
 */
AnimationData *
newAnimationData () {
    AnimationData *anim_data = malloc (sizeof(*anim_data));

    resetAnimationData (anim_data, NULL);

    anim_data->play_speed = 1.0f;

    anim_data->g_scale    = 1.0f;

    anim_data->chframe_cb = NULL;
    anim_data->cb_data    = NULL;

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
AnimationData_setFrameChangedCallback (AnimationData *pdata,
                                       AnimData_frameChanged_cb callback,
                                       void *udata) {
    pdata->chframe_cb = callback;
    pdata->cb_data    = udata;
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
    pdata->movie      = movie;
    pdata->cframe     = -1;
    pdata->play_type  = ANIM_PLAY_NORMAL;
    gettimeofday (&(pdata->ctime), NULL);
}


/*! Sets play status.
 *
 *  \param pdata    a pointer to an #AnimationData structure.
 *  \param type     Playback type. Must be one of #AnimPlayType enumeration
 *                  values.
 *  \param speed    a float that represents the playback velocity.
 */
void
AnimationData_setPlayStatus (AnimationData *pdata,
                             AnimPlayType type, float speed) {
    if (type != ANIM_PLAY_UNCHANGED)
        pdata->play_type = type;
    if (speed >= 0)
        pdata->play_speed = speed;
}


/*! Go to Frame. Callback function for frame changes is not called.
 *
 *  \param pdata    a pointer to an #AnimationData structure.
 *  \param frame    an integer that represents the frame to go to.
 */
void
AnimationData_gotoFrame (AnimationData *pdata, int frame) {
    pdata->cframe = frame;
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
    
    if (!pdata || !pdata->movie)
        return; /* Nothing to display. */
    
    gettimeofday (&tv, NULL);
    
    elapsed_time = tv.tv_sec - pdata->ctime.tv_sec
        + 1e-6f * (tv.tv_usec - pdata->ctime.tv_usec);
    
    if (pdata->cframe != -1) {
        frm_time = pdata->movie->frames_array[pdata->cframe]->duration
            / pdata->play_speed;
    }
    
    /* In any case, we move by one frame max. */
    if (frm_time < elapsed_time) {
        if (pdata->play_type == ANIM_PLAY_NORMAL)
            pdata->cframe++;
        else if (pdata->play_type == ANIM_PLAY_REVERSE)
            pdata->cframe--;
        
        if (pdata->chframe_cb)
            pdata->chframe_cb (pdata->cframe, pdata->cb_data);
        memcpy (&(pdata->ctime), &tv, sizeof (tv));
    }
    
    if (pdata->cframe < 0)
        pdata->cframe = 0;
    else if (pdata->cframe >= pdata->movie->frames_count)
        pdata->cframe = pdata->movie->frames_count - 1;
    
    bgl_anim_DisplayFrame (pdata->movie, pdata->cframe, pdata->g_scale);    
}
