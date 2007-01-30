
/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2004-2005 "the PM2 team" (see AUTHORS file)
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

#define _GNU_SOURCE
#include <stdint.h>
#include <inttypes.h>

/*******************************************************************************
 * Configuration
 */

/* Use FXT trace */
#define FXT

/* else build movie by hand */

//#define SHOWBUILD
#ifndef SHOWBUILD
#define SHOWEVOLUTION
#endif
//#define SHOWPRIO

/* choose between tree and bubble representation */
#undef BUBBLES

#if 1
#define TREES
#else
#define BUBBLES
#endif

/* bubbles / threads aspect */

#define RQ_XMARGIN 20.
#define RQ_YMARGIN 10.

#define RATE 16. /* frame rate */

#define BSIZE 30 /* Button size */

static int DISPPRIO = 0;
static int DISPNAME = 0;
static int showSystem = 0;
static int showEmptyBubbles = 1;

static float thick = 4.;
static float CURVE = 20.;
static float OPTIME = 1; /* Operation time */
#define DELAYTIME (OPTIME/6.) /* FXT delay between switches */
#define BUBBLETHICK thick
#define THREADTHICK thick
#define RQTHICK thick

/* movie size */

static int MOVIEX = 1024;
static int MOVIEY = 600;

static int playing = 1, verbose = 0;
static char *fontfile = "/usr/share/libming/fonts/Timmons.fdb";
#define verbprintf(fmt,...) do { if (verbose) printf(fmt, ## __VA_ARGS__); } while(0);


/*******************************************************************************
 * End of configuration
 */

/*******************************************************************************
 * The principle of animation is to show some entities (showEntity()), and then
 * perform animations, which consist of
 * - calling fooBegin() functions,
 * - modifiying entities parameters, if needed,
 * - calling fooBegin2() functions (for computing the difference between initial
 *   parameters and current parameters),
 * - for step between 0.0 and 1.0, calling fooStep() functions and calling nextFrame(movie); doStepsBegin() and doStepsEnd() are helper macros for this,
 * - calling with fooEnd() functions.
 *
 * See the second part of main() for examples
 */

#define doStepsBegin(j) do {\
	float i,j; \
	for (i=0.;i<=OPTIME*RATE;i++) { \
		j = -cos((i*M_PI)/(OPTIME*RATE))/2.+0.5;
#define doStepsEnd() \
		nextFrame(movie); \
		} \
	} while(0);


#ifdef TREES
#define OVERLAP CURVE
#endif
#ifdef BUBBLES
#define OVERLAP 0
#endif
#include <ming.h>
#include <math.h>
#include <search.h>
#include <getopt.h>
#include <libgen.h>

#ifdef FXT
#define FUT

#include <fxt/fxt.h>

#include <fxt/fut.h>
//#include <string.h>
char *strdup(const char*s);
struct fxt_code_name fut_code_table [] =
{
#include "fut_print.h"
	{0, NULL }
};
#endif

#include <stdlib.h>
#include "pm2_list.h"
#ifndef min
#define min(a,b) ({ typeof(a) _a = (a); typeof(b) _b = (b); _a<_b?_a:_b; })
#endif
#ifndef max
#define max(a,b) ({ typeof(a) _a = (a); typeof(b) _b = (b); _a>_b?_a:_b; })
#endif

#ifdef __ia64
#define _SYS_UCONTEXT_H 1
#define _BITS_SIGCONTEXT_H 1
#endif
#include <signal.h>

/* several useful macros */

#include "tbx_compiler.h"
#include "tbx_macros.h"

#define list_for_each_entry_after(pos,head,member,start)		   \
	for (pos = list_entry((start)->member.next, typeof(*pos), member), \
			tbx_prefetch(pos->member.next);			   \
		&pos->member != (head);					   \
		pos = list_entry(pos->member.next, typeof(*pos), member),  \
			tbx_prefetch(pos->member.next))

/*******************************************************************************
 * SWF Stuff
 */
SWFMovie movie;
SWFAction stop;
SWFFont font;

SWFShape pause_shape;
SWFDisplayItem pause_item;
void nextFrame(SWFMovie movie) {
	static int i = 0;
	if (playing) {
		SWFDisplayItem_rotateTo(pause_item, (++i)*360/RATE);
		SWFMovie_nextFrame(movie);
	}
}

/* bubble_pause for a few seconds, or until mouse click (when sec == 0) */
void bubble_pause(float sec) {
	float i;
	if (!sec) {
		/* grmbl marche pas */
		SWFDisplayItem item;
		SWFShape shape = newSWFShape();
		SWFButton button = newSWFButton();

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
		SWFButton_addShape(button, (void*)shape, SWFBUTTON_UP|SWFBUTTON_DOWN|SWFBUTTON_OVER|SWFBUTTON_HIT);
		SWFButton_addAction(button, compileSWFActionCode("stopped=1; stop();"), 0xff);
		item = SWFMovie_add(movie, (SWFBlock)button);

		nextFrame(movie);
		SWFDisplayItem_remove(item);
	} else {
		int n = sec*RATE;
		if (!n) n = 1;
		for (i=0; i<n;i++) {
			nextFrame(movie);
		}
	}
}

