
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

#define TREES
//#define BUBBLES

/* bubbles / threads aspect */

#define CURVE 20.
#define RQ_XMARGIN 20.
#define RQ_YMARGIN 10.

#define RATE 16. /* frame rate */
#define OPTIME 0.5 /* Operation time */
#define DELAYTIME 0.2 /* FXT delay between switches */

#define BUBBLETHICK 4
#define THREADTHICK 4
#define RQTHICK 4

#define BSIZE 30 /* Button size */

/* movie size */

/* X */
#define MOVIEX 1280.

/* Y */
#define MOVIEY 700.



/*******************************************************************************
 * End of configuration
 */

#ifdef TREES
#define OVERLAP CURVE
#endif
#ifdef BUBBLES
#define OVERLAP 0
#endif
#include <ming.h>
#include <math.h>
#include <search.h>

#ifdef FXT
#define FUT

#include <fxt/fxt.h>

#include <fxt/fut.h>
struct fxt_code_name fut_code_table [] =
{
#include "fut_print.h"
	{0, NULL }
};
#endif

#include <stdlib.h>
#define __KERNEL__
#include <linux/list.h>
#undef __KERNEL__
#ifndef min
#define min(a,b) ({ typeof(a) _a = (a); typeof(b) _b = (b); _a<_b?_a:_b; })
#endif
#ifndef max
#define max(a,b) ({ typeof(a) _a = (a); typeof(b) _b = (b); _a>_b?_a:_b; })
#endif

/* several useful macros */

#define tbx_offset_of(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#define tbx_container_of(ptr, type, member) \
        ((type *)((char *)(typeof (&((type *)0)->member))(ptr)- \
                  tbx_offset_of(type,member)))

#define list_for_each_entry_after(pos,head,member,start)		   \
	for (pos = list_entry((start)->member.next, typeof(*pos), member), \
			prefetch(pos->member.next);			   \
		&pos->member != (head);					   \
		pos = list_entry(pos->member.next, typeof(*pos), member),  \
			prefetch(pos->member.next))

/*******************************************************************************
 * SWF Stuff
 */
SWFMovie movie;
SWFAction stop;
SWFFont font;

void pause(float sec) {
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

		SWFMovie_nextFrame(movie);
		SWFDisplayItem_remove(item);
	} else 
		for (i=0; i<sec*RATE;i++) {
			SWFMovie_nextFrame(movie);
		}
}

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
	/* essaie quand même de sauver ce qu'on peut */
	SWFMovie_save(movie,"rescue.swf", -1);
	fprintf(stderr,"written on rescue.swf\n");
	abort();
}

/*******************************************************************************
 * Pointers Stuff
 */
void newPtr(unsigned long ptr, void *data) {
	char *buf=malloc(9);
	ENTRY *item = malloc(sizeof(*item)), *found;
	item->key = buf;
	item->data = data;
	snprintf(buf,9,"%lx",ptr);
	found=hsearch(*item, ENTER);
	if (!found) {
		perror("hsearch");
		fprintf(stderr,"table pleine !\n");
		gasp();
	}
	found->data=data;
	//fprintf(stderr,"%p -> %p\n",ptr,data);
}

void *getPtr (unsigned long ptr) {
	char buf[32];
	ENTRY item = {.key = buf, .data=NULL}, *found;
	snprintf(buf,sizeof(buf),"%lx",ptr);
	found = hsearch(item, FIND);
	//fprintf(stderr,"%p -> %p\n",ptr,found?found->data:NULL);
	return found ? found->data : NULL;
}
void delPtr(unsigned long ptr) {
	char buf[32];
	ENTRY item = {.key = buf, .data=NULL}, *found;
	snprintf(buf,sizeof(buf),"%lx",ptr);
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
	float x, y;
	float lastx, lasty;
	float width, height;
	float lastwidth, lastheight;
	unsigned thick;
	int prio;
	struct list_head rq;
	struct list_head entity_list;
	enum entity_type type;
	SWFDisplayItem lastitem;
	SWFShape lastshape;
	struct entity_s *holder, *lastholder;
	struct bubble_s *bubble_holder;
	int leaving_holder;
	int nospace;
} entity_t;

