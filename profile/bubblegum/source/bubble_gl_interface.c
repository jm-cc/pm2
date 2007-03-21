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

#include <stdio.h>
#include <signal.h>
#include <stdarg.h>
#include <stdlib.h>
#include <memory.h>

#include "bubble_gl_anim.h"

/*! Prints a message on standard error before aborting.
 *
 *  \param msg      Message format string. It follows the same syntax as the
 *                  printf format string.
 *
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
static void
sig (int sig) {
    error ("got signal %d\n", sig);
}


/* *******************************************************************
 * Action functions
 */

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
static void
bgl_action_free (bgl_action_t *action) {
    if (action)
        free (action);
}



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
 */
static void
BGLBlock_ref (BubbleBlock block) {
    block->ref++;
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
    BubbleDisplayItem new_item = malloc (sizeof (*new_item));

    INIT_LIST_HEAD (&new_item->disp_list);

    new_item->block = block;
    BGLBlock_ref (block);

    new_item->disp_block = NULL;

    new_item->current.x = new_item->current.y = 0;
    new_item->scale.x = new_item->scale.y = 0;

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
    BubbleDisplayItem new_item =
        newBGLDisplayItem (item->block);
    new_item->current.x = item->current.x;
    new_item->current.y = item->current.y;
    new_item->scale.x = item->scale.x;
    new_item->scale.y = item->scale.y;
    return new_item;
}


/*! Destroy a display item.
 *
 *  \param item     A valid pointer to a display item structure.
 */
