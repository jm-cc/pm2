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

#include <stdio.h>
#include <signal.h>
#include <stdarg.h>
#include <stdlib.h>
#include <memory.h>

/* #define BUBBLE_GL_DEBUG */
#include "bubble_gl_anim.h"
#include "bubblelib_anim.h"

/*! Prints a message on standard error before aborting.
 *
 *  \param msg      Message format string. It follows the same syntax as the
 *                  printf format string.
 *
 */
static void
error (const char *msg, ...) {
    va_list args;

    va_start (args, msg);
    vfprintf (stderr, msg, args);
    va_end (args);
    abort();
}

/*! Prints the signal id and aborts.
 *
 *  \param sig      Signal id to display.
 *
 *  \sa error()
 */
#if 0
static void
sig (int sig) {
    error ("got signal %d\n", sig);
}
#endif


/* *******************************************************************
 * Action functions
 */

typedef bgl_action_t *(*setRatio_fn_t)(bgl_action_t *, bgl_action_t *, float);



/*! Creates a new action.
 *
 *  \param type     Type of the action.
 *
 *  \return A new allocated action of the required type.
 */
static bgl_action_t *
bgl_action_new (bgl_action_type_e_t type) {
    bgl_action_t *new_action;
    size_t alloc_size;

    switch (type)
        {
        case BGL_ACTION_MOVE_PEN:
            alloc_size = sizeof (bgl_action_movepen_t);
            break;
        case BGL_ACTION_DRAW_LINE:
            alloc_size = sizeof (bgl_action_drawline_t);
            break;
        case BGL_ACTION_DRAW_CIRCLE:
            alloc_size = sizeof (bgl_action_drawcircle_t);
            break;
        case BGL_ACTION_DRAW_CURVE:
            alloc_size = sizeof (bgl_action_drawcurve_t);
            break;
        case BGL_ACTION_DRAW_GLYPH:
            alloc_size = sizeof (bgl_action_drawglyph_t);
            break;
        default:
            return NULL;
        }

    new_action = malloc (alloc_size);
    memset (new_action, 0, alloc_size);

    new_action->type = type;
    new_action->size = alloc_size;

    return new_action;
}

/*! Creates a copy of an action.
 *
 *  \param action   A valid pointer to an action.
 *
 *  \return A new allocated action structure of the same type and with the same
 *     value that the one pointed by \a action.
 */
static bgl_action_t *
bgl_action_copy (bgl_action_t *action) {
    bgl_action_t *new_action = bgl_action_new(action->type);
    memcpy (new_action, action, new_action->size);

    return new_action;
}


/*! Fix the intermediate values for an action with style.
 *
 *  \param action    action to be fixed
 *  \param a1        initial action
 *  \param a2        final action
 *  \param ratio     an integer that represent the ratio for the interpolation.
 *
 *  \todo Fix the fill member
 */
static void
bgl_action_with_style_setRatio (bgl_action_with_style_t *action,
		                bgl_action_with_style_t *a1,
				bgl_action_with_style_t *a2,
		                float ratio) {
	if (ratio < 0.5)
		action->fill = a1->fill;
	else
		action->fill = a2->fill;
	action->line.color.r = a1->line.color.r + ratio * (a2->line.color.r - a1->line.color.r);
	action->line.color.g = a1->line.color.g + ratio * (a2->line.color.g - a1->line.color.g);
	action->line.color.b = a1->line.color.b + ratio * (a2->line.color.b - a1->line.color.b);
	action->line.color.a = a1->line.color.a + ratio * (a2->line.color.a - a1->line.color.a);
	action->line.width = a1->line.width + ratio * (a2->line.width - a1->line.width);
}

/*! Creates a new movepen action interpolation. This function chould not be
 *  called explicitly. 
 *
 *  \param a1       a pointer to a move pen action.
 *  \param a2       a pointer to a move pen action.
 *  \param ratio    an integer that represent the ratio for the interpolation.
 *
 * \return A newly allocated movepen action.
 */
static bgl_action_t *
bgl_action_movepen_setRatio (bgl_action_movepen_t *a1,
                             bgl_action_movepen_t *a2,
                             float ratio) {
    bgl_action_movepen_t *act =
        (bgl_action_movepen_t *) bgl_action_new (BGL_ACTION_MOVE_PEN);
    act->position.x = a1->position.x + ratio * (a2->position.x - a1->position.x);
    act->position.y = a1->position.y + ratio * (a2->position.y - a1->position.y);

    return (bgl_action_t *) act;
}

/*! Creates a new drawline action interpolation. This function chould not be
 *  called explicitly. 
 *
 *  \param a1       a pointer to a drawline action.
 *  \param a2       a pointer to a drawline action.
 *  \param ratio    an integer that represent the ratio for the interpolation.
 *
 * \return A newly allocated drawline action.
 */
static bgl_action_t *
bgl_action_drawline_setRatio (bgl_action_drawline_t *a1,
                              bgl_action_drawline_t *a2,
                             float ratio) {
    bgl_action_drawline_t *act =
        (bgl_action_drawline_t *) bgl_action_new (BGL_ACTION_DRAW_LINE);
    act->draw_to.x = a1->draw_to.x + ratio * (a2->draw_to.x - a1->draw_to.x);
    act->draw_to.y = a1->draw_to.y + ratio * (a2->draw_to.y - a1->draw_to.y);
    bgl_action_with_style_setRatio(&act->hdr_styles, &a1->hdr_styles, &a2->hdr_styles, ratio);

    return (bgl_action_t *) act;
}

/*! Creates a new drawcircle action interpolation. This function chould not be
 *  called explicitly. 
 *
 *  \param a1       a pointer to a drawcircle action.
 *  \param a2       a pointer to a drawcircle action.
 *  \param ratio    an integer that represent the ratio for the interpolation.
 *
 * \return A newly allocated drawcircle action.
 */
