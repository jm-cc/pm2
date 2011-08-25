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

#ifndef BUBBLE_GL_ANIM_H
#define BUBBLE_GL_ANIM_H

#include "tbx_fast_list.h"
#include <stdio.h>

#ifdef BUBBLE_GL_DEBUG
#endif

#if 0
#  define IN()  printf ("BGL:  --> %s\n", __func__)
#  define OUT() printf ("BGL: <--  %s\n", __func__)
#  define MYTRACE(s, ...) printf (s "\n", ##__VA_ARGS__)
#else
#  define IN()  (void)(0)
#  define OUT() (void)(0)
#  define MYTRACE(s, ...) (void)(0)
#endif

/*! Color structure. */
struct BGLColor {
    unsigned char r; /*! < Red component. */
    unsigned char g; /*! < Green component. */
    unsigned char b; /*! < Blue component. */
    unsigned char a; /*! < Alpha component. */
};

/*! Fill style structure. */
struct BGLFillStyle {
    struct tbx_fast_list_head style_list; /*! < position in styles list. */
    struct BGLColor color;       /*! < Color of the style. */
};

/*! Line style structure. */
struct BGLLineStyle {
    struct BGLColor color; /*! < Style color. */
    unsigned short width;  /*! < Line width. */
};

/*! Vector structure. */
typedef struct bgl_vector_s {
    float x;  /*! < Acscissa. */
    float y;  /*! < Ordinate. */
} bgl_vector_t;



/*! Action types. */
typedef enum {
    BGL_ACTION_MOVE_PEN = 0, /*! < Move pen action. */
    BGL_ACTION_DRAW_LINE,    /*! < Draw line action. */
    BGL_ACTION_DRAW_CIRCLE,  /*! < Draw circle action. */
    BGL_ACTION_DRAW_CURVE,   /*! < Draw curve action. */
    BGL_ACTION_DRAW_GLYPH,   /*! < Draw glyph action. */
    BGL_ACTION_COUNT         /*! < Action types count. */
} bgl_action_type_e_t;

/*! Base action structure. */
typedef struct bgl_action_s {
    bgl_action_type_e_t type;     /*! < action type. */
    struct tbx_fast_list_head action_list; /*! < Position in actions list. */
    size_t size;                  /*! < Size of the action structure. */
} bgl_action_t;

/* Action structure with style informations */
typedef struct bgl_action_with_style_s {
    struct bgl_action_s hdr;   /*! < Base action structure. */
    struct BGLFillStyle *fill; /*! < pointer to a fill style. */
    struct BGLLineStyle line;  /*! < Line style structure. */
} bgl_action_with_style_t;

/*! Move Pen action structure. */
typedef struct bgl_action_movepen_s {
    struct bgl_action_s hdr;      /*! < Base action structure. */
    struct bgl_vector_s position; /*! < Position of the pen. */
} bgl_action_movepen_t;

/*! Draw line action structure. */
typedef struct bgl_action_drawline_s {
    struct bgl_action_with_style_s hdr_styles; /*! < Base action structure. */
    struct bgl_vector_s draw_to;               /*! < Line end point. */
} bgl_action_drawline_t;

/*! Draw circle action structure. */
typedef struct bgl_action_drawcircle_s {
    struct bgl_action_with_style_s hdr_styles; /*! < Base action structure. */
    float radius;                              /*! < Radius of the circle. */
} bgl_action_drawcircle_t;

/*! Draw curve action structure. */
typedef struct bgl_action_drawcurve_s {
    struct bgl_action_with_style_s hdr_styles; /*! < Base action structure. */
    struct bgl_vector_s control;               /*! < Control point. */
    struct bgl_vector_s anchor;                /*! < Anchor point. */
} bgl_action_drawcurve_t;

/*! Draw glyph action structure. */
typedef struct bgl_action_drawglyph_s {
    struct bgl_action_with_style_s hdr_styles; /*! < Base action structure. */
    unsigned short glyph;                      /*! < Glyph. */
    int gsize;                                 /*! < Glyph size. */
} bgl_action_drawglyph_t;