entity_t *getEntity (unsigned long ptr) {
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
	struct list_head entities;
	float nextX;
} rq_t;

rq_t *norq;

static inline rq_t * rq_of_entity(entity_t *e) {
	return tbx_container_of(e,rq_t,entity);
}

rq_t *getRunqueue (unsigned long ptr) {
	rq_t *rq = (rq_t *) getPtr(ptr);
	return rq;
}

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

void setRunqueue(SWFShape shape, unsigned thick, float width, float height) {
	SWFShape_setLine(shape,thick,0,0,0,255);
	SWFShape_movePenTo(shape,0,height-RQ_YMARGIN);
	SWFShape_drawLineTo(shape,width,height-RQ_YMARGIN);
}

void addToRunqueue(rq_t *rq, entity_t *e);

/*******************************************************************************
 * Bubble
 */

typedef struct bubble_s {
	entity_t entity;
	struct list_head heldentities;
	float nextX;
	int exploded;
	SWFMorph morph;
	int morphRecurse;
	entity_t *insertion;
} bubble_t;

static inline bubble_t * bubble_of_entity(entity_t *e) {
	return tbx_container_of(e,bubble_t,entity);
}

bubble_t *newBubble (int prio, rq_t *initrq) {
	bubble_t *b = malloc(sizeof(*b));
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
	b->entity.thick = BUBBLETHICK;
	b->entity.lastx = -1;
	b->entity.prio = prio;
	INIT_LIST_HEAD(&b->heldentities);
	b->entity.type = BUBBLE;
	b->entity.lastitem = NULL;
	b->morph = NULL;
	b->morphRecurse = 0;
	b->entity.holder = NULL;
	b->entity.bubble_holder = NULL;
	b->entity.leaving_holder = 0;
	b->entity.nospace = 0;
	b->exploded = 0;
	b->insertion = NULL;
	if (initrq)
		addToRunqueue(initrq, &b->entity);
	return b;
}

bubble_t *newBubblePtr (unsigned long ptr, rq_t *initrq) {
	bubble_t *b = newBubble(0, initrq);
	newPtr(ptr,b);
	return b;
}

bubble_t *getBubble (unsigned long ptr) {
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
} state_t;
typedef struct {
	entity_t entity;
	state_t state;
} thread_t;

static inline thread_t * thread_of_entity(entity_t *e) {
	return tbx_container_of(e,thread_t,entity);
}

void setThread(SWFShape shape, unsigned thick, float width, float height, int prio, state_t state) {
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
	}
	SWFShape_movePen(shape,    xStep,0);
	SWFShape_drawCurve(shape, 2*xStep,2*yStep,-xStep,yStep);
	SWFShape_drawCurve(shape,-2*xStep,2*yStep, xStep,yStep);
	SWFShape_drawCurve(shape, 2*xStep,2*yStep,-xStep,yStep);
	if (prio) {
		SWFShape_movePen(shape,xStep,yStep);
		SWFShape_drawSizedGlyph(shape,font,'0'+prio,CURVE);
	}
}

thread_t *newThread (int prio, rq_t *initrq) {
	thread_t *t = malloc(sizeof(*t));
	t->entity.width = CURVE;
	t->entity.height = CURVE*3;
	t->entity.thick = THREADTHICK;
	t->entity.lastx = -1;
	t->entity.prio = prio;
	t->entity.type = THREAD;
	t->entity.lastitem = NULL;
	t->entity.holder = NULL;
	t->entity.bubble_holder = NULL;
	t->entity.leaving_holder = 0;
	t->entity.nospace = 0;
	if (initrq)
		addToRunqueue(initrq, &t->entity);
	return t;
}

thread_t *newThreadPtr (unsigned long ptr, rq_t *initrq) {
	thread_t *t = newThread(0, initrq);
	newPtr(ptr,t);
	return t;
}