static bgl_action_t *
bgl_action_drawcircle_setRatio (bgl_action_drawcircle_t *a1,
                                bgl_action_drawcircle_t *a2,
                                float ratio) {
    bgl_action_drawcircle_t *act =
        (bgl_action_drawcircle_t *) bgl_action_new (BGL_ACTION_DRAW_CIRCLE);
    act->radius = a1->radius + ratio * (a2->radius - a1->radius);
    bgl_action_with_style_setRatio(&act->hdr_styles, &a1->hdr_styles, &a2->hdr_styles, ratio);

    return (bgl_action_t *) act;
}

/*! Creates a new drawcurve action interpolation. This function chould not be
 *  called explicitly. 
 *
 *  \param a1       a pointer to a drawcurve action.
 *  \param a2       a pointer to a drawcurve action.
 *  \param ratio    an integer that represent the ratio for the interpolation.
 *
 * \return A newly allocated drawcurve action.
 */
static bgl_action_t *
bgl_action_drawcurve_setRatio (bgl_action_drawcurve_t *a1,
                               bgl_action_drawcurve_t *a2,
                                float ratio) {
    bgl_action_drawcurve_t *act =
        (bgl_action_drawcurve_t *) bgl_action_new (BGL_ACTION_DRAW_CURVE);
    act->control.x = a1->control.x + ratio * (a2->control.x - a1->control.x);
    act->control.y = a1->control.y + ratio * (a2->control.y - a1->control.y);
    act->anchor.x = a1->anchor.x + ratio * (a2->anchor.x - a1->anchor.x);
    act->anchor.y = a1->anchor.y + ratio * (a2->anchor.y - a1->anchor.y);
    bgl_action_with_style_setRatio(&act->hdr_styles, &a1->hdr_styles, &a2->hdr_styles, ratio);

    return (bgl_action_t *) act;
}



/*! Creates a new action interpolation. both actions whould have the same type.
 *
 *  \param a1       a pointer to an action.
 *  \param a2       a pointer to an action.
 *  \param ratio    an integer that represent the ratio for the interpolation.
 *
 * \return A newly allocated action.
 */
static bgl_action_t *
bgl_action_setRatio (bgl_action_t *a1, bgl_action_t *a2, float ratio) {
    static setRatio_fn_t setRatio_Functions[BGL_ACTION_COUNT] = {
        [BGL_ACTION_MOVE_PEN]    (setRatio_fn_t) bgl_action_movepen_setRatio,
        [BGL_ACTION_DRAW_LINE]   (setRatio_fn_t) bgl_action_drawline_setRatio,
        [BGL_ACTION_DRAW_CIRCLE] (setRatio_fn_t) bgl_action_drawcircle_setRatio,
        [BGL_ACTION_DRAW_CURVE]  (setRatio_fn_t) bgl_action_drawcurve_setRatio,
        [BGL_ACTION_DRAW_GLYPH]  (setRatio_fn_t) NULL
    };

    setRatio_fn_t fun;

    if (a1->type == a2->type) {
        fun = setRatio_Functions[a1->type];
        if (fun)
            return fun (a1, a2, ratio);
        else if (ratio < 0.5)
            return bgl_action_copy (a1);
        else
            return bgl_action_copy (a2);
    }

    /* If different types, return a copy of the first (arbitrary) */
    /* shouldn't ever happen */
    abort();
    return bgl_action_copy (a1);
}



/*! Sets the style of an action.
 *  
 *  \param action   A valid pointer to an action.
 *  \param fill     A pointer to a fill style structure. Can be null.
 *  \param line     A valid pointer to a line style structure.
 *
 *  \sa BGLLine_init(), BGLFillStyle_init() and newBGLFillStyle().
 */
static void
bgl_action_set_style (bgl_action_with_style_t *action,
                      struct BGLFillStyle *fill,
                      struct BGLLineStyle *line) {
    action->fill = fill;
    memcpy (&action->line, line, sizeof (*line));
}


/*! Destroy an action. The structure pointed by \a fill (if any) will not be
 *  destroyed.
 *  
 *  \param action   A valid pointer to an action structure, or NULL.
 *
 *  \sa bgl_action_new().
 */
#if 0
static void
bgl_action_free (bgl_action_t *action) {
    if (action)
        free (action);
}
#endif



/* *******************************************************************
 * Block methods
 */

/*! Initializes a block structure.
 *
 *  \param block    A valid pointer to a block structure.
 *  \param type     The type of the block. 
 *  \param free_fn  The free function to use for this block. This function
 *                  should release all memory used by the block.
 */
static void
BGLBlock_init (BubbleBlock block, bgl_block_type_e_t type,
               free_block_fn_t free_fn) {
    block->type = type;
    block->ref = 0;
    block->free_fn = free_fn;
}


/*! Increases the reference count of a block.
 *
 *  \param block    A valid pointer to a block structure.
 *
 *  \return A pointer to the block passed in argument. 
 */
static BubbleBlock
BGLBlock_ref (BubbleBlock block) {
    if (block)
        block->ref++;
    return block;
}

/*! Decreases the reference count of a block. When the reference count reaches
 *  0, the associated free function is called.
 *
 *  \param block    A valid pointer to a block structure.
 *
 *  \sa BGLBlock_init().
 */
static void
BGLBlock_unref (BubbleBlock block) {
    if (!block)
        return;

    block->ref--;
    if (block->ref <= 0 && block->free_fn) {
        block->free_fn (block);
    }
}



/* *******************************************************************
 * DisplayItem methods
 */

/*! Creates a new display item.
 *
 *  \param block    A valid pointer to a block structure.
 *
 *  \return a new allocated BubbleDisplayItem structure.
 *
 *  \todo Check if all fields are initialized.
 */