/* Draw an arrow */
SWFShape newArrow(float width, float height, int right) {
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

void gasp() {
	/* essaie quand m�e de sauver ce qu'on peut */
	fprintf(stderr,"saving to rescue.swf\n");
	SWFMovie_save(movie,"rescue.swf");
	fprintf(stderr,"written on rescue.swf\n");
	abort();
}

/*******************************************************************************
 * Pointers Stuff
 *
 * Association between application pointers and our own pointers
 */
void newPtr(uint64_t ptr, void *data) {
	char *buf=malloc(32);
	ENTRY *item = malloc(sizeof(*item)), *found;
	item->key = buf;
	item->data = data;
	snprintf(buf,32,"%"PRIx64,ptr);
	found=hsearch(*item, ENTER);
	if (!found) {
		perror("hsearch");
		fprintf(stderr,"table pleine !\n");
		gasp();
	}
	found->data=data;
	//fprintf(stderr,"%p -> %p\n",ptr,data);
}

void *getPtr (uint64_t ptr) {
	char buf[32];
	ENTRY item = {.key = buf, .data=NULL}, *found;
	snprintf(buf,sizeof(buf),"%"PRIx64,ptr);
	found = hsearch(item, FIND);
	//fprintf(stderr,"%p -> %p\n",ptr,found?found->data:NULL);
	return found ? found->data : NULL;
}
void delPtr(uint64_t ptr) {
	char buf[32];
	ENTRY item = {.key = buf, .data=NULL}, *found;
	snprintf(buf,sizeof(buf),"%"PRIx64,ptr);
	found = hsearch(item, ENTER);
	if (!found) gasp();
	found->data = NULL;
}

/*******************************************************************************
 * Entity
 */
enum entity_type {
	BUBBLE,
	THREAD,
	RUNQUEUE,
};

typedef struct entity_s {
	float x, y;		/* position */
	float lastx, lasty;	/* position just before move */
	float width, height;	/* size */
	float lastwidth, lastheight;	/* size just before move */
	unsigned thick;		/* drawing thickness */
	int prio;		/* scheduling priority */
	struct list_head rq;	/* position in rq */
	struct list_head entity_list;	/* position in bubble */
	enum entity_type type;		/* type of entity */
	SWFDisplayItem lastitem;	/* last item shown in movie */
	SWFShape lastshape;		/* last shape used in movie */
	struct entity_s *holder, *lastholder;	/* current holder, holder just before move */
	struct bubble_s *bubble_holder;	/* holding bubble */
	int leaving_holder;	/* are we leaving our holder (special case for animations) */
	int nospace;		/* in the case of explosion, space used by an entity might actually be already reserved by the holding bubble */
} entity_t;

entity_t *getEntity (uint64_t ptr) {
	return (entity_t *) getPtr(ptr);
}

void setEntityRecur(SWFShape shape, entity_t *e);
void growInHolderBegin(entity_t *e, float dx, float dy);
void growInHolderBegin2(entity_t *e);
void growInHolderStep(entity_t *e, float step);
void growInHolderEnd(entity_t *e);
void removeFromHolderBegin(entity_t *e);
void removeFromHolderBegin2(entity_t *e);
void removeFromHolderStep(entity_t *e, float step);
void removeFromHolderEnd(entity_t *e);

/*******************************************************************************
 * Runqueue
 */

typedef struct {
	entity_t entity;
	struct list_head entities;	/* held entities */
	float nextX;			/* position of next enqueued entity */
} rq_t;

rq_t *norq;

void switchRunqueuesBegin(rq_t *rq2, entity_t *e);
void switchRunqueuesBegin2(rq_t *rq2, entity_t *e);
void switchRunqueuesStep(rq_t *rq2, entity_t *e, float step);
void switchRunqueuesEnd(rq_t *rq2, entity_t *e);

static inline rq_t * rq_of_entity(entity_t *e) {
	return tbx_container_of(e,rq_t,entity);
}

rq_t *getRunqueue (uint64_t ptr) {
	rq_t *rq = (rq_t *) getPtr(ptr);
	return rq;
}

/* set nb runqueues at (x,y) using (widght,height) size, and put allocated data in rqs */
void setRqs(rq_t **rqs, int nb, float x, float y, float width, float height) {
	int i;
	float rqWidth = (width-(nb-1)*RQ_XMARGIN)/nb;

	*rqs = malloc(nb*sizeof(**rqs));

	for (i=0;i<nb;i++) {
		(*rqs)[i].entity.x = x+i*(rqWidth+RQ_XMARGIN);
		(*rqs)[i].entity.y = y;
		(*rqs)[i].entity.width = rqWidth;
		(*rqs)[i].entity.height = height;
		(*rqs)[i].entity.thick = RQTHICK;
		(*rqs)[i].entity.type = RUNQUEUE;
		(*rqs)[i].entity.lastitem = NULL;
		(*rqs)[i].entity.holder = NULL;
		INIT_LIST_HEAD(&(*rqs)[i].entities);
		(*rqs)[i].nextX = (*rqs)[i].entity.x + RQ_XMARGIN;
	}
}

/* draw a runqueue */
void setRunqueue(SWFShape shape, unsigned thick, float width, float height) {
	SWFShape_setLine(shape,thick,0,0,255,255);
	SWFShape_movePenTo(shape,0,height-RQ_YMARGIN);
	SWFShape_drawLineTo(shape,width,height-RQ_YMARGIN);
}

void addToRunqueue(rq_t *rq, entity_t *e);

/*******************************************************************************
 * Bubble
 */

typedef struct bubble_s {
	entity_t entity;
	struct list_head heldentities;	/* held entities */
	float nextX;			/* position of next enqueued entity */
	int exploded;			/* whether it is exploded */
	SWFMorph morph;			/* current bubble morph */
	int morphRecurse;		/* an animation can involve a bubble several times, count that */
	entity_t *insertion;		/* entity being inserted */
} bubble_t;

void bubbleMorphStep(bubble_t *b, float ratio);

static inline bubble_t * bubble_of_entity(entity_t *e) {
	return tbx_container_of(e,bubble_t,entity);
}

bubble_t *newBubble (int prio, rq_t *initrq) {
	bubble_t *b = malloc(sizeof(*b));
	b->entity.x = b->entity.y = -1;
	b->entity.lastx = b->entity.lasty = -1;
#ifdef BUBBLES
	b->entity.width = CURVE*3;
	b->entity.height = CURVE*5;
	b->nextX = CURVE;
#endif
#ifdef TREES
	b->entity.width = CURVE;
	b->entity.height = CURVE;
	b->nextX = 0;
#endif
	b->entity.lastwidth = b->entity.lastheight = -1;
	b->entity.thick = BUBBLETHICK;
	b->entity.prio = prio;
	//b->entity.rq
	//b->entity.entity_list
	b->entity.type = BUBBLE;
	b->entity.lastitem = NULL;
	b->entity.lastshape = NULL;
	b->entity.holder = NULL;
	b->entity.lastholder = NULL;
	b->entity.bubble_holder = NULL;
	b->entity.leaving_holder = 0;
	b->entity.nospace = 0;
	INIT_LIST_HEAD(&b->heldentities);
	b->exploded = 0;
	b->morph = NULL;
	b->morphRecurse = 0;
	b->insertion = NULL;
	if (initrq)
		addToRunqueue(initrq, &b->entity);
	return b;
}

bubble_t *newBubblePtr (uint64_t ptr, rq_t *initrq) {
	bubble_t *b = newBubble(0, initrq);
	newPtr(ptr,b);
	return b;
}

bubble_t *getBubble (uint64_t ptr) {
	bubble_t *b = (bubble_t *) getPtr(ptr);
	if (!b)
		return newBubblePtr(ptr, NULL);
	else
		return b;
}

/*******************************************************************************
 * Thread
 */

typedef enum {
	THREAD_BLOCKED,
	THREAD_SLEEPING,
	THREAD_RUNNING,
	THREAD_DEAD,
} state_t;
typedef struct {
	entity_t entity;
	state_t state;
	char *name;
	int id, number;
	rq_t *initrq;
} thread_t;

static inline thread_t * thread_of_entity(entity_t *e) {
	return tbx_container_of(e,thread_t,entity);
}

void setThread(SWFShape shape, unsigned thick, float width, float height, int prio, state_t state, char *name) {
	float xStep = width/3, yStep = height/9;

	switch (state) {
	case THREAD_BLOCKED:
		SWFShape_setLine(shape,thick,0,0,0,255);
		break;
	case THREAD_SLEEPING:
		SWFShape_setLine(shape,thick,0,255,0,255);
		break;
	case THREAD_RUNNING:
		SWFShape_setLine(shape,thick,255,0,0,255);
		break;
	case THREAD_DEAD:
		SWFShape_setLine(shape,thick,200,200,200,255);
		break;
	}
	SWFShape_movePen(shape,    xStep,0);
	SWFShape_drawCurve(shape, 2*xStep,2*yStep,-xStep,yStep);
	SWFShape_drawCurve(shape,-2*xStep,2*yStep, xStep,yStep);
	SWFShape_drawCurve(shape, 2*xStep,2*yStep,-xStep,yStep);
	if (DISPPRIO && prio) {
		SWFShape_movePen(shape,xStep,CURVE);
		SWFShape_drawSizedGlyph(shape,font,'0'+prio,CURVE);
	}
	if (DISPNAME && name) {
		SWFShape_movePen(shape,0,CURVE);
		while (*name) {
			SWFShape_drawSizedGlyph(shape,font,*name,CURVE);
			SWFShape_movePen(shape,CURVE,0);
			name++;
		}
	}
}

thread_t *newThread (int prio, rq_t *initrq) {
	thread_t *t = malloc(sizeof(*t));
	t->entity.x = t->entity.y = -1;
	t->entity.lastx = t->entity.lasty = -1;
	t->entity.width = CURVE;
	t->entity.height = CURVE*3;
	t->entity.lastwidth = t->entity.lastheight = -1;
	t->entity.thick = THREADTHICK;
	t->entity.prio = prio;
	//b->entity.rq
	//b->entity.entity_list
	t->entity.type = THREAD;
	t->entity.lastitem = NULL;
	t->entity.lastshape = NULL;
	t->entity.holder = NULL;
	t->entity.lastholder = NULL;
	t->entity.bubble_holder = NULL;
	t->entity.leaving_holder = 0;
	t->entity.nospace = 0;
	t->state = THREAD_BLOCKED;
	t->name = NULL;
	t->id = -1;
	t->number = -1;
	t->initrq = initrq;
	return t;
}

thread_t *newThreadPtr (uint64_t ptr, rq_t *initrq) {
	thread_t *t = newThread(0, initrq);
	newPtr(ptr,t);
	return t;
}

void printfThread(uint64_t ptr, thread_t *t) {
	verbprintf("thread ");
	if (t->name)
		verbprintf("%s ",t->name);
	verbprintf("(%"PRIx64":%p)",ptr,t);
}

thread_t *getThread (uint64_t ptr) {
	thread_t *t = (thread_t *) getPtr(ptr);
	if (!t)
		return newThreadPtr(ptr, NULL);
	else
		return t;
}

#ifdef BUBBLES
void setBubble(SWFShape shape, float width, float height, int prio) {
	SWFShape_movePen(shape,CURVE,0);
	SWFShape_drawLine(shape,width-2*CURVE,0);
	SWFShape_drawCurve(shape,CURVE,0,0,CURVE);
	SWFShape_drawLine(shape,0,height-2*CURVE);
	SWFShape_drawCurve(shape,0,CURVE,-CURVE,0);
	SWFShape_drawLine(shape,-width+2*CURVE,0);
	SWFShape_drawCurve(shape,-CURVE,0,0,-CURVE);
	SWFShape_drawLine(shape,0,-height+2*CURVE);
	SWFShape_drawCurve(shape,0,-CURVE,CURVE,0);
	if (DISPPRIO && prio) {
		SWFShape_movePen(shape,-2*CURVE,-CURVE);
		SWFShape_drawSizedGlyph(shape,font,'0'+prio,CURVE);
	}
}

void setPlainBubble(SWFShape shape, unsigned thick, float width, float height, int prio) {
	SWFShape_setLine(shape,thick,0,0,0,255);
	setBubble(shape,width,height,prio);
}

void setExplodedBubble(SWFShape shape, unsigned thick, float width, float height, int prio) {
	SWFShape_setLine(shape,thick,0,0,0,128);
	setBubble(shape,width,height,prio);
}

void setBubbleRecur(SWFShape shape, bubble_t *b) {
	if (b->exploded)
		setExplodedBubble(shape,b->entity.thick,b->entity.width,b->entity.height,b->entity.prio);
	else
		setPlainBubble(shape,b->entity.thick,b->entity.width,b->entity.height,b->entity.prio);
	SWFDisplayItem_moveTo(b->entity.lastitem,b->entity.x,b->entity.y);
}
void setThreadRecur(SWFShape shape, thread_t *t) {
	setThread(shape,t->entity.thick,t->entity.width,t->entity.height,t->entity.prio,t->state,t->name);
}

float nextX;


#endif /* BUBBLES */
#ifdef TREES
/* in the case of trees, only the root bubble gets drawn, and should draw its
 * children */
void setBubbleRecur(SWFShape shape, bubble_t *b) {
	entity_t *e;
	if (!showEmptyBubbles && list_empty(&b->heldentities))
		return;
	SWFShape_setLine(shape,b->entity.thick,0,0,0,255);
	SWFShape_movePenTo(shape,b->entity.x+CURVE/2,b->entity.y);
	SWFShape_drawCircle(shape,CURVE/2);
	if (DISPPRIO && b->entity.prio) {
		SWFShape_movePenTo(shape,b->entity.x-CURVE,b->entity.y-CURVE);
		SWFShape_drawSizedGlyph(shape,font,'0'+b->entity.prio,CURVE);
	}
	list_for_each_entry(e,&b->heldentities,entity_list) {
		SWFShape_setLine(shape,b->entity.thick,0,0,0,255);
		SWFShape_movePenTo(shape,b->entity.x+CURVE/2,b->entity.y);
		SWFShape_drawLineTo(shape,e->x+CURVE/2,e->y);
		SWFShape_movePen(shape,-CURVE/2,0);
		setEntityRecur(shape,e);
	}
	if (b->insertion) {
		/* fake thread */
		SWFShape_setLine(shape,b->entity.thick,0,0,0,255);
		SWFShape_movePenTo(shape,b->entity.x+CURVE/2,b->entity.y);
		SWFShape_drawLineTo(shape,b->insertion->x+CURVE/2,b->insertion->y);
		SWFShape_movePen(shape,-CURVE/2,0);
		setEntityRecur(shape,b->insertion);
	}
}
void setThreadRecur(SWFShape shape, thread_t *t) {
	setThread(shape, t->entity.thick, t->entity.width, t->entity.height, t->entity.prio, t->state, t->name);
}
void setEntityRecur(SWFShape shape, entity_t *e) {
	switch(e->type) {
		case THREAD: return setThreadRecur(shape,thread_of_entity(e));
		case BUBBLE: return setBubbleRecur(shape,bubble_of_entity(e));
		case RUNQUEUE: return setRunqueue(shape,e->thick,e->x,e->y);
	}
}
#endif

void showEntity(entity_t *e);
void doEntity(entity_t *e) {
	SWFShape shape = e->lastshape = newSWFShape();
	switch(e->type) {
		case BUBBLE: {
			bubble_t *b = bubble_of_entity(e);
#ifndef TREES
			entity_t *el;
#endif

			e->lastitem = SWFMovie_add(movie,(SWFBlock)shape);
			setBubbleRecur(shape,b);
#ifndef TREES
			list_for_each_entry(el,&b->heldentities,entity_list)
				showEntity(el);
#endif
#ifdef BUBBLES
			SWFDisplayItem_moveTo(e->lastitem,e->x,e->y);
#endif
		}
		break;
		case THREAD: {
			thread_t *t = thread_of_entity(e);
			setThreadRecur(shape,t);
			e->lastitem = SWFMovie_add(movie,(SWFBlock)shape);
			SWFDisplayItem_moveTo(e->lastitem,e->x,e->y);
		}
		break;
		case RUNQUEUE: {
			rq_t *rq = rq_of_entity(e);
			setRunqueue(shape, rq->entity.thick, rq->entity.width, rq->entity.height);
			e->lastitem = SWFMovie_add(movie,(SWFBlock)shape);
			SWFDisplayItem_moveTo(e->lastitem,e->x,e->y);
		}
		break;
	}
}

void showEntity(entity_t *e) {
	if (e->lastitem
#ifdef TREES
		|| e->bubble_holder
#endif
		)
		/* already shown */
		return;
	doEntity(e);
}

void updateEntity(entity_t *e) {
	if (!e->lastitem) {
		if (e->bubble_holder)
			updateEntity(&e->bubble_holder->entity);
		return;
	}
	SWFDisplayItem_remove(e->lastitem);
	doEntity(e);
}

void hideEntity(entity_t *e) {
	if (!e->lastitem) gasp();
	SWFDisplayItem_remove(e->lastitem);
	e->lastitem = NULL;
}

void delThread(thread_t *t) {
	t->state = THREAD_DEAD;
	switchRunqueuesBegin(norq,&t->entity);
	switchRunqueuesBegin2(norq,&t->entity);
	doStepsBegin(j)
		switchRunqueuesStep(norq, &t->entity, j);
	doStepsEnd();
	switchRunqueuesEnd(norq, &t->entity);
	bubble_pause(DELAYTIME);
	list_del(&t->entity.rq);
	t->entity.holder = NULL;
	if (t->entity.bubble_holder) {
		list_del(&t->entity.entity_list);
		updateEntity(&t->entity.bubble_holder->entity);
		t->entity.bubble_holder = NULL;
	} else {
		hideEntity(&t->entity);
	}
	// XXX: berk. � marche pour l'instant car on est le dernier sur la liste
	norq->nextX -= t->entity.width+RQ_XMARGIN;
	// TODO: animation
}

void delBubble(bubble_t *b) {
	switchRunqueuesBegin(norq,&b->entity);
	switchRunqueuesBegin2(norq,&b->entity);
	doStepsBegin(j)
		switchRunqueuesStep(norq, &b->entity, j);
	doStepsEnd();
	switchRunqueuesEnd(norq, &b->entity);
	list_del(&b->entity.rq);
	b->entity.holder = NULL;
	if (b->entity.bubble_holder) {
		list_del(&b->entity.entity_list);
		b->entity.bubble_holder = NULL;
	}
	// XXX: berk. � marche pour l'instant car on est le dernier sur la liste
	norq->nextX -= b->entity.width+RQ_XMARGIN;
	// TODO: animation
}

/*******************************************************************************
 * Bubble morph
 */

void bubbleMorphBegin(bubble_t *b) {
	if (b->entity.lastitem) {
		b->morphRecurse++;
		if (!b->morph) {
			SWFShape shape;
			SWFDisplayItem_remove(b->entity.lastitem);
			b->morph = newSWFMorphShape();
			b->entity.lastitem = SWFMovie_add(movie,(SWFBlock)b->morph);
			shape = SWFMorph_getShape1(b->morph);
			setBubbleRecur(shape,b);
		}
	}
#ifdef TREES
	else
		if (b->entity.bubble_holder)
			bubbleMorphBegin(b->entity.bubble_holder);
#endif
}

void bubbleMorphBegin2(bubble_t *b) {
	if (b->morphRecurse) {
		if (!(--b->morphRecurse))
			setBubbleRecur(SWFMorph_getShape2(b->morph),b);
	}
#ifdef TREES
	else
		if (b->entity.bubble_holder)
			bubbleMorphBegin2(b->entity.bubble_holder);
#endif
}

int shown(entity_t *e) {
	return (!!e->lastitem)
#ifdef TREES
		|| (e->bubble_holder && shown(&e->bubble_holder->entity))
#endif
		;
}

void bubbleMorphStep(bubble_t *b, float ratio) {
	if (b->entity.lastitem)
		SWFDisplayItem_setRatio(b->entity.lastitem,ratio);
#ifdef TREES
	else
		if (b->entity.bubble_holder)
			bubbleMorphStep(b->entity.bubble_holder,ratio);
#endif
}

void bubbleMorphEnd(bubble_t *b) {
	if (b->entity.lastitem)
		b->morph = NULL;
#ifdef TREES
	else
		if (b->entity.bubble_holder)
			bubbleMorphEnd(b->entity.bubble_holder);
#endif
}

/*******************************************************************************
 * Move
 */

void entityMoveDeltaBegin(entity_t *e, float dx, float dy) {
#ifdef TREES
	if (e->type == BUBBLE)
		bubbleMorphBegin(bubble_of_entity(e));
	else if (e->bubble_holder)
		bubbleMorphBegin(e->bubble_holder);
#endif
	if (e->lastx<0) {
		e->lastx = e->x;
		e->lasty = e->y;
	}
	e->x += dx;
	e->y += dy;
	if (e->type == BUBBLE) {
		bubble_t *b = bubble_of_entity(e);
		entity_t *el;
		list_for_each_entry(el,&b->heldentities,entity_list)
			if (!el->leaving_holder
#ifdef TREES
			&& (el->holder == e)
#endif
				)
				entityMoveDeltaBegin(el, dx, dy);
	}
}

void entityMoveToBegin(entity_t *e, float x, float y) {
#ifdef TREES
	if (e->type == BUBBLE)
		bubbleMorphBegin(bubble_of_entity(e));
	else if (e->bubble_holder)
		bubbleMorphBegin(e->bubble_holder);
#endif
	if (e->lastx<0) {
		e->lastx = e->x;
		e->lasty = e->y;
	}
	e->x = x;
	e->y = y;
	if (e->type == BUBBLE) {
		bubble_t *b = bubble_of_entity(e);
		entity_t *el;
		list_for_each_entry(el,&b->heldentities,entity_list)
			if (!el->leaving_holder
#ifdef TREES
			&& (el->holder == e)
#endif
				)
				entityMoveDeltaBegin(el, e->x-e->lastx, e->y-e->lasty);
	}
}

void entityMoveBegin2(entity_t *e) {
	if (e->type == BUBBLE) {
#ifdef TREES
		bubbleMorphBegin2(bubble_of_entity(e));
#endif
		bubble_t *b = bubble_of_entity(e);
		entity_t *el;
		list_for_each_entry(el,&b->heldentities,entity_list)
			if (!el->leaving_holder
#ifdef TREES
			&& (el->holder == e)
#endif
				)
				entityMoveBegin2(el);
	}
#ifdef TREES
	else if (e->bubble_holder)
		bubbleMorphBegin2(e->bubble_holder);
#endif
}

void entityMoveStep(entity_t *e, float step) {
	if (e->lastitem)
#ifdef TREES
	if (e->type == THREAD)
#endif
		SWFDisplayItem_moveTo(e->lastitem,
				e->lastx+(e->x-e->lastx)*step,
				e->lasty+(e->y-e->lasty)*step);
	if (e->type == BUBBLE) {
		bubble_t *b = bubble_of_entity(e);
#ifdef TREES
		bubbleMorphStep(b,step);
#endif
#ifdef BUBBLES
		entity_t *el;
		list_for_each_entry(el,&b->heldentities,entity_list)
			if (!el->leaving_holder)
				entityMoveStep(el, step);
#endif
	}
#ifdef TREES
	else if (e->bubble_holder)
		bubbleMorphStep(e->bubble_holder,step);
#endif
}

void entityMoveEnd(entity_t *e) {
	e->lastx=-1;
	if (e->type == BUBBLE) {
		bubble_t *b = bubble_of_entity(e);
		entity_t *el;
#ifdef TREES
		bubbleMorphEnd(bubble_of_entity(e));
#endif
		list_for_each_entry(el,&b->heldentities,entity_list)
			if (!el->leaving_holder
#ifdef TREES
			&& (el->holder == e)
#endif
				)
				entityMoveEnd(el);
	}
#ifdef TREES
	else if (e->bubble_holder)
		bubbleMorphEnd(e->bubble_holder);
#endif
}

/*******************************************************************************
 * Add
 */

void addToRunqueueBegin(rq_t *rq, entity_t *e) {
	entityMoveToBegin(e, rq->nextX, rq->entity.y+rq->entity.height-e->height);
	rq->nextX += e->width+RQ_XMARGIN;
}
void addToRunqueueBegin2(rq_t *rq, entity_t *e) {
	entityMoveBegin2(e);
}

void addToRunqueueAtBegin(rq_t *rq, entity_t *e, float x) {
	entityMoveToBegin(e, x, rq->entity.y+rq->entity.height-e->height);
}

void addToRunqueueAtBegin2(rq_t *rq, entity_t *e) {
	entityMoveBegin2(e);
}

void addToRunqueueEnd(rq_t *rq, entity_t *e) {
	entityMoveEnd(e);
	if (e->holder)
		gasp();
	list_add_tail(&e->rq,&rq->entities);
	e->holder = &rq->entity;
}

void addToRunqueueAtEnd(rq_t *rq, entity_t *e, entity_t *after) {
	entityMoveEnd(e);
	if (e->holder)
		gasp();
	list_add(&e->rq,&after->rq);
	e->holder = &rq->entity;
}

void addToRunqueueStep(rq_t *rq, entity_t *e, float step) {
	entityMoveStep(e,step);
}

void addToRunqueue(rq_t *rq, entity_t *e) {
	addToRunqueueBegin(rq,e);
	addToRunqueueBegin2(rq,e);
	if (shown(e)) {
		doStepsBegin(j)
			addToRunqueueStep(rq, e, j);
		doStepsEnd();
	}
	addToRunqueueEnd(rq,e);
}

/*******************************************************************************
 * Remove
 */

void removeFromRunqueueBegin(rq_t *rq, entity_t *e) {
	entity_t *el;
	float width = e->width+RQ_XMARGIN;

	if (!e->nospace) {
		list_for_each_entry_after(el,&rq->entities,rq,e)
			entityMoveDeltaBegin(el, -width, 0);
		rq->nextX -= width;
	}
	e->lastholder = &rq->entity;
	e->leaving_holder = 1;
}

void removeFromRunqueueBegin2(rq_t *rq, entity_t *e) {
	entity_t *el;
	if (!e->nospace) {
		list_for_each_entry_after(el,&rq->entities,rq,e)
			entityMoveBegin2(el);
	}
}

void removeFromRunqueueEnd(rq_t *rq, entity_t *e) {
	entity_t *el;
	if (!e->nospace) {
		list_for_each_entry_after(el,&rq->entities,rq,e)
			entityMoveEnd(el);
	}
	list_del(&e->rq);
	e->holder = NULL;
	e->leaving_holder = 0;
}

/*******************************************************************************
 * Switch
 */

void switchRunqueuesBegin(rq_t *rq2, entity_t *e) {
	removeFromHolderBegin(e);
	addToRunqueueBegin(rq2, e);
}

void switchRunqueuesBegin2(rq_t *rq2, entity_t *e) {
	removeFromHolderBegin2(e);
	addToRunqueueBegin2(rq2, e);
}

void switchRunqueuesStep(rq_t *rq2, entity_t *e, float step) {
	removeFromHolderStep(e,step);
	addToRunqueueStep(rq2, e, step);
}

void switchRunqueuesEnd(rq_t *rq2, entity_t *e) {
	removeFromHolderEnd(e);
	addToRunqueueEnd(rq2, e);
	e->nospace = 0;
}

void switchRunqueues(rq_t *rq2, entity_t *e) {
	switchRunqueuesBegin(rq2,e);
	switchRunqueuesBegin2(rq2,e);
	if (shown(e))
		doStepsBegin(j)
			switchRunqueuesStep(rq2,e,j);
		doStepsEnd();
	switchRunqueuesEnd(rq2,e);
}

void growInRunqueueBegin(rq_t *rq, entity_t *e, float dx, float dy) {
	entity_t *el;

	list_for_each_entry_after(el,&rq->entities,rq,e)
		if (!el->leaving_holder)
			entityMoveDeltaBegin(el, dx, 0);
	rq->nextX += dx;
}

void growInRunqueueBegin2(rq_t *rq, entity_t *e) {
	entity_t *el;

	list_for_each_entry_after(el,&rq->entities,rq,e)
		if (!el->leaving_holder)
			entityMoveBegin2(el);
}

void changeInRunqueueStep(rq_t *rq, entity_t *e, float step) {
	if (!e->nospace) {
		entity_t *el;
		list_for_each_entry_after(el,&rq->entities,rq,e)
			if (!el->leaving_holder)
				entityMoveStep(el,step);
	}
}

void changeInRunqueueEnd(rq_t *rq, entity_t *e) {
	entity_t *el;

	list_for_each_entry_after(el,&rq->entities,rq,e)
		if (!el->leaving_holder)
			entityMoveEnd(el);
}

/*******************************************************************************
 * Bubble grow
 */

void growInBubbleBegin(bubble_t *b, entity_t *e, float dx, float dy) {
	entity_t *el;
	float mydy = 0;
	float newwidth;

	bubbleMorphBegin(b);

	b->entity.lastx = b->entity.x;
	b->entity.lasty = b->entity.y;
	b->entity.lastwidth = b->entity.width;

	list_for_each_entry_after(el,&b->heldentities,entity_list,e)
#ifdef TREES
		if (el->holder == &b->entity)
#endif
			entityMoveDeltaBegin(el, dx, 0);
	b->nextX += dx;
#ifdef BUBBLES
	if ((mydy = e->height  + 2*CURVE - b->entity.height) > 0) {
		b->entity.y -= mydy;
		b->entity.height += mydy;
	}
#endif
	newwidth = b->nextX-OVERLAP;
	if (newwidth < 
#ifdef BUBBLES
			3*
#endif
			CURVE
			)
		newwidth = 
#ifdef BUBBLES
			3*
#endif
			CURVE;
	if (newwidth!=b->entity.width || b->entity.y != b->entity.lasty) {
		growInHolderBegin(&b->entity,newwidth-b->entity.width,mydy);
		b->entity.width = newwidth;
	}
}

void growInBubbleBegin2(bubble_t *b, entity_t *e) {
	entity_t *el;

	bubbleMorphBegin2(b);
	list_for_each_entry_after(el,&b->heldentities,entity_list,e)
#ifdef TREES
		if (el->holder == &b->entity)
#endif
			entityMoveBegin2(el);
	if (b->entity.lastwidth!=b->entity.width || b->entity.y != b->entity.lasty)
		growInHolderBegin2(&b->entity);
}

void growInBubbleStep(bubble_t *b, entity_t *e, float step) {
	entity_t *el;

	bubbleMorphStep(b, step);

#ifdef BUBBLES
	SWFDisplayItem_moveTo(b->entity.lastitem,
			b->entity.lastx+(b->entity.x-b->entity.lastx)*step,
			b->entity.lasty+(b->entity.y-b->entity.lasty)*step
			);
#endif

	if (shown(&b->entity)) {
		list_for_each_entry_after(el,&b->heldentities,entity_list,e)
#ifdef TREES
			if (el->holder == &b->entity)
#endif
				entityMoveStep(el,step);

		if (b->entity.lastwidth!=b->entity.width || b->entity.y != b->entity.lasty)
			growInHolderStep(&b->entity,step);
	}
}

void growInBubbleEnd(bubble_t *b, entity_t *e) {
	entity_t *el;

	bubbleMorphEnd(b);
	list_for_each_entry_after(el,&b->heldentities,entity_list,e)
#ifdef TREES
		if (el->holder == &b->entity)
#endif
			entityMoveEnd(el);
	if (b->entity.lastwidth!=b->entity.width || b->entity.y != b->entity.lasty) {
		growInHolderEnd(&b->entity);
	}
}

/*******************************************************************************
 * Bubble remove
 */

void removeFromBubbleBegin(bubble_t *b, entity_t *e) {
	e->lastholder = &b->entity;
	e->leaving_holder = 1;
	if (!e->nospace)
		growInBubbleBegin(b,e,-e->width-CURVE,0);
}

void removeFromBubbleBegin2(bubble_t *b, entity_t *e) {
	if (!e->nospace)
		growInBubbleBegin2(b,e);
}

void removeFromBubbleStep(bubble_t *b, entity_t *e, float step) {
	if (!e->nospace)
		growInBubbleStep(b,e,step);
}

void removeFromBubbleEnd(bubble_t *b, entity_t *e) {
	if (!e->nospace)
		growInBubbleEnd(b,e);
	e->leaving_holder = 0;
#ifdef BUBBLES
	list_del(&e->entity_list);
	e->bubble_holder = NULL;
#endif
	e->holder = NULL;
}

/*******************************************************************************
 * Bubble insert
 */

void bubbleInsertEntity(bubble_t *b, entity_t *e) {
	float dx = 0;
	float dy = 0;
#ifdef BUBBLES
	rq_t *rq=NULL;

	if (b->exploded) {
		if (b->entity.holder->type != RUNQUEUE) {
			fprintf(stderr,"bubble exploded on something else than runqueue\n");
			gasp();
		}
		rq = rq_of_entity(b->entity.holder);
		if (e->holder) {
			switchRunqueuesBegin(rq,e);
			switchRunqueuesBegin2(rq,e);
		} else {
			addToRunqueueBegin(rq,e);
			addToRunqueueBegin2(rq,e);
		}
	} else
#endif
	{
		b->insertion = e;
		bubbleMorphBegin(b);
		b->insertion = NULL;

		b->entity.lastx = b->entity.x;
		b->entity.lasty = b->entity.y;

		removeFromHolderBegin(e);
#ifdef BUBBLES
		if ((dy = e->height+2*CURVE - b->entity.height) > 0) {
			b->entity.y -= dy;
			b->entity.height += dy;
		}
#endif

		entityMoveToBegin(e, b->entity.x+b->nextX, b->entity.y+
				OVERLAP+CURVE);

		b->nextX += e->width+CURVE;

		if (b->nextX-OVERLAP>b->entity.width) {
			growInHolderBegin(&b->entity,dx=(b->nextX-OVERLAP-b->entity.width),dy);
			b->entity.width = b->nextX-OVERLAP;
		}

#ifdef TREES
		if (e->bubble_holder)
			gasp();
		list_add_tail(&e->entity_list,&b->heldentities);
		e->bubble_holder = b;
#endif
		bubbleMorphBegin2(b);
		removeFromHolderBegin2(e);
		entityMoveBegin2(e);
		if (dx)
			growInHolderBegin2(&b->entity);
	}

	if (shown(&b->entity)) {
		doStepsBegin(j)
#ifdef BUBBLES
			if (b->exploded)
				if (e->holder)
					switchRunqueuesStep(rq,e,j);
				else
					entityMoveStep(e,j);
 			else
#endif
			{
				bubbleMorphStep(b,j);
#ifdef BUBBLES
				SWFDisplayItem_moveTo(b->entity.lastitem,
						b->entity.lastx+(b->entity.x-b->entity.lastx)*j,
						b->entity.lasty+(b->entity.y-b->entity.lasty)*j
						);
#endif
				removeFromHolderStep(e,j);
				entityMoveStep(e,j);
				if (dx) growInHolderStep(&b->entity,j);
			}
		doStepsEnd();
	}

#ifdef BUBBLES
	if (b->exploded) {
		if (e->holder)
			switchRunqueuesEnd(rq,e);
		else
			addToRunqueueEnd(rq,e);
	} else
#endif
	{
		removeFromHolderEnd(e);
		entityMoveEnd(e);
		if (dx)
			growInHolderEnd(&b->entity);
		bubbleMorphEnd(b);
#ifdef BUBBLES
		if (e->bubble_holder)
			gasp();
		list_add_tail(&e->entity_list,&b->heldentities);
		e->bubble_holder = b;
#endif
		e->holder = &b->entity;
	}
#ifdef TREES
	if (e->lastitem) {
		SWFDisplayItem_remove(e->lastitem);
		e->lastitem = NULL;
	}
#endif
}

void bubbleInsertBubble(bubble_t *bubble, bubble_t *little_bubble);
#define bubbleInsertBubble(b,lb) bubbleInsertEntity(b,&lb->entity)
void bubbleInsertThread(thread_t *bubble, thread_t *thread);
#define bubbleInsertThread(b,t) bubbleInsertEntity(b,&t->entity)

/*******************************************************************************
 * Bubble remove
 */

void bubbleRemoveEntity(bubble_t *b, entity_t *e) {
#ifndef TREES
	gasp();
#endif
	switchRunqueues(norq,e);
	list_del(&e->entity_list);
	if (e->bubble_holder != b)
		gasp();
	e->bubble_holder = NULL;
	updateEntity(&b->entity);
	showEntity(e);
	nextFrame(movie);
}

void bubbleRemoveBubble(bubble_t *bubble, bubble_t *little_bubble);
#define bubbleRemoveBubble(b,lb) bubbleRemoveEntity(b,&lb->entity)
void bubbleRemoveThread(thread_t *bubble, thread_t *thread);
#define bubbleRemoveThread(b,t) bubbleRemoveEntity(b,&t->entity)

/*******************************************************************************
 * Bubble explode
 */

#ifdef BUBBLES
void bubbleExplodeBegin(bubble_t *b) {
	entity_t *el;
	rq_t *rq;

	if (b->entity.holder->type != RUNQUEUE) {
		fprintf(stderr,"bubble exploding on something else than runqueue\n");
		gasp();
	}
	rq = rq_of_entity(b->entity.holder);
	bubbleMorphBegin(b);

	b->entity.lastx = b->entity.x;
	b->entity.lasty = b->entity.y;
	b->nextX = CURVE;

	//growInRunqueueBegin(rq,&b->entity,-b->nextX,0);
	list_for_each_entry(el,&b->heldentities,entity_list) {
		fprintf(stderr,"entity %p to rq %p\n",el,rq);
		el->nospace = 1;
		addToRunqueueAtBegin(rq,el,el->x);
	}

}
void bubbleExplodeBegin2(bubble_t *b) {
	entity_t *el;
	rq_t *rq;

	b->exploded = 1;
	bubbleMorphBegin2(b);
	list_for_each_entry(el,&b->heldentities,entity_list)
		addToRunqueueAtBegin2(rq,el);
}

void bubbleExplodeStep(bubble_t *b, float step) {
	entity_t *el;
	rq_t *rq;

	if (b->entity.holder->type != RUNQUEUE) {
		fprintf(stderr,"bubble exploding on something else than runqueue\n");
		gasp();
	}

	rq = rq_of_entity(b->entity.holder);
	bubbleMorphStep(b,step);
	SWFDisplayItem_moveTo(b->entity.lastitem,
			b->entity.lastx+(b->entity.x-b->entity.lastx)*step,
			b->entity.lasty+(b->entity.y-b->entity.lasty)*step
			);
	list_for_each_entry(el,&b->heldentities,entity_list)
		entityMoveStep(el,step);
	//changeInRunqueueStep(rq,&b->entity,step);
}

void bubbleExplodeEnd(bubble_t *b) {
	entity_t *el;
	rq_t *rq;

	if (b->entity.holder->type != RUNQUEUE) {
		fprintf(stderr,"bubble exploding on something else than runqueue\n");
		gasp();
	}

	rq = rq_of_entity(b->entity.holder);

	list_for_each_entry_reverse(el,&b->heldentities,entity_list) {
		//el->nospace = 0;
		addToRunqueueAtEnd(rq,el,&b->entity);
	}
	INIT_LIST_HEAD(&b->heldentities);
	bubbleMorphEnd(b);
}

void bubbleExplode(bubble_t *b) {
	bubbleExplodeBegin(b);
	bubbleExplodeBegin2(b);
	if (b->entity.lastitem) {
		doStepsBegin(j)
			bubbleExplodeStep(b,j);
		doStepsEnd();
	}
	bubbleExplodeEnd(b);
}
#endif /* BUBBLES */
#ifdef TREES
void bubbleExplodeBegin(bubble_t *b) { }
void bubbleExplodeBegin2(bubble_t *b) { }
void bubbleExplodeStep(bubble_t *b, float step) { }
void bubbleExplodeEnd(bubble_t *b) { }
void bubbleExplode(bubble_t *b) { }
#endif

/*******************************************************************************
 * Generic
 */

void removeFromHolderBegin(entity_t *e) {
	if (!e->holder) return;
	switch (e->holder->type) {
	case RUNQUEUE:
		removeFromRunqueueBegin(rq_of_entity(e->holder),e);
		break;
	case BUBBLE:
		removeFromBubbleBegin(bubble_of_entity(e->holder),e);
		break;
	default:
		fprintf(stderr,"uuh, %p's holder of type %d ?!\n",e,e->holder->type);
		gasp();
	}
}

void removeFromHolderBegin2(entity_t *e) {
	if (!e->holder) return;
	switch (e->holder->type) {
	case RUNQUEUE:
		removeFromRunqueueBegin2(rq_of_entity(e->holder),e);
		break;
	case BUBBLE:
		removeFromBubbleBegin2(bubble_of_entity(e->holder),e);
		break;
	default:
		fprintf(stderr,"uuh, %p's holder of type %d ?!\n",e,e->holder->type);
		gasp();
	}
}

void removeFromHolderStep(entity_t *e, float step) {
	if (!e->holder) return;
	switch (e->lastholder->type) {
	case RUNQUEUE:
		changeInRunqueueStep(rq_of_entity(e->lastholder),e,step);
		break;
	case BUBBLE:
		removeFromBubbleStep(bubble_of_entity(e->holder),e,step);
		break;
	default:
		fprintf(stderr,"uuh, %p's holder of type %d ?!\n",e,e->holder->type);
		gasp();
	}
}

void removeFromHolderEnd(entity_t *e) {
	if (!e->holder) return;
	switch (e->holder->type) {
	case RUNQUEUE:
		removeFromRunqueueEnd(rq_of_entity(e->holder),e);
		break;
	case BUBBLE:
		removeFromBubbleEnd(bubble_of_entity(e->holder),e);
		break;
	default:
		fprintf(stderr,"uuh, %p's holder of type %d ?!\n",e,e->holder->type);
		gasp();
	}
}

void growInHolderBegin(entity_t *e, float dx, float dy) {
	switch (e->holder->type) {
	case RUNQUEUE:
		growInRunqueueBegin(rq_of_entity(e->holder),e,dx,dy);
		break;
	case BUBBLE:
		growInBubbleBegin(bubble_of_entity(e->holder), e, dx, dy);
		break;
	default:
		fprintf(stderr,"uuh, %p's holder of type %d ?!\n",e,e->holder->type);
		gasp();
	}
}

void growInHolderBegin2(entity_t *e) {
	switch (e->holder->type) {
	case RUNQUEUE:
		growInRunqueueBegin2(rq_of_entity(e->holder),e);
		break;
	case BUBBLE:
		growInBubbleBegin2(bubble_of_entity(e->holder), e);
		break;
	default:
		fprintf(stderr,"uuh, %p's holder of type %d ?!\n",e,e->holder->type);
		gasp();
	}
}

void growInHolderStep(entity_t *e, float step) {
	switch (e->holder->type) {
	case RUNQUEUE:
		changeInRunqueueStep(rq_of_entity(e->holder),e,step);
		break;
	case BUBBLE:
		growInBubbleStep(bubble_of_entity(e->holder), e, step);
		break;
	default:
		fprintf(stderr,"uuh, %p's holder of type %d ?!\n",e,e->holder->type);
		gasp();
	}
}

void growInHolderEnd(entity_t *e) {
	switch (e->holder->type) {
	case RUNQUEUE:
		changeInRunqueueEnd(rq_of_entity(e->holder),e);
		break;
	case BUBBLE:
		growInBubbleEnd(bubble_of_entity(e->holder), e);
		break;
	default:
		fprintf(stderr,"uuh, %p's holder of type %d ?!\n",e,e->holder->type);
		gasp();
	}
}


/*******************************************************************************
 * Main
 */

//static bubble_t *bigb;

static void error(const char *msg, ...) {
	va_list args;
	static int recur = 0;
	
	va_start(args, msg);
	vfprintf(stderr,msg, args);
	va_end(args);
	if (!recur++) {
		fprintf(stderr,"saving to rescue.swf\n");
		SWFMovie_save(movie,"rescue.swf");
		fprintf(stderr,"saved to rescue.swf\n");
	}
	abort();
}

static void sig(int sig) {
	error("got signal %d\n", sig);
}

static void usage(char *argv0) {
	fprintf(stderr,"usage: %s [options] [ prof_file ]\n", basename(argv0));
	fprintf(stderr,"  -f fontfile		use the specified fontfile\n");
	fprintf(stderr,"  -d			delay the animation start to the START_PLAYING event\n");
	fprintf(stderr,"  -v			verbose log\n");
	fprintf(stderr,"  -x width		movie width\n");
	fprintf(stderr,"  -y height		movie height\n");
	fprintf(stderr,"  -t thickness		thread/bubble/runqueue thickness\n");
	fprintf(stderr,"  -c curve		curve size\n");
	fprintf(stderr,"  -o optime		operation time (in sec.)\n");
	fprintf(stderr,"  -p			display priorities\n");
	fprintf(stderr,"  -n			display thread names\n");
	fprintf(stderr,"  -s			display system threads\n");
	fprintf(stderr,"  -e			display empty bubbles\n");
}

int main(int argc, char *argv[]) {
	rq_t **rqs=NULL;
	FILE *f;
	char c;

	while((c=getopt(argc,argv,":fdvx:y:t:c:o:hpnse")) != EOF)
		switch(c) {
		case 'f':
			fontfile = optarg;
			break;
		case 'd':
			// delayed start
			playing = 0;
			break;
		case 'v':
			verbose = 1;
			break;

#define GETVAL(var, f) \
			if ((var = ato##f(optarg)) <= 0) { \
				fprintf(stderr,"bad "#var" '%s'\n", optarg); \
				exit(1); \
			}
		case 'x':
			GETVAL(MOVIEX,i)
			break;
		case 'y':
			GETVAL(MOVIEY,i)
			break;
		case 't':
			GETVAL(thick,f)
			break;
		case 'c':
			GETVAL(CURVE,f)
			break;
		case 'o':
			GETVAL(OPTIME,f)
			break;
		case 'p':
			DISPPRIO = 1;
			break;
		case 'n':
			DISPNAME = 1;
			break;
		case 's':
			showSystem = 1;
			break;
		case 'e':
			showEmptyBubbles = 1;
			break;
		case ':':
			fprintf(stderr,"missing parameter to switch %c\n", optopt);
		case '?':
			usage(argv[0]);
			exit(1);
		case 'h':
			usage(argv[0]);
			exit(0);
		}

	Ming_init();

	Ming_setErrorFunction(error);
	signal(SIGSEGV,sig);
	signal(SIGINT,sig);
	f = fopen(fontfile,"r");
	if (!f) {
		perror("Warning: could not open font file, will not able to print thread priority");
		fprintf(stderr,"(tried %s. Use -f option for changing this)\n",fontfile);
	}

	/* pause macro */
	stop = compileSWFActionCode(" if (!stopped) { stop(); stopped=1; } else { play(); stopped=0; }");

	/* used font */
	font = (SWFFont) loadSWFFontFromFile(f);
	if (!font)
		perror("can't load font");

	/* movie */
	movie = newSWFMovie();
	//SWFMovie_setBackground(movie,0,0,0);
	SWFMovie_setRate(movie,RATE);
	SWFMovie_setDimension(movie,MOVIEX,MOVIEY);

	pause_shape = newSWFShape();
	SWFShape_setLine(pause_shape,thick,0,0,0,255);
	SWFShape_movePenTo(pause_shape, -CURVE/2, 0);
	SWFShape_drawLineTo(pause_shape, CURVE/2, 0);
	pause_item = SWFMovie_add(movie,(SWFBlock)pause_shape);
	SWFDisplayItem_moveTo(pause_item, CURVE/2, MOVIEY-CURVE/2);

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
		SWFButton_addShape(button, (void*)shape, SWFBUTTON_HIT);
		SWFButton_addAction(button, stop, SWFBUTTON_MOUSEUP);
		item = SWFMovie_add(movie,(SWFBlock)button);
		SWFDisplayItem_moveTo(item,0,0);

		button = newSWFButton();
		SWFButton_addShape(button, (void*)newArrow(BSIZE,BSIZE,1), SWFBUTTON_UP|SWFBUTTON_DOWN|SWFBUTTON_OVER|SWFBUTTON_HIT);
		SWFButton_addAction(button, 
	compileSWFActionCode(" nextFrame(); if (!stopped) play();")
				, SWFBUTTON_MOUSEDOWN|SWFBUTTON_OVERDOWNTOIDLE);
		item = SWFMovie_add(movie,(SWFBlock)button);
		SWFDisplayItem_moveTo(item,MOVIEX-BSIZE,MOVIEY-BSIZE);

		button = newSWFButton();
		SWFButton_addShape(button, (void*)newArrow(BSIZE,BSIZE,1), SWFBUTTON_UP|SWFBUTTON_DOWN|SWFBUTTON_OVER|SWFBUTTON_HIT);
		SWFButton_addAction(button, 
	compileSWFActionCode(" for (i=0;i<16;i++) nextFrame(); if (!stopped) play(); ")
				, SWFBUTTON_MOUSEUP);
		item = SWFMovie_add(movie,(SWFBlock)button);
		SWFDisplayItem_moveTo(item,MOVIEX-2*BSIZE,MOVIEY-BSIZE);

		button = newSWFButton();
		SWFButton_addShape(button, (void*)newArrow(BSIZE,BSIZE,0), SWFBUTTON_UP|SWFBUTTON_DOWN|SWFBUTTON_OVER|SWFBUTTON_HIT);
		SWFButton_addAction(button, 
	compileSWFActionCode(" for (i=0;i<16;i++) prevFrame();  if (!stopped) play();")
				, SWFBUTTON_MOUSEUP);
		item = SWFMovie_add(movie,(SWFBlock)button);
		SWFDisplayItem_moveTo(item,MOVIEX-3*BSIZE,MOVIEY-BSIZE);

		button = newSWFButton();
		SWFButton_addShape(button, (void*)newArrow(BSIZE,BSIZE,0), SWFBUTTON_UP|SWFBUTTON_DOWN|SWFBUTTON_OVER|SWFBUTTON_HIT);
		SWFButton_addAction(button, 
	compileSWFActionCode(" prevFrame();  if (!stopped) play();")
				, SWFBUTTON_MOUSEDOWN|SWFBUTTON_OVERDOWNTOIDLE);
		item = SWFMovie_add(movie,(SWFBlock)button);
		SWFDisplayItem_moveTo(item,MOVIEX-4*BSIZE,MOVIEY-BSIZE);
	}

	/* The hidden "norq" runqueue */
	setRqs(&norq,1,0,0,MOVIEX,
#ifdef BUBBLES
			200
#endif
#ifdef TREES
			2*CURVE
#endif
			);

/*******************************************************************************
 * FXT: automatically generated from trace
 */
#ifdef FXT
if (optind != argc) {
	fxt_t fut;
	struct fxt_ev ev;
	unsigned nrqlevels = 0, *rqnums = NULL;
	fxt_blockev_t block;
	unsigned keymask = 0;
	int ret;

	if (optind != argc-1) {
		fprintf(stderr,"too many files, only one accepted\n");
		usage(argv[0]);
		exit(1);
	}

	if (!(fut = fxt_open(argv[optind]))) {
		perror("fxt_open");
		exit(1);
	}
	block = fxt_blockev_enter(fut);
	hcreate(1024);

	while (!(ret = fxt_next_ev(block, FXT_EV_TYPE_64, &ev))) {
		static int i;
		i++;
		if (!(i%1000))
			fprintf(stderr,"\r%d",i);
		if (ev.ev64.code == FUT_KEYCHANGE_CODE)
			keymask = ev.ev64.param[0];
		switch (ev.ev64.code) {
#ifdef BUBBLE_SCHED_NEW
			case BUBBLE_SCHED_NEW: {
				bubble_t *b = newBubblePtr(ev.ev64.param[0], norq);
				verbprintf("new bubble %p -> %p\n", (void *)(intptr_t)ev.ev64.param[0], b);
				//showEntity(&b->entity);
				break;
			}
			case SCHED_SETPRIO: {
				entity_t *e = getEntity(ev.ev64.param[0]);
				e->prio = ev.ev64.param[1];
				verbprintf("%s %p(%p) priority set to %"PRIi64"\n", e->type==BUBBLE?"bubble":"thread",(void *)(intptr_t)ev.ev64.param[0],e,ev.ev64.param[1]);
				updateEntity(e);
				nextFrame(movie);
				break;
			}
#ifdef BUBBLE_SCHED_EXPLODE
			case BUBBLE_SCHED_CLOSE: {
				bubble_t *b = getBubble(ev.ev64.param[0]);
				verbprintf("bubble %p(%p) close\n", (void *)(intptr_t)ev.ev64.param[0],b);
				break;
			}
			case BUBBLE_SCHED_CLOSING: {
				bubble_t *b = getBubble(ev.ev64.param[0]);
				verbprintf("bubble %p(%p) closing\n", (void *)(intptr_t)ev.ev64.param[0],b);
				break;
			}
			case BUBBLE_SCHED_CLOSED: {
				bubble_t *b = getBubble(ev.ev64.param[0]);
				verbprintf("bubble %p(%p) closed\n", (void *)(intptr_t)ev.ev64.param[0],b);
				break;
			}
			case BUBBLE_SCHED_EXPLODE: {
				bubble_t *b = getBubble(ev.ev64.param[0]);
				verbprintf("bubble %p(%p) exploding on rq %p\n", (void *)(intptr_t)ev.ev64.param[0],b,b->entity.holder);
				bubbleExplode(b);
				break;
			}
			case BUBBLE_SCHED_GOINGBACK: {
				uint64_t t = ev.ev64.param[0];
				thread_t *e = getThread(t);
				bubble_t *b = getBubble(ev.ev64.param[1]);
				printfThread(t,e);
				verbprintf(" going back in bubble %p(%p)\n", (void *)(intptr_t)ev.ev64.param[1], b);
				bubbleInsertThread(b,e);
				break;
			}
#endif
			case BUBBLE_SCHED_SWITCHRQ: {
				entity_t *e = getEntity(ev.ev64.param[0]);
				rq_t *rq = getRunqueue(ev.ev64.param[1]);
				if (!e) break;
				verbprintf("%s %p(%p) switching to %p (%p)\n", e->type==BUBBLE?"bubble":"thread",(void *)(intptr_t)ev.ev64.param[0], e, (void *)(intptr_t)ev.ev64.param[1], rq);
				if (showSystem || e->type!=THREAD || thread_of_entity(e)->number >= 0)
					switchRunqueues(rq, e);
				break;
			}
			case BUBBLE_SCHED_INSERT_BUBBLE: {
				bubble_t *e = getBubble(ev.ev64.param[0]);
				bubble_t *b = getBubble(ev.ev64.param[1]);
				verbprintf("bubble %p(%p) inserted in bubble %p(%p)\n", (void *)(intptr_t)ev.ev64.param[0], e, (void *)(intptr_t)ev.ev64.param[1], b);
				showEntity(&b->entity);
				bubbleInsertBubble(b,e);
				break;
			}
			case BUBBLE_SCHED_INSERT_THREAD: {
				uint64_t t = ev.ev64.param[0];
				thread_t *e = getThread(t);
				bubble_t *b = getBubble(ev.ev64.param[1]);
				printfThread(t,e);
				verbprintf(" inserted in bubble %p(%p)\n", (void *)(intptr_t)ev.ev64.param[1], b);
				if (showSystem || e->number>=0) {
					showEntity(&b->entity);
					bubbleInsertThread(b,e);
				}
				break;
			}
			case BUBBLE_SCHED_REMOVE_BUBBLE: {
				bubble_t *e = getBubble(ev.ev64.param[0]);
				bubble_t *b = getBubble(ev.ev64.param[1]);
				verbprintf("bubble %p(%p) inserted in bubble %p(%p)\n", (void *)(intptr_t)ev.ev64.param[0], e, (void *)(intptr_t)ev.ev64.param[1], b);
				showEntity(&b->entity);
				bubbleRemoveBubble(b,e);
				break;
			}
			case BUBBLE_SCHED_REMOVE_THREAD: {
				uint64_t t = ev.ev64.param[0];
				thread_t *e = getThread(t);
				bubble_t *b = getBubble(ev.ev64.param[1]);
				printfThread(t,e);
				verbprintf(" inserted in bubble %p(%p)\n", (void *)(intptr_t)ev.ev64.param[1], b);
				if (showSystem || e->number>=0) {
					showEntity(&b->entity);
					bubbleRemoveThread(b,e);
				}
				break;
			}
			case BUBBLE_SCHED_WAKE: {
				bubble_t *b = getBubble(ev.ev64.param[0]);
				rq_t *rq = getRunqueue(ev.ev64.param[1]);
				verbprintf("bubble %p(%p) waking up on runqueue %p(%p)\n", (void *)(intptr_t)ev.ev64.param[0], b, (void *)(intptr_t)ev.ev64.param[1], rq);
				switchRunqueues(rq, &b->entity);
				break;
			}
			case BUBBLE_SCHED_JOIN: {
				bubble_t *b = getBubble(ev.ev64.param[0]);
				verbprintf("bubble %p(%p) join\n", (void *)(intptr_t)ev.ev64.param[0], b);
				delBubble(b);
				break;
			}
#endif
			case FUT_RQS_NEWLEVEL: {
				unsigned num = ev.ev64.param[0];
				unsigned rqlevel = nrqlevels++;
				rqnums = realloc(rqnums,(nrqlevels)*sizeof(*rqnums));
				rqnums[rqlevel] = 0;
				rqs = realloc(rqs,(nrqlevels)*sizeof(*rqs));
				setRqs(&rqs[rqlevel],num,0,(rqlevel)*150+
#ifdef BUBBLES
						200+
#endif
						100
						,MOVIEX,150);
				for (i=0;i<num;i++)
					showEntity(&rqs[rqlevel][i].entity);
				verbprintf("new runqueue level %u with %d rqs\n", rqlevel, num);
				break;
			}
			case FUT_RQS_NEWLWPRQ: {
				unsigned rqnum = ev.ev64.param[0];
				unsigned rqlevel = nrqlevels-1;
				/* eux peuvent �re dans le d�ordre */
				newPtr(ev.ev64.param[1],&rqs[rqlevel][rqnum]);
				verbprintf("new lwp runqueue %d at %p -> %p\n",rqnum,(void *)(intptr_t)ev.ev64.param[1],&rqs[rqlevel][rqnum]);
				break;
			}
			case FUT_RQS_NEWRQ: {
				unsigned rqlevel = ev.ev64.param[0];
				unsigned rqnum;
				if (rqlevel == -1) {
					rqnum = 0;
					newPtr(ev.ev64.param[1],norq);
					verbprintf("dontsched runqueue %u.%u at %p -> %p\n",rqlevel,rqnum,(void *)(intptr_t)ev.ev64.param[1],norq);
				} else {
					rqnum = rqnums[rqlevel]++;
					newPtr(ev.ev64.param[1],&rqs[rqlevel][rqnum]);
					verbprintf("new runqueue %d.%d at %p -> %p\n",rqlevel,rqnum,(void *)(intptr_t)ev.ev64.param[1],&rqs[rqlevel][rqnum]);
				}
				break;
			}
#ifdef RQ_LOCK
			case RQ_LOCK: {
				rq_t *rq = getRunqueue(ev.ev64.param[0]);
				verbprintf("rq %p (%p) locked\n",rq,(void*)(intptr_t)ev.ev64.param[0]);
				if (!rq) break;
				rq->entity.thick = 2*RQTHICK;
				updateEntity(&rq->entity);
				break;
			}
#endif
#ifdef RQ_UNLOCK
			case RQ_UNLOCK: {
				rq_t *rq = getRunqueue(ev.ev64.param[0]);
				verbprintf("rq %p (%p) unlocked\n",rq,(void*)(intptr_t)ev.ev64.param[0]);
				if (!rq) break;
				rq->entity.thick = RQTHICK;
				updateEntity(&rq->entity);
				break;
			}
#endif
			case FUT_SETUP_CODE:
			case FUT_RESET_CODE:
			case FUT_CALIBRATE0_CODE:
			case FUT_CALIBRATE1_CODE:
			case FUT_CALIBRATE2_CODE:
			case FUT_NEW_LWP_CODE:
			case FUT_GCC_INSTRUMENT_ENTRY_CODE:
			case FUT_GCC_INSTRUMENT_EXIT_CODE:
				break;
#ifdef SCHED_IDLE_START
			case SCHED_IDLE_START: {
				int n;
				n = (int)ev.ev64.param[0];
				break;
			}
#endif
#ifdef SCHED_IDLE_STOP
			case SCHED_IDLE_STOP: {
				int n;
				n = (int)ev.ev64.param[0];
				break;
			}
#endif
#ifdef FUT_START_PLAYING
			case FUT_START_PLAYING: {
				playing = 1;
				verbprintf("start playing\n");
				break;
			}
#endif
			default:
			if (keymask) switch (ev.ev64.code) {
				case FUT_THREAD_BIRTH_CODE: {
					uint64_t th = ev.ev64.param[0];
					thread_t *t = newThreadPtr(th, norq);
					verbprintf("new ");
					printfThread(th,t);
					verbprintf("\n");
					//showEntity(&t->entity);
					break;
				}
				case FUT_THREAD_DEATH_CODE: {
					uint64_t th = ev.ev64.param[0];
					thread_t *t = getThread(th);
					printfThread(th,t);
					verbprintf(" death\n");
					if (showSystem || t->number >= 0)
						delThread(t);
					delPtr(ev.ev64.param[0]);
					// on peut en avoir encore besoin... free(t);
					// penser �free(t->name) aussi
					break;
				}
				case FUT_SET_THREAD_NAME_CODE: {
					uint64_t th = ev.ev64.param[0];
					thread_t *t = getThread(th);
					char name[16];
					uint32_t *ptr = (uint32_t *) name;
					ptr[0] = ev.ev64.param[1];
					ptr[1] = ev.ev64.param[2];
					ptr[2] = ev.ev64.param[3];
					ptr[3] = ev.ev64.param[4];
					name[15] = 0;
					printfThread(th,t);
					verbprintf(" named %s\n", name);
					t->name=strdup(name);
					updateEntity(&t->entity);
					if (showSystem || t->number >= 0) {
						nextFrame(movie);
					}
					break;
				}
				case SET_THREAD_ID: {
					uint64_t th = ev.ev64.param[0];
					thread_t *t = getThread(th);
					int id = ev.ev64.param[1];
					printfThread(th,t);
					verbprintf(" id %d\n", id);
					t->id=id;
					break;
				}
				case SET_THREAD_NUMBER: {
					uint64_t th = ev.ev64.param[0];
					thread_t *t = getThread(th);
					int number = ev.ev64.param[1];
					printfThread(th,t);
					verbprintf(" number %d\n", number);
					t->number=number;
					if (showSystem || number >= 0) {
						if (t->initrq && !t->entity.holder)
							addToRunqueue(t->initrq, &t->entity);
						showEntity(&t->entity);
						nextFrame(movie);
					} else
						updateEntity(&t->entity);
					break;
				}
				case SCHED_TICK: {
					bubble_pause(DELAYTIME);
					break;
				}
				case SCHED_THREAD_BLOCKED: {
					uint64_t th = ev.ev64.user.tid;
					thread_t *t = getThread(th);
					if (t->entity.type!=THREAD) gasp();
					printfThread(th,t);
					verbprintf(" going to sleep\n");
					if (t->state == THREAD_BLOCKED) break;
					t->state = THREAD_BLOCKED;
					updateEntity(&t->entity);
					if (showSystem || t->number >= 0)
						bubble_pause(DELAYTIME);
					break;
				}
				case SCHED_THREAD_WAKE: {
					uint64_t th = ev.ev64.user.tid;
					thread_t *t = getThread(th);
					uint64_t thw = ev.ev64.param[0];
					thread_t *tw = getThread(thw);
					if (t->entity.type!=THREAD) gasp();
					printfThread(th,t);
					verbprintf(" waking up\n");
					printfThread(thw,tw);
					verbprintf("\n");
					if (t->state == THREAD_SLEEPING) break;
					t->state = THREAD_SLEEPING;
					updateEntity(&t->entity);
					if (showSystem || t->number >= 0)
						bubble_pause(DELAYTIME);
					break;
				}
#ifdef SCHED_RESCHED_LWP
				case SCHED_RESCHED_LWP: {
					int from = ev.ev64.param[0];
					int to = ev.ev64.param[1];
					verbprintf("lwp %d rescheduling lwp %d\n",from,to);
				}
#endif
				case FUT_SWITCH_TO_CODE: {
					uint64_t thprev = ev.ev64.user.tid;
					thread_t *tprev = getThread(thprev);
					uint64_t thnext = ev.ev64.param[0];
					thread_t *tnext = getThread(thnext);
					if (tprev==tnext) gasp();
					if (tprev->entity.type!=THREAD) gasp();
					if (tnext->entity.type!=THREAD) gasp();
					verbprintf("switch from ");
					printfThread(thprev,tprev);
					verbprintf(" to ");
					printfThread(thnext,tnext);
					verbprintf("\n");
					if (tprev->state == THREAD_RUNNING) {
						tprev->state = THREAD_SLEEPING;
						updateEntity(&tprev->entity);
					}
					if (tnext->state != THREAD_RUNNING) {
						tnext->state = THREAD_RUNNING;
						updateEntity(&tnext->entity);
					}
					if (showSystem || tprev->number>=0 || tnext->number>=0)
						bubble_pause(DELAYTIME);
#if 0
					if (tprev->active) {
						entityMoveDeltaBegin(&tprev->entity,0,0);
						tprev->active = 0;
						entityMoveBegin2(&tprev->entity);
						doStepsBegin(step)
							entityMoveStep(&tprev->entity,step);
						doStepsEnd();
						entityMoveEnd(&tprev->entity);
					}
					if (!tnext->active) {

						entityMoveDeltaBegin(&tnext->entity,0,0);
						tnext->active = 1;
						entityMoveBegin2(&tnext->entity);
						doStepsBegin(step)
							entityMoveStep(&tnext->entity,step);
						doStepsEnd();
						entityMoveEnd(&tnext->entity);
					}
#endif
					break;
				}
				default:
					verbprintf("%16"PRIu64" %"PRIx64" %p %010"PRIx64" %1u",ev.ev64.time ,ev.ev64.code, (void*)(uintptr_t)ev.ev64.user.tid ,ev.ev64.code ,ev.ev64.nb_params);
					for (i=0;i<ev.ev64.nb_params;i++)
						verbprintf(" %010"PRIx64, ev.ev64.param[i]);
					verbprintf("\n");
					break;
			}
		}

		fflush(stdout);
#if 0
		{ static int pair = 0;
		// Sauvegarde r�uli�e, pour d�ecter rapidement les probl�es
		SWFMovie_save(movie,(pair^=1)?"autobulles.swf":"autobulles2.swf");
		}
#endif
	}
	switch(ret) {
		case FXT_EV_EOT: break;
		case FXT_EV_ERROR: perror("fxt_next_ev"); break;
		case FXT_EV_TYPEERROR: fprintf(stderr,"wrong trace word size\n"); break;
	}
	printf("saving to autobulles.swf\n");
	SWFMovie_save(movie,"autobulles.swf");
	printf("saved as autobulles.swf\n");
} else
#endif /* FXT */
{


/*******************************************************************************
 * Manual build
 */
	/* 3 runqueue levels */
	rqs = malloc(3*sizeof(*rqs));

#ifndef SHOWBUILD
#ifdef SHOWPRIO
	setRqs(&rqs[0],1,0,0,MOVIEX,150);
	setRqs(&rqs[1],2,0,150,MOVIEX,150);
#else
	setRqs(&rqs[0],1,0,50,MOVIEX,150);
	setRqs(&rqs[1],2,0,200,MOVIEX,150);
	setRqs(&rqs[2],4,0,350,MOVIEX,150);
#endif

	/* show runqueues */
	showEntity(&rqs[0][0].entity);
	showEntity(&rqs[1][0].entity);
	showEntity(&rqs[1][1].entity);
#ifndef SHOWPRIO
	showEntity(&rqs[2][0].entity);
	showEntity(&rqs[2][1].entity);
	showEntity(&rqs[2][2].entity);
	showEntity(&rqs[2][3].entity);
#endif
#endif

	{
#ifndef SHOWPRIO
		/* d�onstration bulles simples */
		bubble_t *b1, *b2, *b;
		thread_t *t1, *t2, *t3, *t4, *t5;
		t1 = newThread(0, norq);
		t2 = newThread(0, norq);
		t3 = newThread(0, norq);
		t4 = newThread(0, norq);
		t5 = newThread(0, norq);
		fprintf(stderr,"t1=%p, t2=%p, t3=%p, t4=%p, t5=%p\n",t1,t2,t3,t4,t5);

#ifdef SHOWBUILD
		showEntity(&t1->entity);
		showEntity(&t2->entity);
		showEntity(&t3->entity);
		showEntity(&t4->entity);
		showEntity(&t5->entity);
		bubble_pause(1);
#endif /* SHOWBUILD */


		b1 = newBubble(0, norq);

#ifdef SHOWBUILD
		showEntity(&b1->entity);
		bubble_pause(1);
#endif /* SHOWBUILD */
		b2 = newBubble(0, norq);
#ifdef SHOWBUILD
		showEntity(&b2->entity);
		bubble_pause(1);
#endif /* SHOWBUILD */

		bubbleInsertThread(b1,t1);
		bubbleInsertThread(b1,t2);

#ifdef SHOWBUILD
		bubble_pause(1);
#endif /* SHOWBUILD */
		bubbleInsertThread(b2,t3);
		bubbleInsertThread(b2,t4);
#ifdef SHOWBUILD
		bubble_pause(1);
#endif
		b = newBubble(0, norq);
#ifdef SHOWBUILD
		showEntity(&b->entity);
		bubble_pause(1);
#endif
		bubbleInsertBubble(b,b2);
		bubbleInsertBubble(b,b1);
		//bubbleInsertThread(b,t5);

#ifndef SHOWBUILD
		showEntity(&b->entity);
#endif
		fprintf(stderr,"b1=%p, b2=%p, b=%p\n",b1,b2,b);
#ifdef SHOWEVOLUTION

#if 0
		/* �olution de vol */
		bubble_pause(1);
		switchRunqueues(&rqs[0][0], &b->entity);
		bubble_pause(1);
		switchRunqueues(&rqs[2][0], &b->entity);
		bubble_pause(1);
		switchRunqueues(&rqs[2][3], &b->entity);
		bubble_pause(1);
		switchRunqueuesBegin(&rqs[2][3], &b1->entity);
		switchRunqueuesBegin(&rqs[2][2], &b2->entity);
		switchRunqueuesBegin(&rqs[1][1], &b->entity);
		switchRunqueuesBegin2(&rqs[2][3], &b1->entity);
		switchRunqueuesBegin2(&rqs[2][2], &b2->entity);
		switchRunqueuesBegin2(&rqs[1][1], &b->entity);
		doStepsBegin(step);
			switchRunqueuesStep(&rqs[2][3], &b1->entity, step);
			switchRunqueuesStep(&rqs[2][2], &b2->entity, step);
			switchRunqueuesStep(&rqs[1][1], &b->entity, step);
		doStepsEnd();
		switchRunqueuesEnd(&rqs[2][3], &b1->entity);
		switchRunqueuesEnd(&rqs[2][2], &b2->entity);
		switchRunqueuesEnd(&rqs[1][1], &b->entity);
		bubble_pause(1);

#else
#if 0
		/* �olution d'explosion */
		bubble_pause(1);
		bubbleExplode(b);
		bubble_pause(1);
		switchRunqueues(&rqs[1][1], &b1->entity);
		switchRunqueues(&rqs[1][0], &b2->entity);
		bubble_pause(1);
		bubbleExplodeBegin(b1);
		bubbleExplodeBegin(b2);
		bubbleExplodeBegin2(b1);
		bubbleExplodeBegin2(b2);
		doStepsBegin(step)
			bubbleExplodeStep(b1,step);
			bubbleExplodeStep(b2,step);
		doStepsEnd();
		bubbleExplodeEnd(b2);
		bubbleExplodeEnd(b1);
		bubble_pause(1);
		switchRunqueuesBegin(&rqs[2][1], &t4->entity);
		switchRunqueuesBegin(&rqs[2][3], &t2->entity);
		switchRunqueuesBegin2(&rqs[2][1], &t4->entity);
		switchRunqueuesBegin2(&rqs[2][3], &t2->entity);
		doStepsBegin(step)
			switchRunqueuesStep(&rqs[2][1], &t4->entity, step);
			switchRunqueuesStep(&rqs[2][3], &t2->entity, step);
		doStepsEnd();
		switchRunqueuesEnd(&rqs[2][3], &t2->entity);
		switchRunqueuesEnd(&rqs[2][1], &t4->entity);
		switchRunqueuesBegin(&rqs[2][0], &t3->entity);
		switchRunqueuesBegin(&rqs[2][2], &t1->entity);
		switchRunqueuesBegin2(&rqs[2][0], &t3->entity);
		switchRunqueuesBegin2(&rqs[2][2], &t1->entity);
		doStepsBegin(step)
			switchRunqueuesStep(&rqs[2][2], &t1->entity, step);
			switchRunqueuesStep(&rqs[2][0], &t3->entity, step);
		doStepsEnd();
		switchRunqueuesEnd(&rqs[2][2], &t1->entity);
		switchRunqueuesEnd(&rqs[2][0], &t3->entity);
#else
	/* A handful shadow runqueue */
		rq_t *norq2, *norq3;
		setRqs(&norq2,1,90,500,MOVIEX,4*CURVE);

		/* exemples de manipulations */
		bubble_pause(1);
		switchRunqueues(&rqs[0][0], &b->entity);
		bubble_pause(1);
		switchRunqueues(&rqs[2][0], &b->entity);
		bubble_pause(1);

		rqs[2][0].entity.thick = 2*RQTHICK;
		updateEntity(&rqs[2][0].entity);
		bubble_pause(1);

		switchRunqueues(norq2, &b->entity);
		bubble_pause(1);
		switchRunqueues(&rqs[2][0], &b2->entity);
		bubble_pause(1);

		rqs[2][0].entity.thick = RQTHICK;
		updateEntity(&rqs[2][0].entity);

		bubble_pause(1);

		rqs[0][0].entity.thick = 2*RQTHICK;
		updateEntity(&rqs[0][0].entity);
		bubble_pause(1);

		switchRunqueues(&rqs[0][0], &b->entity);
		bubble_pause(1);
		setRqs(&norq3,1,90,170,MOVIEX,4*CURVE);
		switchRunqueues(norq3, &b1->entity);
		bubble_pause(1);

		rqs[0][0].entity.thick = RQTHICK;
		updateEntity(&rqs[0][0].entity);

		bubble_pause(1);

		rqs[1][1].entity.thick = 2*RQTHICK;
		updateEntity(&rqs[0][0].entity);
		switchRunqueues(&rqs[1][1], &b1->entity);
		rqs[1][1].entity.thick = RQTHICK;
		updateEntity(&rqs[0][0].entity);

#endif
#endif

#endif


#else /* SHOWPRIO */

		/* d�onstration des priorit� */
#define N 5
		bubble_t *b[N];
		thread_t *t[2*N], *comm;
		//bigb = newBubble(5, norq);
#ifdef SHOWBUILD
		//showEntity(&bigb->entity);
#endif
		comm = newThread(3, norq);
		//bubbleInsertThread(bigb,comm);
		
		switchRunqueues(&rqs[0][0],&comm->entity);
		showEntity(&comm->entity);

		for (i=0;i<sizeof(b)/sizeof(*b);i++) {
			b[i] = newBubble(1, norq);
			t[2*i] = newThread(2, norq);
			t[2*i+1] = newThread(2, norq);
#ifdef SHOWBUILD
			showEntity(&t[2*i]->entity);
			showEntity(&t[2*i+1]->entity);
#endif
			bubbleInsertThread(b[i],t[2*i]);
			bubbleInsertThread(b[i],t[2*i+1]);
			//bubbleInsertBubble(bigb,b[i]);
			switchRunqueues(&rqs[0][0],&b[i]->entity);
			showEntity(&b[i]->entity);
		}
#ifndef SHOWBUILD
		//showEntity(&bigb->entity);
#endif
		//switchRunqueues(&rqs[0][0],&bigb->entity);
		//bubbleExplode(bigb);
		bubbleExplode(b[0]);
		switchRunqueues(&rqs[1][1],&t[1]->entity);
		switchRunqueues(&rqs[1][0],&t[0]->entity);
		bubble_pause(1);
		b[0]->exploded = 0;
		bubbleInsertThread(b[0],t[0]);
		bubbleInsertThread(b[0],t[1]);
		switchRunqueues(&rqs[0][0],&b[0]->entity);
		bubble_pause(1);
#endif /* SHOWPRIO */
	}
	SWFMovie_save(movie,"bulles.swf");
}

	return 0;
}
