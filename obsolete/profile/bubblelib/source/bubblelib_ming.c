
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

#include <ming.h>
#include <stdio.h>
#include <signal.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

typedef struct {
	SWFMovie swf;
	SWFDisplayItem pause_item;
	int playing;
	SWFDisplayItem status_item;
} *BubbleMovie;

typedef SWFShape BubbleShape;
typedef SWFFillStyle BubbleFillStyle;
typedef SWFDisplayItem BubbleDisplayItem;
typedef SWFMorph BubbleMorph;
typedef SWFBlock BubbleBlock;
#define BUBBLE_TYPES_DEFINED

#include "bubblelib_output.h"
#include "bubblelib_anim.h"

#if (BUBBLES_MING_VERSION_MAJOR == 0) && (BUBBLES_MING_VERSION_MINOR <= 3)
#define mySWFButton_addCharacter(button, shape, flags) \
	SWFButton_addShape((button), (void*)(shape), (flags))
#define mynewSWFAction(script) \
	compileSWFActionCode(script)
#define mynewSWFFont_fromFile(filename) ({ \
		SWFFont *__font = NULL; \
		FILE *__f = fopen((filename), "r"); \
		if (__f) { \
			__font = (SWFFont) loadSWFFontFromFile(__f); \
			if (!__font) \
				perror("can't load font"); \
		} \
		__font; \
	})
#else
#define mySWFButton_addCharacter(button, shape, flags) \
	SWFButton_addCharacter((button), (void*)(shape), (flags))
#define mynewSWFAction(script) \
	newSWFAction(script)
#define mynewSWFFont_fromFile(filename) \
	newSWFFont_fromFile(filename)
#endif

/* movie size */
int MOVIEX = 1024;
int MOVIEY = 768;

char *SWF_fontfile;
SWFFont font;

SWFAction stop;

static BubbleMovie lastmovie;

static void error(const char *msg, ...) {
	va_list args;
	static int recur = 0;
	
	va_start(args, msg);
	vfprintf(stderr,msg, args);
	va_end(args);
	if (!recur++) {
		fprintf(stderr,"saving to rescue.swf\n");
		unlink("rescue.swf");
		SWFMovie_save(lastmovie->swf,"rescue.swf");
		fprintf(stderr,"saved to rescue.swf\n");
	}
	abort();
}
static void sig(int sig) {
	error("got signal %d\n", sig);
}

/* Draw an arrow */
static SWFShape newArrow(coordinate_t width, coordinate_t height, int right) {
	SWFShape shape = newSWFShape();
	SWFFillStyle style = SWFShape_addSolidFillStyle(shape,0,0,0,255);
	SWFShape_setRightFillStyle(shape,style);
	if (right) {
		SWFShape_movePenTo(shape,0,0);
		SWFShape_drawLineTo(shape,width,height/2);
		SWFShape_drawLineTo(shape,0,height);
		SWFShape_drawLineTo(shape,0,0);
	} else {
		SWFShape_movePenTo(shape,width,0);
		SWFShape_drawLineTo(shape,width,height);
		SWFShape_drawLineTo(shape,0,height/2);
		SWFShape_drawLineTo(shape,width,0);
	}
	return shape;
}