/*! Block types enumeration.  */
typedef enum {
    BGL_BLOCK_TYPE_SHAPE, /*! < Shape type. */
    BGL_BLOCK_TYPE_MORPH, /*! < Morph type. */
    BGL_BLOCK_TYPE_COUNT  /*! < Types count. */
} bgl_block_type_e_t;

struct BGLBlock;

/*! Block free function pointer type. */
typedef void (*free_block_fn_t)(struct BGLBlock *);

/*! Block base structure. */
struct BGLBlock {
    bgl_block_type_e_t type;    /*! < Block type. */
    int                ref;     /*! < References counter. */
    free_block_fn_t    free_fn; /*! < Free function pointer. */
};


/*! Shape structure. */
struct BGLShape {
    struct BGLBlock blk;      /*! < Block base structure. */
    struct tbx_fast_list_head actions; /*! < Head of actions list. */

    struct bgl_vector_s orig; /*! < Offset of actions coordinates. */

    struct tbx_fast_list_head styles;  /*! < Head of created fill styles. */

    /* drawing data */
    struct BGLFillStyle *current_style; /*! < Current fill style. */
    struct BGLLineStyle line;           /*! < Current line style. */
    struct bgl_vector_s pen;            /*! < Current pen position. */
};


/*! Morph structure. */
struct BGLMorph {
    struct BGLBlock blk;     /*! < Block base structure. */
    struct BGLShape *shape1; /*! < Pointer to the begining shape. */
    struct BGLShape *shape2; /*! < Pointer to the ending shape. */
};



/*! Display Item structure. */
struct BGLDisplayItem {
    struct tbx_fast_list_head     disp_list;  /*! < Position in display item list. */
    struct BGLBlock     *block;      /*! < Source block. */
    struct BGLBlock     *disp_block; /*! < Block to display. */
    struct bgl_vector_s  current;    /*! < Position for current frame. */
    struct bgl_vector_s  scale;      /*! < Scale values. */
};



/*! Frame structure. */
typedef struct bgl_frame_s {
    struct tbx_fast_list_head  frame_list;    /*! < Position of the frame. */
    float             duration;      /*! < Frame duration, in seconds. */
    struct tbx_fast_list_head  display_items; /*! < Head of display items list. */
    const char *      status;        /*! < Status text. */
} bgl_frame_t;



/*! Movie structure.
 *  \todo complete structure.
 */
struct BGLMovie {
    struct tbx_fast_list_head frames;    /*! < Head of frames list. */

    /* Set in BGLMovie_save. */
    int           frames_count; /*! < Frames count. */
    bgl_frame_t **frames_array; /*! < Frames array. Used to speed up play control
                                  features. */

    float height;               /*! < Boundary height. */
    float width;                /*! < Boundary width */

    int playing;                /*! < Movie is playing. */
    int current_frame;          /*! < Frame beeing displayed. */
};



#ifndef BUBBLE_TYPES_DEFINED
typedef struct BGLFillStyle   *BubbleFillStyle;
typedef struct BGLBlock       *BubbleBlock;
typedef struct BGLShape       *BubbleShape;
typedef struct BGLMorph       *BubbleMorph;
typedef struct BGLDisplayItem *BubbleDisplayItem;
typedef struct BGLMovie       *BubbleMovie;
#  define BUBBLE_TYPES_DEFINED
#endif /* BUBBLE_TYPES_DEFINED */

#include "bubblelib_output.h"


/*! Maps bubbles ops to BGL interface. */
void
BubbleOps_setBGL();

/*! Maps bubbles ops to SWF interface. */
void
BubbleOps_setSWF();

/*! Creates a new movie from a trace file. */
BubbleMovie
newBubbleMovieFromFxT (const char *trace, const char *out_file);

/*! Destroys a movie structure. */
void
destroyBGLMovie (BubbleMovie movie);

#ifndef destroyBubbleMovie
# define destroyBubbleMovie destroyBGLMovie
#endif

/*! Displays a frame of a BGLMovie. */
void
bgl_anim_DisplayFrame (BubbleMovie movie, int iframe, float scale,
                      float goff_x, float goff_y, float rev_x, float rev_y);


#endif /* BUBBLE_GL_ANIM_H */