static BubbleDisplayItem
newBGLDisplayItem (BubbleBlock block) {
    IN();
    BubbleDisplayItem new_item = malloc (sizeof (*new_item));

    INIT_LIST_HEAD (&new_item->disp_list);

    new_item->block = BGLBlock_ref (block);

    if (block->type == BGL_BLOCK_TYPE_MORPH)
        new_item->disp_block =
            BGLBlock_ref ((struct BGLBlock *)((struct BGLMorph *) block)->shape1);
    else
        new_item->disp_block = BGLBlock_ref (block);        

    new_item->current.x = new_item->current.y = 0;
    new_item->scale.x = new_item->scale.y = 1.0;

    OUT();
    return new_item;
}


/*! Creates a copy of a display item.
 *
 *  \param item     A valid pointer to a display item structure.
 *
 *  \return A new allocated BubbleDisplayItem structure.
 */
static BubbleDisplayItem
BGLDisplayItem_copy (BubbleDisplayItem item) {
    IN();
    BubbleDisplayItem new_item = newBGLDisplayItem (item->block);
    new_item->current.x = item->current.x;
    new_item->current.y = item->current.y;
    new_item->scale.x = item->scale.x;
    new_item->scale.y = item->scale.y;
    if (new_item->disp_block)
        BGLBlock_unref (new_item->disp_block);
    new_item->disp_block = BGLBlock_ref(item->disp_block);

    OUT();
    return new_item;
}


/*! Destroy a display item.
 *
 *  \param item     A valid pointer to a display item structure.
 */
static void
destroyBGLDisplayItem (BubbleDisplayItem item) {
    IN();
    list_del (&item->disp_list);
    BGLBlock_unref (item->disp_block);
    BGLBlock_unref (item->block);
    free (item);
    OUT();
}


/*! Sets the rotation. It is applied to the block displayed by the
 * display item.
 *
 *  \param item     A valid pointer to a display item structure.
 *  \param degrees  A float value that represents, in degrees, the angle of the
 *                  rotation. Positive values stand for rotations in
 *                  trigonometric direction.
 *
 *  \todo Not yet implemented.
 */
static void
BGLDisplayItem_rotateTo (BubbleDisplayItem item, coordinate_t degrees) {
    IN();
    OUT();
}


/*! \copydoc destroyBGLDisplayItem()
 *
 *  \sa destroyBGLDisplayItem().
 *  \todo To remove.
 */
static void
BGLDisplayItem_remove (BubbleDisplayItem item) {
    destroyBGLDisplayItem (item);
}


/*! Sets the position of an item.
 *
 *  \param item     A valid pointer to a display item structure.
 *  \param x        A float that represents the abscissa position.
 *  \param y        A float that represnets the ordinate position.
 */
static void
BGLDisplayItem_moveTo (BubbleDisplayItem item, coordinate_t x, coordinate_t y) {
    IN();
    item->current.x = x;
    item->current.y = y;
    MYTRACE("x = %f -- y = %f", x, y);
    OUT();
}


static BubbleShape newBGLShape ();

/*! Sets the ratio of a morph to be displayed. This function has an effect only
 *  if the display item is associated to a BGLMorph structure.
 *
 *  \param item     A valid pointer to a display item structure.
 *  \param ratio    A float that represents the ratio of the morph to display.
 *
 *  \todo Not yet implemented. Currently, this function displays only begining
 *        or ending shape depending on ratio value.
 */
static void 
BGLDisplayItem_setRatio (BubbleDisplayItem item, float ratio) {
    IN();
    BubbleMorph morph  = NULL;
    BubbleShape shape1 = NULL;
    BubbleShape shape2 = NULL;
    BubbleShape shp    = NULL;

    bgl_action_t *a1;
    bgl_action_t *a2;

    if (!item) {
        OUT();
        return;
    }

    if (item->block->type != BGL_BLOCK_TYPE_MORPH) {
	    abort();
        OUT();
		return ;
    }

    morph = (BubbleMorph) item->block;
    shape1 = morph->shape1;
    shape2 = morph->shape2;

    if (ratio < 0.0f)
        ratio = 0.0f;
    else if (ratio > 1.0f)
        ratio = 1.0f;

    if (item->disp_block) {
        BGLBlock_unref (item->disp_block);
        item->disp_block = NULL;
    }

#if 1
    /* Morph implementation */
    shp = newBGLShape();

    a1 = list_entry (shape1->actions.next, typeof (*a1), action_list);
    a2 = list_entry (shape2->actions.next, typeof (*a2), action_list);

    while(&(a1->action_list) != &(shape1->actions) &&
          &(a2->action_list) != &(shape2->actions)) {

        bgl_action_t *nact = bgl_action_setRatio (a1, a2, ratio);
        
        list_add_tail (&nact->action_list, &shp->actions);

        a1 = list_entry (a1->action_list.next, typeof (*a1), action_list);
        a2 = list_entry (a2->action_list.next, typeof (*a2), action_list);
    }

    item->disp_block = BGLBlock_ref ((BubbleBlock) shp);
#else
    /* Simplified morph display for tests. */
    if (ratio < 0.5f) {
        item->disp_block = BGLBlock_ref ((BubbleBlock) shape1);
    } else {
        item->disp_block = BGLBlock_ref ((BubbleBlock) shape2);
    }
#endif

    OUT();
}



/* *******************************************************************
 * Frame methods
 */

/*! Creates a new frame structure.
 *
 *  \return a new allocated #bgl_frame_t structure.
 */
static bgl_frame_t *
bgl_frame_new () {
    bgl_frame_t *new_frame = malloc (sizeof (*new_frame));
    INIT_LIST_HEAD (&new_frame->frame_list);
    new_frame->duration = 0;
    INIT_LIST_HEAD (&new_frame->display_items);
    new_frame->status = "";

    return new_frame;
}


