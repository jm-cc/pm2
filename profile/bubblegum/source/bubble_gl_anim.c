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

#include <wchar.h>
#include <string.h>
#include <math.h>
#include <gtk/gtk.h>
#include <stdlib.h>
#include <stdio.h>
#include <gtk/gtkgl.h>
#include <GL/gl.h>
#include <GL/glu.h>

#include "timer.h"
#include "polices.h"

#include "load.h"
#include "rightwindow.h"

#include "bubble_gl_anim.h"
#include "bubblelib_fxt.h"
#include "bubblelib_anim.h"

BubbleMovie movie = NULL;

/*! Draw modes. Since shape can have both filled regions or/and bordered
 *   regions, we need to have 2 passes on each shape :
 *      \li \c DRAW_MODE_FILL to draw filled regions ;
 *      \li \c DRAW_MODE_LINE to draw regions border lines.
 */
enum draw_mode {
    DRAW_MODE_FILL,
    DRAW_MODE_LINE,
    DRAW_MODE_COUNT
};

/*! Data for current state of cursor and fill style. This structure is used in
 *  functions in charge of drawing shapes.
 */
struct draw_current_data {
    struct BGLFillStyle *fill;     /*! < a #BGLFillStyle structure pointer. */

    float         off_x;           /*! < abscissa offset of the cursor. */
    float         off_y;           /*! < ordinate offset of the cursor. */

    float         sx;              /*! < abscissa scale value. */
    float         sy;              /*! < ordinate scale value. */

    float         cx;              /*! < current output cursor abscissa. offset
                                    * and scaling applied. */
    float         cy;              /*! < current output cursor ordinate. offset
                                    * and scaling applied. */
};



/*! Drawing action function type definition. */
typedef void (*do_action_fn_t)(bgl_action_t *,
                             struct draw_current_data *,
                             enum draw_mode);




#define set_scaled_position(pstate, x, y)  do { \
        (pstate)->cx = (pstate)->sx * ((pstate)->off_x * (x)); \
        (pstate)->cy = (pstate)->sy * ((pstate)->off_y * (y)); \
    } while (0);

#define reset_scaled_position(pstate)   set_scaled_position ((pstate), 0, 0)


#define set_gl_color(pcolor) \
    glColor4f ((pcolor)->r, (pcolor)->g, (pcolor)->b, (pcolor)->a)




/*! Move the pen. */
static void
bgl_anim_do_movepen (bgl_action_movepen_t *p_action,
                     struct draw_current_data *p_draw_state,
                     enum draw_mode mode);

/*! Draws a line. */
static void
bgl_anim_do_drawline (bgl_action_drawline_t *p_action,
                      struct draw_current_data *p_draw_state,
                      enum draw_mode mode);

/*! Draws a circle. */
static void
bgl_anim_do_drawcircle (bgl_action_drawcircle_t *p_action,
                        struct draw_current_data *p_draw_state,
                        enum draw_mode mode);

/*! Draws a curve. */
static void
bgl_anim_do_drawcurve (bgl_action_drawcurve_t *p_action,
                       struct draw_current_data *p_draw_state,
                       enum draw_mode mode);

/*! Draws a glyph. */
static void
bgl_anim_do_drawglyph (bgl_action_drawglyph_t *p_action,
                       struct draw_current_data *p_draw_state,
                       enum draw_mode mode);



static do_action_fn_t actions_TABLE[BGL_ACTION_COUNT] = {
    [BGL_ACTION_MOVE_PEN]    (do_action_fn_t) bgl_anim_do_movepen,
    [BGL_ACTION_DRAW_LINE]   (do_action_fn_t) bgl_anim_do_drawline,
    [BGL_ACTION_DRAW_CIRCLE] (do_action_fn_t) bgl_anim_do_drawcircle,
    [BGL_ACTION_DRAW_CURVE]  (do_action_fn_t) bgl_anim_do_drawcurve,
    [BGL_ACTION_DRAW_GLYPH]  (do_action_fn_t) bgl_anim_do_drawglyph
};



/*! Creates a new movie from a trace file.
 *
 *  \param trace    A string that represents the file containing the trace.
 *  \param out_file A string that represents the file to save the movie,
 *                  or NULL.
 *
 *  \return A new allocated #BubbleMovie structure on success, NULL otherwise.
 */
BubbleMovie
newBubbleMovieFromFxT (const char *trace, const char *out_file) {

    /*! \todo remove global variable. */
    printf("Init... ");
    curBubbleOps->init();
    printf("OK\n");

    printf("newBubbleMovie... ");
    movie = newBubbleMovie();
    printf("OK\n");

    BubbleMovie_startPlaying (movie, 0); /* -d bubbles cl arg. */

    /*! \todo Check size values. */
    /*! \todo Must really this method be called there ? */
    setRqs(&norq, 1, 0, 0, MOVIEX, 2 * CURVE);

    printf("BubbleFromFxT... ");
    BubbleFromFxT (movie, trace);
    printf("OK\n");

    printf("BubbleMovie_save... ");
    BubbleMovie_save (movie, out_file);
    printf("OK\n");

    return movie;
}