static BubbleMovie mynewSWFMovie(void) {
	BubbleMovie movie = malloc(sizeof(*movie));
	SWFShape pause_shape;

	movie->swf = newSWFMovie();
	movie->playing = 1;
	movie->status_item = NULL;

	//SWFMovie_setBackground(movie->swf,0,0,0);
	SWFMovie_setRate(movie->swf,RATE);
	SWFMovie_setDimension(movie->swf,MOVIEX,MOVIEY);

	pause_shape = newSWFShape();
	SWFShape_setLine(pause_shape,thick,0,0,0,255);
	SWFShape_movePenTo(pause_shape, -CURVE/2, 0);
	SWFShape_drawLineTo(pause_shape, CURVE/2, 0);
	movie->pause_item = SWFMovie_add(movie->swf,(SWFBlock)pause_shape);
	SWFDisplayItem_moveTo(movie->pause_item, CURVE/2, MOVIEY-CURVE/2);

	{ /* stop / continue action */
		SWFButton button = newSWFButton();
		SWFDisplayItem item;
		SWFShape shape = newSWFShape();

		SWFShape_setRightFillStyle(shape,SWFShape_addSolidFillStyle(shape,0,0,0,255));
		SWFShape_movePenTo(shape,0,0);
		SWFShape_drawLineTo(shape,0,MOVIEY);
		SWFShape_drawLineTo(shape,MOVIEX,MOVIEY);
		SWFShape_drawLineTo(shape,MOVIEX,0);
		SWFShape_drawLineTo(shape,0,0);
		mySWFButton_addCharacter(button, shape, SWFBUTTON_HIT);
		SWFButton_addAction(button, stop, SWFBUTTON_MOUSEUP);
		item = SWFMovie_add(movie->swf,(SWFBlock)button);
		SWFDisplayItem_moveTo(item,0,0);

		button = newSWFButton();
		mySWFButton_addCharacter(button, newArrow(BSIZE,BSIZE,1), SWFBUTTON_UP|SWFBUTTON_DOWN|SWFBUTTON_OVER|SWFBUTTON_HIT);
		SWFButton_addAction(button, 
	mynewSWFAction(" nextFrame(); if (!stopped) play();")
				, SWFBUTTON_MOUSEDOWN|SWFBUTTON_OVERDOWNTOIDLE);
		item = SWFMovie_add(movie->swf,(SWFBlock)button);
		SWFDisplayItem_moveTo(item,MOVIEX-BSIZE,MOVIEY-BSIZE);

		button = newSWFButton();
		mySWFButton_addCharacter(button, newArrow(BSIZE,BSIZE,1), SWFBUTTON_UP|SWFBUTTON_DOWN|SWFBUTTON_OVER|SWFBUTTON_HIT);
		SWFButton_addAction(button, 
	mynewSWFAction(" for (i=0;i<16;i++) nextFrame(); if (!stopped) play(); ")
				, SWFBUTTON_MOUSEUP);
		item = SWFMovie_add(movie->swf,(SWFBlock)button);
		SWFDisplayItem_moveTo(item,MOVIEX-2*BSIZE,MOVIEY-BSIZE);

		button = newSWFButton();
		mySWFButton_addCharacter(button, newArrow(BSIZE,BSIZE,0), SWFBUTTON_UP|SWFBUTTON_DOWN|SWFBUTTON_OVER|SWFBUTTON_HIT);
		SWFButton_addAction(button, 
	mynewSWFAction(" for (i=0;i<16;i++) prevFrame();  if (!stopped) play();")
				, SWFBUTTON_MOUSEUP);
		item = SWFMovie_add(movie->swf,(SWFBlock)button);
		SWFDisplayItem_moveTo(item,MOVIEX-3*BSIZE,MOVIEY-BSIZE);

		button = newSWFButton();
		mySWFButton_addCharacter(button, newArrow(BSIZE,BSIZE,0), SWFBUTTON_UP|SWFBUTTON_DOWN|SWFBUTTON_OVER|SWFBUTTON_HIT);
		SWFButton_addAction(button, 
	mynewSWFAction(" prevFrame();  if (!stopped) play();")
				, SWFBUTTON_MOUSEDOWN|SWFBUTTON_OVERDOWNTOIDLE);
		item = SWFMovie_add(movie->swf,(SWFBlock)button);
		SWFDisplayItem_moveTo(item,MOVIEX-4*BSIZE,MOVIEY-BSIZE);
	}

	lastmovie = movie;
	return movie;
}

static void mySWFMovie_startPlaying(BubbleMovie movie, int yes) {
	movie->playing = yes;
}

static void mySWFMovie_abort(BubbleMovie movie) {
	error("aborting");
}

static void mySWFMovie_nextFrame(BubbleMovie movie) {
	static int i = 0;
	if (movie->playing) {
		SWFDisplayItem_rotateTo(movie->pause_item, (++i)*360/RATE);
		SWFMovie_nextFrame(movie->swf);
	}
}

static BubbleDisplayItem mySWFMovie_add(BubbleMovie movie, BubbleBlock block) {
	return SWFMovie_add(movie->swf, block);
}

static void mySWFMovie_pause(BubbleMovie movie, coordinate_t sec) {
	coordinate_t i;
	if (!sec) {
		/* grmbl marche pas */
		SWFDisplayItem item;
		SWFShape shape = newSWFShape();
		SWFButton button = newSWFButton();

		SWFShape_setLine(shape,4,0,0,0,255);
		SWFShape_setRightFillStyle(shape, SWFShape_addSolidFillStyle(shape,0,0,0,255));
		SWFShape_movePenTo(shape,0,0);
		SWFShape_drawLineTo(shape,BSIZE/3,0);
		SWFShape_drawLineTo(shape,BSIZE/3,BSIZE);
		SWFShape_drawLineTo(shape,0,BSIZE);
		SWFShape_drawLineTo(shape,0,0);
		SWFShape_movePenTo(shape,(BSIZE*2)/3,0);
		SWFShape_drawLineTo(shape,BSIZE,0);
		SWFShape_drawLineTo(shape,BSIZE,BSIZE);
		SWFShape_drawLineTo(shape,(BSIZE*2)/3,BSIZE);
		SWFShape_drawLineTo(shape,(BSIZE*2)/3,0);
		mySWFButton_addCharacter(button, shape, SWFBUTTON_UP|SWFBUTTON_DOWN|SWFBUTTON_OVER|SWFBUTTON_HIT);
		SWFButton_addAction(button, mynewSWFAction("stopped=1; stop();"), 0xff);
		item = SWFMovie_add(movie->swf, (SWFBlock)button);
		SWFDisplayItem_moveTo(item, CURVE, MOVIEY-BSIZE);

		mySWFMovie_nextFrame(movie);
		SWFDisplayItem_remove(item);
	} else {
		int n = sec*RATE;
		if (!n) n = 1;
		for (i=0; i<n;i++) {
			mySWFMovie_nextFrame(movie);
		}
	}
}