/*! Creates a copy of a frame.
 *
 *  \param frame    A valid pointer to a frame structure.
 *
 *  \return a new allocated #bgl_frame_t structure.
 */
static bgl_frame_t *
bgl_frame_copy (bgl_frame_t *frame) {
    BubbleDisplayItem citem;
    BubbleDisplayItem cpy;

    bgl_frame_t *new_frame = bgl_frame_new();
    new_frame->duration = frame->duration;

    list_for_each_entry (citem, &frame->display_items, disp_list) {
        cpy = BGLDisplayItem_copy (citem);
        list_add_tail (&cpy->disp_list, &new_frame->display_items);
    }
    new_frame->status = frame->status;

    return new_frame;
}


/*! Destroy a frame.
 *
 *  \param frame    A valid pointer to a frame structure.
 *
 *  \pre The frame has been removed from the frame_list it belonged to.
 */
static void
bgl_frame_free (bgl_frame_t *frame) {
    BubbleDisplayItem item, next;

    list_for_each_entry_safe (item, next, &frame->display_items, disp_list)
        destroyBGLDisplayItem (item);

    free (frame);
}



/* *******************************************************************
 * Movie methods
 */

static void BGLMovie_nextFrame (BubbleMovie movie);

/*! Gets the current frame in the movie (actually the last frame). If no frame
 *  exists already, a new one is created.
 *
 *  \param movie    A valid pointer to a movie structure.
 *
 *  \return a pointer to a #bgl_frame_t structure.
 */
static bgl_frame_t *
BGLMovie_get_current_frame (BubbleMovie movie) {
    if (list_empty (&movie->frames)) /* creates default frame */
        BGLMovie_nextFrame (movie);
    return list_entry (movie->frames.prev, bgl_frame_t, frame_list);
}


/*! Creates a new movie.
 *
 *  \return a new allocated #BGLMovie structure.
 *
 *  \todo Check if all fields are initialized.
 */
static BubbleMovie
newBGLMovie (void) {
    IN();
    BubbleMovie movie = malloc (sizeof (*movie));

    INIT_LIST_HEAD (&movie->frames);

    movie->frames_count = 0;
    movie->frames_array = NULL;

    movie->playing = 0;

    /* arbitrary defaults. */
    movie->height = MOVIEY;
    movie->width  = MOVIEX;

    OUT();
    return movie;
}


/*! Sets movie status.
 *
 *  \param movie    A valid pointer to a movie structure.
 *  \param yes      An integer that indicate the movie status. A non null value
 *                  indicates that the movie is playing (we can add frames).
 */
static void
BGLMovie_startPlaying (BubbleMovie movie, int yes) {
    IN();
    movie->playing = yes;
    OUT();
}

/*! Causes the movie generation to abort.
 *
 *  \param movie    A valid pointer to a movie structure.
 *
 *  \todo Define what must be done in this function.
 */
static void
BGLMovie_abort (BubbleMovie movie) {
    error ("aborting");
}

/*! Moves to the next frame in the movie. Actually, The current frame is saved
 * and a new one is created.
 *
 *  \param movie    A valid pointer to a movie structure.
 */
static void
BGLMovie_nextFrame (BubbleMovie movie) {
    IN();
    bgl_frame_t *cframe = NULL;
    bgl_frame_t *nframe = NULL;

    if (list_empty (&movie->frames)) {
        nframe = bgl_frame_new ();
    } else {
        if (!movie->playing)
            return;

        cframe = BGLMovie_get_current_frame (movie);
        nframe = bgl_frame_copy (cframe);
    }

    /* cframe is always kept as the last frame in order to maintain
     * external BGLDisplayItem pointers valid.
     */
    if (cframe)
		list_del (&cframe->frame_list);

    if (nframe)
		list_add_tail (&nframe->frame_list, &movie->frames);

    if (cframe)
		{
            cframe->duration = 0;
            list_add_tail (&cframe->frame_list, &movie->frames);
		}
    OUT();
}


/*! Adds a block to be displayed in the current frame.
 *
 *  \param movie    A valid pointer to a movie structure.
 *  \param block    A valid pointer to a block structure.
 *
 *  \return a new allocated display item associated to the block.
 */
BubbleDisplayItem
BGLMovie_add (BubbleMovie movie, BubbleBlock block) {
    IN();
    bgl_frame_t *cframe;
    BubbleDisplayItem new_item;

    cframe = BGLMovie_get_current_frame (movie);
    new_item = newBGLDisplayItem (block);
    list_add_tail (&new_item->disp_list, &cframe->display_items);

    OUT();
    return new_item;
}


/*! Sets the time the current frame must be displayed.
 *
 *  \param movie    A valid pointer to a movie structure.
 *  \param sec      A float tha represents the duration of the frame in
 *                  seconds.
 */
static void
BGLMovie_pause (BubbleMovie movie, coordinate_t sec) {
    IN();
    BGLMovie_get_current_frame (movie)->duration += sec;
    OUT();
}