static void
destroyBGLDisplayItem (BubbleDisplayItem item) {
    list_del (&item->disp_list);
    BGLBlock_unref (item->block);
    BGLBlock_unref (item->disp_block);
    free (item);
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
BGLDisplayItem_rotateTo (BubbleDisplayItem item, float degrees) {
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
BGLDisplayItem_moveTo (BubbleDisplayItem item, float x, float y) {
    item->current.x = x;
    item->current.y = y;
}


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
    BubbleMorph morph = NULL;
    BubbleShape shape1 = NULL;
    BubbleShape shape2 = NULL;

    if (!item)
        return;

    if (item->block->type != BGL_BLOCK_TYPE_MORPH)
		return ;

    morph = (BubbleMorph) item->block;
    shape1 = morph->shape1;
    shape2 = morph->shape2;

    if (ratio < 0.0f)
        ratio = 0.0f;
    else if (ratio > 1.0f)
        ratio = 1.0f;

#if 0
    /* Morph implementation */
    item->disp_block = newBGLShape();

#else
    /* Simplified morph display for tests. */
    if (ratio < 0.5f) {
        item->disp_block = (BubbleBlock) shape1;
        BGLBlock_ref ((BubbleBlock) shape1);
    } else {
        item->disp_block = (BubbleBlock) shape2;
        BGLBlock_ref ((BubbleBlock) shape2);
    }
#endif

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
        if (citem->disp_block) {
            cpy->disp_block = citem->disp_block;
            BGLBlock_ref (cpy->disp_block);
        }
        list_add_tail (&cpy->disp_list, &new_frame->display_items);
    }

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
    BubbleDisplayItem item;

    while (!list_empty (&frame->display_items)) {
        item = list_entry (frame->display_items.next, typeof (*item), disp_list);
        list_del (&item->disp_list);
        destroyBGLDisplayItem (item);
    }

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
    BubbleMovie movie = malloc (sizeof (*movie));

    INIT_LIST_HEAD (&movie->frames);

    movie->frames_count = 0;
    movie->frames_array = NULL;

    movie->playing = 0;

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
    movie->playing = yes;
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
    bgl_frame_t *cframe = NULL;
    bgl_frame_t *nframe = NULL;

    if (list_empty (&movie->frames)) {
        nframe = bgl_frame_new ();
    }
    else if (movie->playing) {
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
    bgl_frame_t *cframe;
    BubbleDisplayItem new_item;

    cframe = BGLMovie_get_current_frame (movie);
    new_item = newBGLDisplayItem (block);
    list_add (&new_item->disp_list, &cframe->display_items);

    return new_item;
}


/*! Sets the time the current frame must be displayed.
 *
 *  \param movie    A valid pointer to a movie structure.
 *  \param sec      A float tha represents the duration of the frame in
 *                  seconds.
 */
static void
BGLMovie_pause (BubbleMovie movie, float sec) {
    BGLMovie_get_current_frame (movie)->duration += sec;
}


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
    }

    movie->current_frame = 0;

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
    if (!shape)
        return NULL;

    BubbleFillStyle new_style = newBGLFillStyle (r, g, b, a);

    list_add (&new_style->style_list, &shape->styles);

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
    /* XXX mem leak. */
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
    BubbleShape new_shape = malloc (sizeof (*new_shape));
    BGLBlock_init ((BubbleBlock) new_shape, BGL_BLOCK_TYPE_SHAPE,
                   (free_block_fn_t) __destroyBGLShape);

    INIT_LIST_HEAD (&new_shape->actions);

    new_shape->orig.x = new_shape->orig.y = 0;

    INIT_LIST_HEAD (&new_shape->styles);
    new_shape->current_style = NULL;

    BGLLine_init (&new_shape->line, 1, 0, 0, 0, 255);

    new_shape->pen.x = new_shape->pen.y = 0;
  
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
    shape->current_style = style;
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
BGLShape_movePenTo (BubbleShape shape, float x, float y) {
    bgl_action_movepen_t *action;

    if (!shape) return;

    shape->pen.x = x;
    shape->pen.y = y;

    action = (bgl_action_movepen_t *)
        BGLShape_addAction (shape, BGL_ACTION_MOVE_PEN);

    if (!action) return;

    action->position.x = x;
    action->position.y = y;
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
BGLShape_movePen (BubbleShape shape, float dx, float dy) {
    BGLShape_movePenTo (shape, shape->pen.x + dx, shape->pen.y + dy);
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
BGLShape_drawLineTo (BubbleShape shape, float x, float y) {
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
BGLShape_drawLine (BubbleShape shape, float dx, float dy) {
    BGLShape_drawLineTo (shape, shape->pen.x + dx, shape->pen.y + dy);
}


/*! Draws a circle.
 *
 *  \param shape    A valid pointer to #BGLShape structure or NULL.
 *  \param r        A float that represents the radius of the circle.
 */
static void
BGLShape_drawCircle (BubbleShape shape, float r) {
    bgl_action_drawcircle_t *action;

    if (!shape) return;

    action = (bgl_action_drawcircle_t *)
        BGLShape_addAction (shape, BGL_ACTION_DRAW_CIRCLE);

    if (!action) return;

    bgl_action_set_style (&action->hdr_styles,
                          shape->current_style, &shape->line);

    action->radius = r;
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
BGLShape_setLine (BubbleShape shape, unsigned short width, unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
    BGLLine_init (&shape->line, width, r, g, b, a);
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
BGLShape_drawCurve (BubbleShape shape, float controldx, float controldy, float anchordx, float anchordy) {
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
}


/*! Prints a glyph.
 *
 *  \param shape    A valid pointer to a #BGLShape structure or NULL.
 *  \param c        a short that represents the glyph to print.
 *  \param size     an integer that represents the size of the glyph.
 */
static void
BGLShape_drawSizedGlyph (BubbleShape shape, unsigned short c, int size) {
    bgl_action_drawglyph_t *action;

    if (!shape) return;

    action = (bgl_action_drawglyph_t *)
        BGLShape_addAction (shape, BGL_ACTION_DRAW_GLYPH);

    if (!action) return;

    bgl_action_set_style (&action->hdr_styles,
                          shape->current_style, &shape->line);

    action->glyph = c;
    action->gsize = size;
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

static void
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
    BubbleMorph new_morph = malloc (sizeof (*new_morph));
    BGLBlock_init ((BubbleBlock) new_morph, BGL_BLOCK_TYPE_MORPH,
                   (free_block_fn_t) __destroyBGLMorph);
    new_morph->shape1 = newBGLShape();
    new_morph->shape2 = newBGLShape();

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
BGLsetThread (BubbleShape shape, int id, float x, float y,
              float width, float height) {

}

/*! Sets a bubble boundary box.
 *
 *  /todo Not yet implemented. Propotype must be redesigned before.
 */
static void
BGLsetBubble (BubbleShape shape, int id, float x, float y,
              float width, float height) {

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
}

/*! Maps bubbles ops to BGL interface.
 */
void
BubbleOps_setBGL() {
    static BubbleOps BGLBubbleOps = {
        /* Movie methods */
        .newMovie = newBGLMovie,
        .Movie_startPlaying = BGLMovie_startPlaying,
        .Movie_abort = BGLMovie_abort,
        .Movie_nextFrame = BGLMovie_nextFrame,
        .Movie_add = BGLMovie_add,
        .Movie_pause = BGLMovie_pause,
        .Movie_save = BGLMovie_save,
	 
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
	 
        /* Thread/Bubble-specific methods */
        .SetThread = BGLsetThread,
        .SetBubble = BGLsetBubble
    };

    curBubbleOps = &BGLBubbleOps;
}