static int mySWFMovie_save(BubbleMovie movie, const char *filename) {
	unlink(filename);
	return SWFMovie_save(movie->swf, filename);
}

static void mySWFShape_drawSizedGlyph(BubbleShape shape, unsigned short c, int size) {
	if (!font)
		return;
	SWFShape_drawSizedGlyph(shape, font, c, size);
}

static void mySWFMovie_status(BubbleMovie movie, const char *str) {
	SWFShape shape;
	int i;
	if (!font)
		return;
	if (movie->status_item)
		SWFDisplayItem_remove(movie->status_item);
	shape = newSWFShape();
	SWFShape_setLine(shape,2,0,0,0,255);
	for (i=0; i<strlen(str); i++) {
		SWFShape_movePenTo(shape, i*13., 0);
		mySWFShape_drawSizedGlyph(shape, str[i], 20.);
	}
	movie->status_item = SWFMovie_add(movie->swf, (SWFBlock)shape);
	SWFDisplayItem_moveTo(movie->status_item, CURVE+BSIZE, MOVIEY-20.);
}

static void init(void) {
	char *fontpath;

	Ming_init();
	Ming_setErrorFunction(error);
	signal(SIGSEGV, sig);
	signal(SIGINT, sig);

	fontpath = SWF_fontfile;
	if (!SWF_fontfile) {
		char *root = getenv("PM2_SRCROOT");
		static const char path[] = "/profile/bubblelib/font/Sans.fdb";
		int len = strlen(root) + strlen(path);
		fontpath = malloc(len+1);
		strncpy(fontpath, root, strlen(root));
		strncpy(fontpath + strlen(root), path, strlen(path)+1);
	}

	font = mynewSWFFont_fromFile(fontpath);
	if (!font) {
		perror("Warning: could not open font file, will not able to print thread name, priority and scheduler status");
		fprintf(stderr,"(tried %s. Use -f option for changing this. fdb files can be generated from fft files by using makefdb, and fft files can be generated from ttf files by using ttftofft)\n",fontpath);
	}

	if (!SWF_fontfile)
		free(fontpath);

	/* pause macro */
	stop = mynewSWFAction(" if (!stopped) { stop(); stopped=1; } else { play(); stopped=0; }");
}

static void fini(void) {
	signal(SIGSEGV, SIG_DFL);
	signal(SIGINT, SIG_DFL);
}

BubbleOps SWFBubbleOps = {
	/* Movie methods */
	.newMovie = mynewSWFMovie,
	.Movie_startPlaying = mySWFMovie_startPlaying,
	.Movie_abort = mySWFMovie_abort,
	.Movie_nextFrame = mySWFMovie_nextFrame,
	.Movie_add = mySWFMovie_add,
	.Movie_pause = mySWFMovie_pause,
	.Movie_save = mySWFMovie_save,
	.Movie_status = mySWFMovie_status,

	/* Shape methods */
	.newShape = newSWFShape,
	.Shape_setRightFillStyle = SWFShape_setRightFillStyle,
	.Shape_movePenTo = SWFShape_movePenTo,
	.Shape_movePen = SWFShape_movePen,
	.Shape_drawLineTo = SWFShape_drawLineTo,
	.Shape_drawLine = SWFShape_drawLine,
	.Shape_drawCircle = SWFShape_drawCircle,
	.Shape_setLine = SWFShape_setLine,
	.Shape_drawCurve = SWFShape_drawCurve,
	.Shape_drawSizedGlyph = mySWFShape_drawSizedGlyph,

	/* FillStyle methods */
	.Shape_addSolidFillStyle = SWFShape_addSolidFillStyle,

	/* DisplayItem methods */
	.DisplayItem_rotateTo = SWFDisplayItem_rotateTo,
	.DisplayItem_remove = SWFDisplayItem_remove,
	.DisplayItem_moveTo = SWFDisplayItem_moveTo,
	.DisplayItem_setRatio = SWFDisplayItem_setRatio,

	/* Morph methods */
	.newMorphShape = newSWFMorphShape,
	.Morph_getShape1 = SWFMorph_getShape1,
	.Morph_getShape2 = SWFMorph_getShape2,
	
	/* Misc methods */
	.init = init,
	.fini = fini,
};