#ifdef BUBBLE_GL_DEBUG
static void
bgl_print_frame_tree (bgl_frame_t *frame) {
    printf ("    Frame duration = %f\n", frame->duration);
    if (list_empty (&frame->display_items)) {
        printf ("  ! Frame is Empty!\n");
        return;
    }
    struct BGLDisplayItem *citem;
    bgl_action_t         *caction;
    list_for_each_entry (citem, &frame->display_items, disp_list) {
        printf ("    === Item :\n");
        if (!citem->block) {
            printf ("     !! Goups !\n");
            continue;
        }
        printf ("      - BlockType : %s\n",
                 citem->block->type == BGL_BLOCK_TYPE_SHAPE ? "SHAPE" : "MORPH");
        printf ("      - OffSet : x=%f, y=%f\n", citem->current.x, citem->current.y);
        printf ("      - Actions :\n");
        if (list_empty (&((struct BGLShape *) citem->disp_block)->actions)) {
            printf ("        ! No Actions !\n");
            printf ("      - ----\n");
            fflush (stdout);
            continue;
        }
        fflush (stdout);
        list_for_each_entry (caction,
                             &((struct BGLShape *) citem->disp_block)->actions,
                             action_list) {
            bgl_action_with_style_t *style;
            switch (caction->type) {
            case BGL_ACTION_MOVE_PEN:
                printf ("        * Move Pen : x=%f, y=%f\n",
                         ((bgl_action_movepen_t *)caction)->position.x,
                         ((bgl_action_movepen_t *)caction)->position.y);
                break;
            case BGL_ACTION_DRAW_LINE:
                printf ("        * Line     : to_x=%f, to_y=%f\n",
                         ((bgl_action_drawline_t *)caction)->draw_to.x,
                         ((bgl_action_drawline_t *)caction)->draw_to.y);
                break;
            case BGL_ACTION_DRAW_CIRCLE:
                printf ("        * Circle   : radius=%f\n",
                         ((bgl_action_drawcircle_t *)caction)->radius);
                break;
            case BGL_ACTION_DRAW_CURVE:
                printf ("        * Curve    : c_x=%f, c_y=%f\n",
                         ((bgl_action_drawcurve_t *)caction)->control.x,
                         ((bgl_action_drawcurve_t *)caction)->control.y);
                printf ("                     a_x=%f, a_y=%f\n",
                         ((bgl_action_drawcurve_t *)caction)->anchor.x,
                         ((bgl_action_drawcurve_t *)caction)->anchor.y);
                break;
            case BGL_ACTION_DRAW_GLYPH:
                printf ("        * Glyph\n");
                break;
            default:
                printf ("     ! Unknown action !\n");
            }
            switch (caction->type) {
            case BGL_ACTION_DRAW_LINE:
            case BGL_ACTION_DRAW_CIRCLE:
            case BGL_ACTION_DRAW_CURVE:
            case BGL_ACTION_DRAW_GLYPH:
                style = (bgl_action_with_style_t *)caction;
                printf ("          _ Fill Style : ");
                if (!style->fill)
                    printf ("[NULL]");
                else {
                    printf ("r=%d, g=%d, b=%d, a=%d",
                             style->fill->color.r, style->fill->color.g,
                             style->fill->color.b, style->fill->color.a);
                }
                printf ("\n          _ Line Style : ");
                printf ("w=%d -- r=%d, g=%d, b=%d, a=%d\n",
                         style->line.width,
                         style->line.color.r, style->line.color.g,
                         style->line.color.b, style->line.color.a);
            default:
                printf ("        * ****\n");
            }
            fflush (stdout);
        }
        printf ("      - ----\n");
        fflush (stdout);
    }
}
#endif

/*! Saves the movie. Actually, it builds data structures that optimizes movie
 *  playing control features.
 *
 *  \param movie    A valid pointer to a movie structure.
 *  \param filename Not used.
 *
 *  \return 0 on success, -1 on error.
 */
static int
BGLMovie_save (BubbleMovie movie, const char *filename TBX_UNUSED) {
    IN();
    bgl_frame_t *cframe = NULL;
    int i;

    /* Free previous save if any. */
    free (movie->frames_array);
    movie->frames_array = NULL;
    movie->frames_count = 0;

    if (list_empty (&movie->frames)) /* Empty movie. */
        return 0;

    /* get frames count. */
    list_for_each_entry (cframe, &movie->frames, frame_list)
        movie->frames_count++;

    movie->frames_array = malloc (movie->frames_count * sizeof (bgl_frame_t *));

    i = 0;
    list_for_each_entry (cframe, &movie->frames, frame_list) {
        movie->frames_array[i] = cframe;
        i++;

        if (cframe->duration == 0)
            cframe->duration = 1. / RATE;

#ifdef BUBBLE_GL_DEBUG
        printf ("=== Frame %d\n", i);
        bgl_print_frame_tree(cframe);
        printf ("  = ====\n");
        fflush (stdout);
#endif
    }

    movie->current_frame = 0;

    OUT();
    return 0;
}



/*! Destroys a movie structure. It releases all memory used by movie
 * components. This means that 
 *
 *  \param movie    A pointer to a movie or NULL.
 *
 *  \post All pointers to structures (like BGLShape, BGLMorph or BGLFillStyle)
 *        that are involved in the movie will become invalid and thus should no
 *        longer be used.
 */
void
destroyBGLMovie (BubbleMovie movie) {
    IN();
    bgl_frame_t *frame;

    if (!movie)
        return;

    free (movie->frames_array);

    while (!list_empty (&movie->frames)) {
        frame = list_entry (movie->frames.next, bgl_frame_t, frame_list);
        list_del (&frame->frame_list);
        bgl_frame_free (frame);
    }

    free (movie);
    OUT();
}

void
BGLMovie_status (BubbleMovie movie, const char *str) {
    IN();
    bgl_frame_t *frame = BGLMovie_get_current_frame (movie);
    frame->status = strdup(str);
    OUT();
}

/* *******************************************************************
 * Style methods
 */

/*! Initializes a color structure.
 *
 *  \param color    A valid pointer to a #BGLColor structure.
 *  \param r        A byte that represents the red component of the color.
 *  \param g        A byte that represents the green component of the color.
 *  \param b        A byte that represents the blue component of the color.
 *  \param a        A byte that represents the alpha component of the color.
 */
