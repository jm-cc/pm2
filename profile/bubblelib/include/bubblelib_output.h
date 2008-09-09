
/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2007, 2008 "the PM2 team" (see AUTHORS file)
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

#ifndef BUBBLELIB_OUTPUT_H
#define BUBBLELIB_OUTPUT_H

#include <ming.h>
#include <ming-version.h>

#ifndef BUBBLE_TYPES_DEFINED
/* Types of manipulated objects, see ming documentation */
typedef struct BubbleMovie_s *BubbleMovie;
typedef struct BubbleShape_s *BubbleShape;
typedef struct BubbleFillStyle_s *BubbleFillStyle;
typedef struct BubbleDisplayItem_s *BubbleDisplayItem;
typedef struct BubbleMorph_s *BubbleMorph;
typedef struct BubbleBlock_s *BubbleBlock;
#endif

#if (BUBBLES_MING_VERSION_MAJOR == 0) && (BUBBLES_MING_VERSION_MINOR == 3)

typedef float coordinate_t;

#else

/* Starting from 0.4.0, Ming uses `double' for coordinates and time
	 units.  */
typedef double coordinate_t;

#endif


#define RATE 16. /* frame rate */

#define BSIZE 30. /* Button size */

/* Methods on objects, see ming documentation */
typedef struct {
	/* Movie methods */
	BubbleMovie (*newMovie)(void);
#define newBubbleMovie curBubbleOps->newMovie
	/* Really play or not */
	void (*Movie_startPlaying)(BubbleMovie movie, int yes);
#define BubbleMovie_startPlaying curBubbleOps->Movie_startPlaying
	void (*Movie_nextFrame)(BubbleMovie movie);
#define BubbleMovie_nextFrame curBubbleOps->Movie_nextFrame
	BubbleDisplayItem (*Movie_add)(BubbleMovie movie, BubbleBlock block);
#define BubbleMovie_add curBubbleOps->Movie_add
	/* pause for a few seconds, or until mouse click (when sec == 0) */
	void (*Movie_pause)(BubbleMovie movie, coordinate_t sec);
#define BubbleMovie_pause curBubbleOps->Movie_pause
	void (*Movie_abort)(BubbleMovie movie);
#define BubbleMovie_abort curBubbleOps->Movie_abort
	int (*Movie_save)(BubbleMovie movie, const char *filename);
#define BubbleMovie_save curBubbleOps->Movie_save
	void (*Movie_status)(BubbleMovie movie, const char *str);
#define BubbleMovie_status curBubbleOps->Movie_status
	/* TODO: we probably need a destroyMovie method too... */

	/* Shape methods */
	BubbleShape (*newShape)(void);
#define newBubbleShape curBubbleOps->newShape
	void (*Shape_setRightFillStyle)(BubbleShape shape, BubbleFillStyle fill);
#define BubbleShape_setRightFillStyle curBubbleOps->Shape_setRightFillStyle
	void (*Shape_movePenTo)(BubbleShape shape, coordinate_t x, coordinate_t y);
#define BubbleShape_movePenTo curBubbleOps->Shape_movePenTo
	void (*Shape_movePen)(BubbleShape shape, coordinate_t x, coordinate_t y);
#define BubbleShape_movePen curBubbleOps->Shape_movePen
	void (*Shape_drawLineTo)(BubbleShape shape, coordinate_t x, coordinate_t y);
#define BubbleShape_drawLineTo curBubbleOps->Shape_drawLineTo
	void (*Shape_drawLine)(BubbleShape shape, coordinate_t dx, coordinate_t dy);
#define BubbleShape_drawLine curBubbleOps->Shape_drawLine
	void (*Shape_drawCircle)(BubbleShape shape, coordinate_t r);
#define BubbleShape_drawCircle curBubbleOps->Shape_drawCircle
	void (*Shape_setLine)(BubbleShape shape, unsigned short width, unsigned char r, unsigned char g, unsigned char b, unsigned char a);
#define BubbleShape_setLine curBubbleOps->Shape_setLine
	void (*Shape_drawCurve)(BubbleShape shape, coordinate_t controldx, coordinate_t controldy, coordinate_t anchordx, coordinate_t anchordy);
#define BubbleShape_drawCurve curBubbleOps->Shape_drawCurve
	void (*Shape_drawSizedGlyph)(BubbleShape shape, unsigned short c, int size);
#define BubbleShape_drawSizedGlyph curBubbleOps->Shape_drawSizedGlyph

	/* FillStyle methods */
	BubbleFillStyle (*Shape_addSolidFillStyle)(BubbleShape shape, unsigned char r, unsigned char g, unsigned char b, unsigned char a);
#define BubbleShape_addSolidFillStyle curBubbleOps->Shape_addSolitFillStyle

	/* DisplayItem methods */
	void (*DisplayItem_rotateTo)(BubbleDisplayItem item, coordinate_t degrees);
#define BubbleDisplayItem_rotateTo curBubbleOps->DisplayItem_rotateTo
	void (*DisplayItem_remove)(BubbleDisplayItem item);
#define BubbleDisplayItem_remove curBubbleOps->DisplayItem_remove
	void (*DisplayItem_moveTo)(BubbleDisplayItem item, coordinate_t x, coordinate_t y);
#define BubbleDisplayItem_moveTo curBubbleOps->DisplayItem_moveTo
	void (*DisplayItem_setRatio)(BubbleDisplayItem item, float ratio);
#define BubbleDisplayItem_setRatio curBubbleOps->DisplayItem_setRatio

	/* Morph methods */
	BubbleMorph (*newMorphShape)(void);
#define newBubbleMorphShape curBubbleOps->newMorphShape
	BubbleShape (*Morph_getShape1)(BubbleMorph morph);
#define BubbleMorph_getShape1 curBubbleOps->Morph_getShape1
	BubbleShape (*Morph_getShape2)(BubbleMorph morph);
#define BubbleMorph_getShape2 curBubbleOps->Morph_getShape2

	/* misc methods */
	void (*init)(void);
	void (*fini)(void);

	/* Thread/Bubble-specific methods */
	void (*SetThread)(BubbleShape shape, int id, coordinate_t x, coordinate_t y, coordinate_t width, coordinate_t height);
#define BubbleSetThread curBubbleOps->SetThread
	void (*SetBubble)(BubbleShape shape, int id, coordinate_t x, coordinate_t y, coordinate_t width, coordinate_t height);
#define BubbleSetBubble curBubbleOps->SetBubble
} BubbleOps;

extern BubbleOps *curBubbleOps, SWFBubbleOps;

extern char *SWF_fontfile;

#endif /* BUBBLELIB_OUTPUT_H */