/*! Maps bubbles ops to SWF interface.
 *  \todo Move to the right place.
 */
void
BubbleOps_setSWF() {
    curBubbleOps = &SWFBubbleOps;
}



/*! Sets GL parameters according to current mode.
 *  
 *  \param mode     Drawing mode
 *  \param pfill    a valid pointer to a #BGLFillStyle structure.
 *  \param pline    a valid pointer to a #BGLLineStyle structure.
 */
static void
bgl_set_mode_gl_params (enum draw_mode mode,
                        struct BGLFillStyle *pfill,
                        struct BGLLineStyle *pline) {
    if (mode == DRAW_MODE_LINE) {
            set_gl_color (&pline->color);
            glLineWidth(pline->width);
    }
    else {
        set_gl_color (&pfill->color);
    }
}


/*! move the pen. */
static void
bgl_anim_do_movepen (bgl_action_movepen_t *p_action,
                     struct draw_current_data *p_draw_state,
                     enum draw_mode mode) {
    if (mode == DRAW_MODE_FILL && p_draw_state->fill) {
        glEnd ();
        p_draw_state->fill = NULL;
    }

    set_scaled_position (p_draw_state,
                         p_action->position.x, p_action->position.y);
}

/*! Draws a line. */
static void
bgl_anim_do_drawline (bgl_action_drawline_t *p_action,
                      struct draw_current_data *p_draw_state,
                      enum draw_mode mode) {
    struct BGLFillStyle *fill = p_draw_state->fill;
    float orig_x = p_draw_state->cx;
    float orig_y = p_draw_state->cy;

    set_scaled_position (p_draw_state,
                         p_action->draw_to.x, p_action->draw_to.y);

    struct BGLFillStyle *nfill = p_action->hdr_styles.fill;
    struct BGLLineStyle *pline = &(p_action->hdr_styles.line);
    float nx = p_draw_state->cx;
    float ny = p_draw_state->cy;

    /* Detects begining and ending of forms. */
    if (mode == DRAW_MODE_LINE) {
        glBegin (GL_LINE);            

    } else {
        if (nfill && !fill)
            glBegin (GL_POLYGON);
        else if (fill && !nfill)
            glEnd ();
        
        p_draw_state->fill = nfill;
    }
    
    if (nfill || mode == DRAW_MODE_LINE) {
        bgl_set_mode_gl_params (mode, nfill, pline);
        
        if (!fill || mode == DRAW_MODE_LINE)
            glVertex3f (orig_x, orig_y, 0.0f);
        glVertex3f (nx, ny, 0.0f);        
    }

    if (mode == DRAW_MODE_LINE)
        glEnd();
}

/*! Draws a circle. */
static void
bgl_anim_do_drawcircle (bgl_action_drawcircle_t *p_action,
                        struct draw_current_data *p_draw_state,
                        enum draw_mode mode) {
    struct BGLFillStyle *fill = p_draw_state->fill;
    float orig_x = p_draw_state->cx;
    float orig_y = p_draw_state->cy;

    struct BGLFillStyle *nfill = p_action->hdr_styles.fill;
    struct BGLLineStyle *pline = &(p_action->hdr_styles.line);
    float radius_x = p_draw_state->sx * p_action->radius;
    float radius_y = p_draw_state->sy * p_action->radius;

    if (mode == DRAW_MODE_FILL) {
        if (fill) {
            glEnd();
            p_draw_state->fill = NULL;
        }
        glBegin (GL_POLYGON);

    } else {
        glBegin (GL_LINE_LOOP);
    }

    if (nfill || mode == DRAW_MODE_LINE) {
        bgl_set_mode_gl_params (mode, nfill, pline);

        /*! \todo Draw circle. How many steps ? */
        glVertex3f (orig_x, orig_y + radius_y, 0.0f);
        glVertex3f (orig_x + radius_x, orig_y, 0.0f);
        glVertex3f (orig_x, orig_y - radius_y, 0.0f);
        glVertex3f (orig_x - radius_x, orig_y, 0.0f);
    }

    glEnd();
}