static void
BGLColor_init (struct BGLColor *color, unsigned char r, unsigned char g,
               unsigned char b, unsigned char a) {
    color->r = r;
    color->g = g;
    color->b = b;
    color->a = a;
}


/*! Initializes a line style structure.
 *
 *  \param line     A valid pointer to a BGLLineStyle structure.
 *  \param width    An unsigned short that represents the width of the line. 
 *  \param r        A byte that represents the red style's color component.
 *  \param g        A byte that represents the green style's color component.
 *  \param b        A byte that represents the blue style's color component.
 *  \param a        A byte that represents the alpha style's color component.
 */
static void
BGLLine_init (struct BGLLineStyle *line, unsigned short width, unsigned char r,
              unsigned char g, unsigned char b, unsigned char a) {
    line->width = width;
    BGLColor_init (&line->color, r, g, b, a);
}


/*! Initializes a fill style structure.
 *
 *  \param style    A valid pointer to a BGLFillStyle structure.
 *  \param r        A byte that represents the red style's color component.
 *  \param g        A byte that represents the green style's color component.
 *  \param b        A byte that represents the blue style's color component.
 *  \param a        A byte that represents the alpha style's color component.
 */
static void
BGLFillStyle_init (BubbleFillStyle style,
                   unsigned char r, unsigned char g, unsigned char b,
                   unsigned char a) {
    INIT_LIST_HEAD (&style->style_list);
    BGLColor_init (&style->color, r, g, b, a);
}


/*! Creates a new fill style.
 *
 *  \param r        A byte that represents the red style's color component.
 *  \param g        A byte that represents the green style's color component.
 *  \param b        A byte that represents the blue style's color component.
 *  \param a        A byte that represents the alpha style's color component.
 *
 *  \return A pointer to a new allocated #BGLFillStyle structure.
 */
static BubbleFillStyle
newBGLFillStyle (unsigned char r, unsigned char g, unsigned char b,
                 unsigned char a) {
    BubbleFillStyle new_style = malloc (sizeof (*new_style));
    if (new_style != NULL)
        BGLFillStyle_init (new_style, r, g, b, a);
    return new_style;
}


/*! Creates a new fill style in a shape.
 *
 *  \param shape    A valid pointer to a #BGLShape structure.
 *  \param r        A byte that represents the red style's color component.
 *  \param g        A byte that represents the green style's color component.
 *  \param b        A byte that represents the blue style's color component.
 *  \param a        A byte that represents the alpha style's color component.
 *
 *  \return A pointer to a #BGLFillStyle structure. It has been allocated inside
 *          the shape and thus should never be externally destroyed.
 */
static BubbleFillStyle
BGLShape_addSolidFillStyle (BubbleShape shape,
                            unsigned char r, unsigned char g, unsigned char b,
                            unsigned char a) {
    IN();
    BubbleFillStyle new_style = newBGLFillStyle (r, g, b, a);

    list_add (&new_style->style_list, &shape->styles);

    OUT();
    return new_style;
}


/*********************************************************************
 * Shape methods
 */

/*! Destroys a shape. It also releases all memory dynamically allocated throught
 * it, including #BGLFillStyle structures that can have external references that
 * should no longer be used.
 *
 * \param shape     A valid pointer to a #BGLShape structure.
 *
 * \todo not yet implemented.
 * \warning Memory leak !!
 */
static void
__destroyBGLShape (BubbleShape shape) {
    
}

/*! Destroys a shape. Actually, it decreases the reference counter of the
 * shape, that will be destroyed when this counter reaches 0, by calling
 * __destroyBGLShape().
 *
 *  \param shape    A valid pointer to a #BGLShape structure.
 * 
 *  \sa __destroyBGLShape().
 */
static void
destroyBGLShape (BubbleShape shape) {
    /*   __destroyBGLShape (shape); */
    BGLBlock_unref ((BubbleBlock) shape);
}


/*! Adds a new action in a shape.
 *
 *  \param shape    A valid pointer to a #BGLShape structure.
 *  \param action_type
 *                  The type of action to add.
 *
 *  \return A pointer to a #bgl_action_t structure. Should not be externally
 *          freed.
 */
static bgl_action_t *
BGLShape_addAction (BubbleShape shape, bgl_action_type_e_t action_type) {
    bgl_action_t *new_action = bgl_action_new (action_type);
    list_add_tail (&new_action->action_list, &shape->actions);

    return new_action;
}


/*! Creates a new shape.
 *
 *  \return a pointer to a new allocated #BGLShape structure.
 */
static BubbleShape
newBGLShape () {
    IN();
    BubbleShape new_shape = malloc (sizeof (*new_shape));
    BGLBlock_init ((BubbleBlock) new_shape, BGL_BLOCK_TYPE_SHAPE,
                   (free_block_fn_t) __destroyBGLShape);

    INIT_LIST_HEAD (&new_shape->actions);

    new_shape->orig.x = new_shape->orig.y = 0;

    INIT_LIST_HEAD (&new_shape->styles);
    new_shape->current_style = NULL;

    BGLLine_init (&new_shape->line, 1, 0, 0, 0, 255);

    new_shape->pen.x = new_shape->pen.y = 0;

    OUT();
    return new_shape;
}


/*! Sets the style of the shape to be used for next actions.
 *
 *  \param shape    A valid pointer to a #BGLShape structure.
 *  \param style    A valid pointer to a #BGLFillStyle structure. This style
 *                  should have been created in the shape pointed by \a shape.
 */
static void
BGLShape_setRightFillStyle (BubbleShape shape, BubbleFillStyle style) {
    IN();
    shape->current_style = style;
    OUT();
}


/*! Moves the pen to an absolute position.
 *
 *  \param shape    a valid pointer to a #BGLShape structure or NULL.
 *  \param x        a float that represents the absolute abscissa.
 *  \param y        a float that represents the absolute ordinate.
 *
 *  \sa BGLShape_movePen().
 */