thread_t *getThread (unsigned long ptr) {
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
	if (prio) {
		SWFShape_movePen(shape,5*width/2+10-CURVE,5*height/2+10);
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
	setThread(shape,t->entity.thick,t->entity.width,t->entity.height,t->entity.prio,t->state);
}

float nextX;


#endif /* BUBBLES */
#ifdef TREES
void setBubbleRecur(SWFShape shape, bubble_t *b) {
	entity_t *e;
	SWFShape_setLine(shape,b->entity.thick,0,0,0,255);
	SWFShape_movePenTo(shape,b->entity.x+CURVE/2,b->entity.y);
	SWFShape_drawCircle(shape,CURVE/2);
	if (b->entity.prio) {
		SWFShape_movePen(shape,CURVE,CURVE/2);
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
	setThread(shape, t->entity.thick, t->entity.width, t->entity.height, t->entity.prio, t->state);
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
	if (e->lastitem)
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
	float i,j;
	removeFromHolderBegin(&t->entity);
	removeFromHolderBegin2(&t->entity);
	for (i=0.;i<=OPTIME*RATE;i++)
		j = -cos((i*M_PI)/(OPTIME*RATE))/2.+0.5;
			removeFromHolderStep(&t->entity, j);
	removeFromHolderEnd(&t->entity);
	free(t);
}

/*******************************************************************************
 * Bubble morph
 */

int recurse;
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
	if (b->entity.lastitem) {
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
	list_add_tail(&e->rq,&rq->entities);
	e->holder = &rq->entity;
}

void addToRunqueueAtEnd(rq_t *rq, entity_t *e, entity_t *after) {
	entityMoveEnd(e);
	list_add(&e->rq,&after->rq);
}

void addToRunqueueStep(rq_t *rq, entity_t *e, float step) {
	entityMoveStep(e,step);
}

void addToRunqueue(rq_t *rq, entity_t *e) {
	addToRunqueueBegin(rq,e);
	addToRunqueueBegin2(rq,e);
	if (shown(e)) {
		float i,j;
		for (i=0.;i<=OPTIME*RATE;i++) {
			j = -cos((i*M_PI)/(OPTIME*RATE))/2.+0.5;
			addToRunqueueStep(rq, e, j);
			SWFMovie_nextFrame(movie);
		}
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
	float i,j;
	switchRunqueuesBegin(rq2,e);
	switchRunqueuesBegin2(rq2,e);
	if (shown(e))
		for (i=0.;i<=OPTIME*RATE;i++) {
			j = -cos((i*M_PI)/(OPTIME*RATE))/2.+0.5;
			switchRunqueuesStep(rq2,e,j);
			SWFMovie_nextFrame(movie);
		}
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
	if (b->nextX-OVERLAP>b->entity.width || b->entity.y != b->entity.lasty) {
		growInHolderBegin(&b->entity,b->nextX-OVERLAP-b->entity.width,mydy);
		b->entity.width = b->nextX-OVERLAP;
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
	//list_del(&e->entity_list);
	//e->bubble_holder = NULL;
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
		list_add_tail(&e->entity_list,&b->heldentities);
#endif
		bubbleMorphBegin2(b);
		removeFromHolderBegin2(e);
		entityMoveBegin2(e);
		if (dx)
			growInHolderBegin2(&b->entity);
	}

	if (shown(&b->entity)) {
		float i,j;
		for (i=0.;i<=OPTIME*RATE;i++) {
			j = -cos((i*M_PI)/(OPTIME*RATE))/2.+0.5;
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
			SWFMovie_nextFrame(movie);
		}
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
		list_add_tail(&e->entity_list,&b->heldentities);
#endif
		e->bubble_holder = b;
		e->holder = &b->entity;
	}
#ifdef TREES
	if (e->lastitem) {
		SWFDisplayItem_remove(e->lastitem);
		e->lastitem = NULL;
	}
#endif
}

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
		float i,j;
		for (i=0.;i<=OPTIME*RATE;i++) {
			j = -cos((i*M_PI)/(OPTIME*RATE))/2.+0.5;
			bubbleExplodeStep(b,j);
			SWFMovie_nextFrame(movie);
		}
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

#define doStepsBegin(j) do {\
	float i,j; \
	for (i=0.;i<=OPTIME*RATE;i++) { \
		j = -cos((i*M_PI)/(OPTIME*RATE))/2.+0.5;
#define doStepsEnd() \
		SWFMovie_nextFrame(movie); \
		} \
	} while(0);


void bubbleInsertBubble(bubble_t *bubble, bubble_t *little_bubble);
#define bubbleInsertBubble(b,lb) bubbleInsertEntity(b,&lb->entity)
void bubbleInsertThread(thread_t *bubble, thread_t *thread);
#define bubbleInsertThread(b,t) bubbleInsertEntity(b,&t->entity)

/*******************************************************************************
 * Main
 */

bubble_t *bigb;

int main(int argc, char *argv[]) {
	rq_t **rqs=NULL;
	/* choose whatever font you want */
	FILE *f;

#ifdef FXT
	if (argc<=1) {
		fprintf(stderr,"I need a profile trace file name as argument\n");
		exit(1);
	}
#endif
	
	Ming_init();
	int __attribute__((unused)) i;

	f = fopen("/usr/share/libming/fonts/Timmons.fdb","r");

	/* pause macro */
	stop = compileSWFActionCode(" if (!stopped) { stop(); stopped=1; } else { play(); stopped=0; }");

	font = (SWFFont) loadSWFFontFromFile(f);

	/* movie */
	movie = newSWFMovie();
	//SWFMovie_setBackground(movie,0,0,0);
	SWFMovie_setRate(movie,RATE);
	SWFMovie_setDimension(movie,MOVIEX,MOVIEY);

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

	setRqs(&norq,1,0,0,MOVIEX,
#ifdef FXT
			120
#else
#ifdef BUBBLES
			150
#endif
#ifdef TREES
			2*CURVE
#endif
#endif
			);

/*******************************************************************************
 * FXT: automatically generated from trace
 */
#ifdef FXT
	fxt_t fut;
	struct fxt_ev ev;
	unsigned rqlevel,rqnum;
	fxt_blockev_t block;
	unsigned keymask = 0;

	if (!(fut = fxt_open(argv[1]))) {
		perror("fxt_open");
		exit(1);
	}
	block = fxt_blockev_enter(fut);
	hcreate(100);

	while (!fxt_next_ev(block, FXT_EV_TYPE_NATIVE, &ev)) {
		if (ev.native.code == FUT_KEYCHANGE_CODE)
			keymask = ev.native.param[0];
		switch (ev.native.code) {
			case BUBBLE_SCHED_NEW: {
				bubble_t *b = newBubblePtr(ev.native.param[0], norq);
				printf("new bubble %p -> %p\n", (void *)ev.native.param[0], b);
				showEntity(&b->entity);
				break;
			}
			case BUBBLE_SCHED_SETPRIO: {
				bubble_t *b = getBubble(ev.native.param[0]);
				b->entity.prio = ev.native.param[1];
				printf("bubble %p(%p) priority set to %lx\n", (void *)ev.native.param[0],b,ev.native.param[1]);
				break;
			}
#ifdef BUBBLE_SCHED_CLOSED
			case BUBBLE_SCHED_CLOSED: {
				bubble_t *b = getBubble(ev.native.param[0]);
				printf("bubble %p(%p) closed\n", (void *)ev.native.param[0],b);
				break;
			}
#endif
#ifdef BUBBLE_SCHED_CLOSING
			case BUBBLE_SCHED_CLOSING: {
				bubble_t *b = getBubble(ev.native.param[0]);
				printf("bubble %p(%p) closing\n", (void *)ev.native.param[0],b);
				break;
			}
#endif
			case BUBBLE_SCHED_SWITCHRQ: {
				entity_t *e = getEntity(ev.native.param[0]);
				rq_t *rq = getRunqueue(ev.native.param[1]);
				printf("entity %p(%p) switching to %p (%p)\n", (void *)ev.native.param[0], e, (void *)ev.native.param[1], rq);
				switchRunqueues(rq, e);
				break;
			}
#ifdef BUBBLE_SCHED_EXPLODE
			case BUBBLE_SCHED_EXPLODE: {
				bubble_t *b = getBubble(ev.native.param[0]);
				printf("bubble %p(%p) exploding on rq %p\n", (void *)ev.native.param[0],b,b->entity.holder);
				bubbleExplode(b);
				break;
			}
#endif
#ifdef BUBBLE_SCHED_GOINGBACK
			case BUBBLE_SCHED_GOINGBACK: {
				thread_t *e = getThread(ev.native.param[0]);
				bubble_t *b = getBubble(ev.native.param[1]);
				printf("thread %p(%p) going back in bubble %p(%p)\n", (void *)ev.native.param[0], e, (void *)ev.native.param[1], b);
				bubbleInsertThread(b,e);
				break;
			}
#endif
			case BUBBLE_SCHED_INSERT_BUBBLE: {
				bubble_t *e = getBubble(ev.native.param[0]);
				bubble_t *b = getBubble(ev.native.param[1]);
				printf("bubble %p(%p) inserted in bubble %p(%p)\n", (void *)ev.native.param[0], e, (void *)ev.native.param[1], b);
				bubbleInsertBubble(b,e);
				break;
			}
			case BUBBLE_SCHED_INSERT_THREAD: {
				thread_t *e = getThread(ev.native.param[0]);
				bubble_t *b = getBubble(ev.native.param[1]);
				printf("thread %p(%p) inserted in bubble %p(%p)\n", (void *)ev.native.param[0], e, (void *)ev.native.param[1], b);
				bubbleInsertThread(b,e);
				break;
			}
			case BUBBLE_SCHED_WAKE: {
				bubble_t *b = getBubble(ev.native.param[0]);
				rq_t *rq = getRunqueue(ev.native.param[1]);
				printf("bubble %p(%p) waking up on runqueue %p(%p)\n", (void *)ev.native.param[0], b, (void *)ev.native.param[1], rq);
				switchRunqueues(rq, &b->entity);
				break;
			}
			case FUT_RQS_NEWLEVEL: {
				rqlevel = ev.native.param[0];
				rqnum = 0;
				rqs = realloc(rqs,(rqlevel+1)*sizeof(*rqs));
				setRqs(&rqs[rqlevel],ev.native.param[1],0,rqlevel*150+100,MOVIEX,150);
				for (i=0;i<ev.native.param[1];i++)
				  showEntity(&rqs[rqlevel][i].entity);
				printf("new runqueue level %u arity %lu\n", rqlevel, ev.native.param[1]);
				break;
			}
			case FUT_RQS_NEWLWPRQ: {
				/* eux peuvent être dans le désordre */
				newPtr(ev.native.param[1],&rqs[rqlevel][ev.native.param[0]]);
				printf("new lwp runqueue %lu at %p\n",ev.native.param[0],(void *)ev.native.param[1]);
				break;
			}
			case FUT_RQS_NEWRQ: {
				newPtr(ev.native.param[0],&rqs[rqlevel][rqnum]);
				printf("new runqueue %d.%d at %p -> %p\n",rqlevel,rqnum,(void *)ev.native.param[0],&rqs[rqlevel][rqnum]);
				rqnum++;
				break;
			}
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
				n = (int)ev.native.param[0];
				break;
			}
#endif
#ifdef SCHED_IDLE_STOP
			case SCHED_IDLE_STOP: {
				int n;
				n = (int)ev.native.param[0];
				break;
			}
#endif
			default:
			if (keymask) switch (ev.native.code) {
				case FUT_THREAD_BIRTH_CODE: {
					thread_t *t = newThreadPtr(ev.native.param[0], norq);
					printf("new thread %p(%p)\n", (void *)ev.native.param[0], t);
					showEntity(&t->entity);
					break;
				}
				case FUT_THREAD_DEATH_CODE: {
					thread_t *t = getThread(ev.native.param[0]);
					printf("thread death %p(%p)\n", (void *)ev.native.param[0], t);
					hideEntity(&t->entity);
					delPtr(ev.native.param[0]);
					delThread(t);
					break;
				}
				case FUT_SET_THREAD_NAME_CODE:
					/* TODO */
					break;
				case SCHED_THREAD_BLOCKED: {
					thread_t *t = getThread(ev.native.user.tid);
					if (t->entity.type!=THREAD) gasp();
					printf("thread %p(%p) going to sleep\n",(void*)ev.native.user.tid,t);
					t->state = THREAD_BLOCKED;
					updateEntity(&t->entity);
					pause(DELAYTIME);
					break;
				}
				case SCHED_THREAD_WAKE: {
					thread_t *t = getThread(ev.native.param[0]);
					if (t->entity.type!=THREAD) gasp();
					printf("thread %p(%p) waking up\n",(void*)ev.native.user.tid,t);
					t->state = THREAD_SLEEPING;
					updateEntity(&t->entity);
					pause(DELAYTIME);
					break;
				}
				case FUT_SWITCH_TO_CODE: {
					thread_t *tprev = getThread(ev.native.user.tid);
					thread_t *tnext = getThread(ev.native.param[0]);
					if (tprev==tnext) gasp();
					if (tprev->entity.type!=THREAD) gasp();
					if (tnext->entity.type!=THREAD) gasp();
					printf("switch from thread %p(%p) to thread %p(%p)\n",(void *)ev.native.user.tid,tprev,(void *)ev.native.param[0],tnext);
					if (tprev->state == THREAD_RUNNING)
						tprev->state = THREAD_SLEEPING;
					tnext->state = THREAD_RUNNING;
					updateEntity(&tprev->entity);
					updateEntity(&tnext->entity);
					pause(DELAYTIME);
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
#if 0
					printf("%16llu %p %010lx %1u" ,ev.native.time ,ev.native.user.tid ,ev.native.code ,ev.native.nb_params);
					for (i=0;i<ev.native.nb_params;i++)
						printf(" %010lx", ev.native.param[i]);
					printf("\n");
#endif
					break;
			}
		}

		fflush(stdout);
	}
	SWFMovie_save(movie,"autobulles.swf", -1);
	exit(0);
#else

/*******************************************************************************
 * Manual build generated from trace
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
		/* démonstration bulles simples */
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
		pause(1);
#endif /* SHOWBUILD */


		b1 = newBubble(0, norq);

#ifdef SHOWBUILD
		showEntity(&b1->entity);
		pause(1);
#endif /* SHOWBUILD */
		b2 = newBubble(0, norq);
#ifdef SHOWBUILD
		showEntity(&b2->entity);
		pause(1);
#endif /* SHOWBUILD */

		bubbleInsertThread(b1,t1);
		bubbleInsertThread(b1,t2);

#ifdef SHOWBUILD
		pause(1);
#endif /* SHOWBUILD */
		bubbleInsertThread(b2,t3);
		bubbleInsertThread(b2,t4);
#ifdef SHOWBUILD
		pause(1);
#endif
		b = newBubble(0, norq);
#ifdef SHOWBUILD
		showEntity(&b->entity);
		pause(1);
#endif
		bubbleInsertBubble(b,b2);
		bubbleInsertBubble(b,b1);
		//bubbleInsertThread(b,t5);

#ifndef SHOWBUILD
		showEntity(&b->entity);
#endif
		fprintf(stderr,"b1=%p, b2=%p, b=%p\n",b1,b2,b);
#ifdef SHOWEVOLUTION

#if 1
		/* Évolution de vol */
		pause(1);
		switchRunqueues(&rqs[0][0], &b->entity);
		pause(1);
		switchRunqueues(&rqs[2][0], &b->entity);
		pause(1);
		switchRunqueues(&rqs[2][3], &b->entity);
		pause(1);
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
		pause(1);

		/* Évolution d'explosion */
#else
		pause(1);
		bubbleExplode(b);
		pause(1);
		switchRunqueues(&rqs[1][1], &b1->entity);
		switchRunqueues(&rqs[1][0], &b2->entity);
		pause(1);
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
		pause(1);
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
			switchRunqueuesStep(&rqs[2][0], &t1->entity, step);
			switchRunqueuesStep(&rqs[2][2], &t3->entity, step);
		doStepsEnd();
		switchRunqueuesEnd(&rqs[2][2], &t1->entity);
		switchRunqueuesEnd(&rqs[2][0], &t3->entity);
#endif

#endif


#else /* SHOWPRIO */

		/* démonstration des priorités */
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
		pause(1);
		b[0]->exploded = 0;
		bubbleInsertThread(b[0],t[0]);
		bubbleInsertThread(b[0],t[1]);
		switchRunqueues(&rqs[0][0],&b[0]->entity);
		pause(1);
#endif /* SHOWPRIO */
	}
	SWFMovie_save(movie,"bulles.swf", -1);

	return 0;
#endif /* FXT */
}