/*! Draws a curve. */
static void
bgl_anim_do_drawcurve (bgl_action_drawcurve_t *p_action,
                       struct draw_current_data *p_draw_state,
                       enum draw_mode mode) {
    struct BGLFillStyle *fill = p_draw_state->fill;
    float orig_x = p_draw_state->cx;
    float orig_y = p_draw_state->cy;

    struct BGLFillStyle *nfill = p_action->hdr_styles.fill;
    struct BGLLineStyle *pline = &(p_action->hdr_styles.line);

    set_scaled_position (p_draw_state,
                         p_action->control.x, p_action->control.y);

    float control_x = p_draw_state->cx;
    float control_y = p_draw_state->cy;

    set_scaled_position (p_draw_state,
                         p_action->anchor.x, p_action->anchor.y);

    float anchor_x = p_draw_state->cx;
    float anchor_y = p_draw_state->cy;

    /* Detects begining and ending of forms. */
    if (mode == DRAW_MODE_LINE) {
        glBegin (GL_LINE_STRIP);

    } else {
        if (nfill && !fill)
            glBegin (GL_POLYGON);
        else if (fill && !nfill)
            glEnd ();
        
        p_draw_state->fill = nfill;
    }
    
    if (nfill || mode == DRAW_MODE_LINE) {
        bgl_set_mode_gl_params (mode, nfill, pline);
        
        if (!fill || mode == DRAW_MODE_LINE)
            glVertex3f (orig_x, orig_y, 0.0f);

        /*! \todo Draw a true curve */
        glVertex3f (control_x, control_y, 0.0f);
        glVertex3f (anchor_x, anchor_y, 0.0f);
    }

    if (mode == DRAW_MODE_LINE)
        glEnd();
}

/*! Draws a glyph. */
static void
bgl_anim_do_drawglyph (bgl_action_drawglyph_t *p_action,
                       struct draw_current_data *p_draw_state,
                       enum draw_mode mode) {
    struct BGLFillStyle *fill = p_draw_state->fill;
#if 0
    float orig_x = p_draw_state->cx;
    float orig_y = p_draw_state->cy;
#endif

    struct BGLFillStyle *nfill = p_action->hdr_styles.fill;
    struct BGLLineStyle *pline = &(p_action->hdr_styles.line);

    if (fill) {
        glEnd ();
        p_draw_state->fill = NULL;
    }

    if (nfill || mode == DRAW_MODE_LINE) {
        bgl_set_mode_gl_params (mode, nfill, pline);
        /* todo. draw glyph (print a character) */
    }
}


/*! Displays an item.
 *
 *  \param display_item
 *                  A valid pointer to a #DisplayItem structure.
 */
static void
bgl_anim_DisplayItem (BubbleDisplayItem display_item) {
    BubbleShape shape = NULL;

    struct draw_current_data state = {
        .fill            = NULL,
        .off_x           = 0.0f,
        .off_y           = 0.0f,
        .sx              = 1.0f,
        .sy              = 1.0f,
        .cx              = 0.0f,
        .cy              = 0.0f,
    };

    bgl_action_t *current_action = NULL;

    /* Ignores blocks other than shapes. */
    if (!display_item->disp_block ||
        display_item->disp_block->type != BGL_BLOCK_TYPE_SHAPE)
        return;

    shape = (BubbleShape) display_item->disp_block;
    state.off_x = display_item->current.x;
    state.off_y = display_item->current.y;
    state.sx   *= display_item->scale.x;
    state.sy   *= display_item->scale.y;

    /* Shape display is done in 2 passes.
     *   - The first pass draws filled regions (when actions for which a fill
     *     style is selected) ;
     *   - The second pass draws all lines (actions that draw no line may have
     *     a line style's color's alpha set to 0.
     * This means that lines in a shape are visible above all filled regions.
     */

    reset_scaled_position (&state);

    /* Draws filled zone. */
    list_for_each_entry (current_action, &shape->actions, action_list) {
        if (current_action->type < BGL_ACTION_COUNT)
            {
                do_action_fn_t do_action = actions_TABLE[current_action->type];
                do_action (current_action, &state, DRAW_MODE_FILL);
            }
    }

    if (state.fill) { /* Close last region if needed. */
        glEnd();
        state.fill = NULL;
    }

    /* reset cursor position. */
    reset_scaled_position (&state);

    /* Draws lines. */
    list_for_each_entry (current_action, &shape->actions, action_list) {
        if (current_action->type < BGL_ACTION_COUNT)
            {
                do_action_fn_t do_action = actions_TABLE[current_action->type];
                do_action (current_action, &state, DRAW_MODE_LINE);
            }
    }

    
}


/*! Displays a frame of a BGLMovie.
 *
 *  \pre gdk_gl_drawable_gl_begin() function has been called with success.
 *
 *  \param movie    A valid pointer to a #BGLMovie structure.
 *  \param frame    An integer that represents the frame to display.
 */
void
bgl_anim_DisplayFrame (BubbleMovie movie, int iframe) {
    if (iframe < 0)
        iframe = 0;
    if (iframe >= movie->frames_count)
        iframe = movie->frames_count - 1;

    if (iframe < 0) /* the movie has no frame. */
        return;

    bgl_frame_t       *frame = movie->frames_array[iframe];
    BubbleDisplayItem display_item;

    list_for_each_entry (display_item, &frame->display_items, disp_list) {
        bgl_anim_DisplayItem (display_item);
    }
}