static void
BGLShape_movePenTo (BubbleShape shape, coordinate_t x, coordinate_t y) {
    IN();
    bgl_action_movepen_t *action;

    if (!shape) return;

    shape->pen.x = x;
    shape->pen.y = y;

    action = (bgl_action_movepen_t *)
        BGLShape_addAction (shape, BGL_ACTION_MOVE_PEN);

    if (!action) return;

    action->position.x = x;
    action->position.y = y;
    OUT();
}

/*! Moves the pen to a relative position from the current.
 *
 *  \param shape    a valid pointer to a #BGLShape structure or NULL.
 *  \param dx       a float that represents the relative abscissa.
 *  \param dy       a float that represents the relative ordinate.
 *
 *  \sa BGLShape_movePenTo().
 */
static void
BGLShape_movePen (BubbleShape shape, coordinate_t dx, coordinate_t dy) {
    IN();
    BGLShape_movePenTo (shape, shape->pen.x + dx, shape->pen.y + dy);
    OUT();
}


/*! Draws a line to an absolute position.
 *
 *  \param shape    a valid pointer to a #BGLShape structure or NULL.
 *  \param x        a float that represents the absolute abscissa.
 *  \param y        a float that represents the absolute ordinate.
 *
 *  \sa BGLShape_drawLine().
 */
static void
BGLShape_drawLineTo (BubbleShape shape, coordinate_t x, coordinate_t y) {
    IN();
    bgl_action_drawline_t *action;

    if (!shape) return;

    shape->pen.x = x;
    shape->pen.y = y;

    action = (bgl_action_drawline_t *)
        BGLShape_addAction (shape, BGL_ACTION_DRAW_LINE);

    if (!action) return;

    bgl_action_set_style (&action->hdr_styles,
                          shape->current_style, &shape->line);

    action->draw_to.x = x;
    action->draw_to.y = y;
    OUT();
}

/*! Draws a line to a relative position from the current.
 *
 *  \param shape    a valid pointer to a #BGLShape structure or NULL.
 *  \param dx       a float that represents the relative abscissa.
 *  \param dy       a float that represents the relative ordinate.
 *
 *  \sa BGLShape_drawLineTo().
 */
static void
BGLShape_drawLine (BubbleShape shape, coordinate_t dx, coordinate_t dy) {
    IN();
    BGLShape_drawLineTo (shape, shape->pen.x + dx, shape->pen.y + dy);
    OUT();
}


/*! Draws a circle.
 *
 *  \param shape    A valid pointer to #BGLShape structure or NULL.
 *  \param r        A float that represents the radius of the circle.
 */
static void
BGLShape_drawCircle (BubbleShape shape, coordinate_t r) {
    IN();
    bgl_action_drawcircle_t *action;

    if (!shape) return;

    action = (bgl_action_drawcircle_t *)
        BGLShape_addAction (shape, BGL_ACTION_DRAW_CIRCLE);

    if (!action) return;

    bgl_action_set_style (&action->hdr_styles,
                          shape->current_style, &shape->line);

    action->radius = r;
    OUT();
}


/*! Sets the shape line style for next actions.
 *
 *  \param shape    A valid pointer to a #BGLShape structure or NULL.
 *  \param width    An unsigned short that represents the width of the line. 
 *  \param r        A byte that represents the red line's color component.
 *  \param g        A byte that represents the green line's color component.
 *  \param b        A byte that represents the blue line's color component.
 *  \param a        A byte that represents the alpha line's color component.
 */
static void
BGLShape_setLine (BubbleShape shape, unsigned short width,
                  unsigned char r, unsigned char g, unsigned char b,
                  unsigned char a) {
    IN();
    BGLLine_init (&shape->line, width, r, g, b, a);
    OUT();
}


/*! Draws a curve.
 *
 *  \param shape    A valid pointer to a #BGLShape structure or NULL.
 *  \param controldx
 *                  A float that represents the relative abscissa of the control
 *                  point.
 *  \param controldy
 *                  A float that represents the relative ordinate of the control
 *                  point. 
 *  \param anchordx A float that represents the relative abscissa (from the
 *                  control point) of the anchor point.
 *  \param anchordy A float that represents the relative ordinate (from the
 *                  control point) of the anchor point.
 */
static void
BGLShape_drawCurve (BubbleShape shape,
                    coordinate_t controldx, coordinate_t controldy,
                    coordinate_t anchordx, coordinate_t anchordy) {
    IN();
    bgl_action_drawcurve_t *action;

    if (!shape) return;

    action = (bgl_action_drawcurve_t *)
        BGLShape_addAction (shape, BGL_ACTION_DRAW_CURVE);

    if (!action) {
        shape->pen.x += controldx + anchordx;
        shape->pen.y += controldy + anchordy;
        return;
    }
  
    bgl_action_set_style (&action->hdr_styles,
                          shape->current_style, &shape->line);

    action->control.x = shape->pen.x + controldx;
    action->control.y = shape->pen.y + controldy;
  
    shape->pen.x += controldx;
    shape->pen.y += controldy;
  
    action->anchor.x = shape->pen.x + anchordx;
    action->anchor.y = shape->pen.y + anchordy;

    shape->pen.x += anchordx;
    shape->pen.y += anchordy;
    OUT();
}


/*! Prints a glyph.
 *
 *  \param shape    A valid pointer to a #BGLShape structure or NULL.
 *  \param c        a short that represents the glyph to print.
 *  \param size     an integer that represents the size of the glyph.
 */
static void
BGLShape_drawSizedGlyph (BubbleShape shape, unsigned short c, int size) {
    IN();
    bgl_action_drawglyph_t *action;

    if (!shape) return;

    action = (bgl_action_drawglyph_t *)
        BGLShape_addAction (shape, BGL_ACTION_DRAW_GLYPH);

    if (!action) return;

    bgl_action_set_style (&action->hdr_styles,
                          shape->current_style, &shape->line);

    action->glyph = c;
    action->gsize = size;
    OUT();
}



/*********************************************************************
 * BGLMorph methods.
 */

/*! Destroys a morph. It also releases all memory dynamically allocated throught
 * it, including #BGLShape structures that can have external references that
 * should no longer be used.
 *
 *  \param morph     A valid pointer to a #BGLMorph structure.
 */
static void
__destroyBGLMorph (BubbleMorph morph) {
    destroyBGLShape (morph->shape1);
    destroyBGLShape (morph->shape2);
    free (morph);
}

void
destroyBGLMorph (BubbleMorph morph) {
    /*   __destroyBGLMorph (morph); */
    BGLBlock_unref ((BubbleBlock) morph);
}

/*! Destroys a morph. Actually, it decreases the reference counter of the shape,
 * that will be destroyed when this counter reaches 0, by calling
 * __destroyBGLMorph().
 *
 *  \param morph     A valid pointer to a #BGLMorph structure.
 * 
 *  \sa __destroyBGLMorph().
 */
static BubbleMorph
newBGLMorphShape () {
    IN();
    BubbleMorph new_morph = malloc (sizeof (*new_morph));
    BGLBlock_init ((BubbleBlock) new_morph, BGL_BLOCK_TYPE_MORPH,
                   (free_block_fn_t) __destroyBGLMorph);
    new_morph->shape1 = (BubbleShape) BGLBlock_ref ((BubbleBlock)newBGLShape());
    new_morph->shape2 = (BubbleShape) BGLBlock_ref ((BubbleBlock)newBGLShape());

    OUT();
    return new_morph;
}


/*! Gets the begining shape of a morph object.
 *
 *  \param morph    A valid pointer to a #BGLMorph structure.
 *
 *  \return A pointer to a #BGLShape. This shape must not be destroyed.
 */
static BubbleShape
BGLMorph_getShape1 (BubbleMorph morph) {
    IN(); OUT();
    return morph->shape1;
}

/*! Gets the ending shape of a morph object.
 *
 *  \param morph    A valid pointer to a #BGLMorph structure.
 *
 *  \return A pointer to a #BGLShape. This shape must not be destroyed.
 */
static BubbleShape
BGLMorph_getShape2 (BubbleMorph morph) {
    IN(); OUT();
    return morph->shape2;
}



/*********************************************************************
 * Thread/Bubble-specific methods
 */

/*! Sets a thread boundary box.
 *
 *  /todo Not yet implemented. Propotype must be redesigned before.
 */
static void
BGLsetThread (BubbleShape shape, int id, coordinate_t x, coordinate_t y,
              coordinate_t width, coordinate_t height) {
    IN();
    OUT();
}

/*! Sets a bubble boundary box.
 *
 *  /todo Not yet implemented. Propotype must be redesigned before.
 */
static void
BGLsetBubble (BubbleShape shape, int id, coordinate_t x, coordinate_t y,
              coordinate_t width, coordinate_t height) {
    IN();
    OUT();
}


/*********************************************************************
 * misc methods
 */

/*! Initializes BGL module.
 *
 *  /todo Not yet implemented. Is there any data to be initialized here ?
 */
static void
BGLinit () {
    IN();
    OUT();
}

/*! Finalizes BGL module.
 *
 *  /todo Not yet implemented. Is there any data to be freed here ?
 */

static void
BGLfini () {
}

/*! Maps bubbles ops to BGL interface.
 */
void
BubbleOps_setBGL() {
    IN();
    static BubbleOps BGLBubbleOps = {
        /* Movie methods */
        .newMovie = newBGLMovie,
        .Movie_startPlaying = BGLMovie_startPlaying,
        .Movie_abort = BGLMovie_abort,
        .Movie_nextFrame = BGLMovie_nextFrame,
        .Movie_add = BGLMovie_add,
        .Movie_pause = BGLMovie_pause,
        .Movie_save = BGLMovie_save,
        .Movie_status = BGLMovie_status,
	 
        /* Shape methods */
        .newShape = newBGLShape,
        .Shape_setRightFillStyle = BGLShape_setRightFillStyle,
        .Shape_movePenTo = BGLShape_movePenTo,
        .Shape_movePen = BGLShape_movePen,
        .Shape_drawLineTo = BGLShape_drawLineTo,
        .Shape_drawLine = BGLShape_drawLine,
        .Shape_drawCircle = BGLShape_drawCircle,
        .Shape_setLine = BGLShape_setLine,
        .Shape_drawCurve = BGLShape_drawCurve,
        .Shape_drawSizedGlyph = BGLShape_drawSizedGlyph,
	 
        /* FillStyle methods */
        .Shape_addSolidFillStyle = BGLShape_addSolidFillStyle,
	 
        /* DisplayItem methods */
        .DisplayItem_rotateTo = BGLDisplayItem_rotateTo,
        .DisplayItem_remove = BGLDisplayItem_remove,
        .DisplayItem_moveTo = BGLDisplayItem_moveTo,
        .DisplayItem_setRatio = BGLDisplayItem_setRatio,
	 
        /* Morph methods */
        .newMorphShape = newBGLMorphShape,
        .Morph_getShape1 = BGLMorph_getShape1,
        .Morph_getShape2 = BGLMorph_getShape2,

        /* Misc methods */
        .init = BGLinit,
        .fini = BGLfini,
	 
        /* Thread/Bubble-specific methods */
        .SetThread = BGLsetThread,
        .SetBubble = BGLsetBubble
    };

    curBubbleOps = &BGLBubbleOps;
    OUT();
}
