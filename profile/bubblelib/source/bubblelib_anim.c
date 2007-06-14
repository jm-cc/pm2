
/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2007 "the PM2 team" (see AUTHORS file)
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

#include <math.h>
#include "bubblelib_anim.h"
#include "bubblelib_output.h"

/* XXX TO MOVE */
extern BubbleMovie movie;

/*******************************************************************************
 * Configuration
 */

/* bubbles / threads aspect */

#define RQ_XMARGIN 20.
#define RQ_YMARGIN 10.

int DISPPRIO = 0;
int DISPNAME = 0;
int showEmptyBubbles = 1;

float thick = 4.;
float CURVE = 20.;
float OPTIME = 1; /* Operation time */

BubbleOps *curBubbleOps;

/*******************************************************************************
 * End of configuration
 */

extern void bad_type_in_gasp(void);
#define highlight(x) { \
	entity_t *_x = (entity_t*) (x); \
	_x->gasp = 1; \
	updateEntity(_x); \
}

#define gasp(msg,...) do { \
	/* essaie quand même de sauver ce qu'on peut */ \
	BubbleMovie_status(movie, msg); \
	fprintf(stderr,msg "\n",##__VA_ARGS__); \
	BubbleMovie_nextFrame(movie); \
	BubbleMovie_abort(movie); \
} while(0)

#define gasp1(x, msg, ...) do { \
	highlight(x); \
	gasp(msg, ##__VA_ARGS__); \
} while(0)

#define gasp2(x, y, msg, ...) do { \
	highlight(x); \
	gasp1(y, msg, ##__VA_ARGS__); \
} while(0)

#define gasp3(x, y, z, msg, ...) do { \
	highlight(x); \
	gasp2(y, z, msg, ##__VA_ARGS__); \
} while(0)

/*******************************************************************************
 * The principle of animation is to show some entities (showEntity()), and then
 * perform animations, which consist of
 * - calling fooBegin() functions,
 * - modifying entities parameters, if needed,
 * - calling fooBegin2() functions (for computing the difference between initial
 *   parameters and current parameters),
 * - for step between 0.0 and 1.0, calling fooStep() functions and calling BubbleMovie_nextFrame(movie); doStepsBegin() and doStepsEnd() are helper macros for this,
 * - calling with fooEnd() functions.
 *
 * See the second part of main() for examples
 */

#define doStepsBegin(j) do {\
	float i,j; \
	for (i=0.;i<=OPTIME*RATE;i++) { \
		j = -cos((i*M_PI)/(OPTIME*RATE))/2.+0.5;
#define doStepsEnd() \
		BubbleMovie_nextFrame(movie); \
		} \
	} while(0);


#ifdef TREES
#define OVERLAP CURVE
#endif
#ifdef BUBBLES
#define OVERLAP 0
#endif

/*******************************************************************************
 * Entity
 */
static void setEntityRecur(BubbleShape shape, entity_t *e);
static void growInHolderBegin(entity_t *e, float dx, float dy);
static void growInHolderBegin2(entity_t *e);
static void growInHolderStep(entity_t *e, float step);
static void growInHolderEnd(entity_t *e);
static void removeFromHolderBegin(entity_t *e);
static void removeFromHolderBegin2(entity_t *e);
static void removeFromHolderStep(entity_t *e, float step);
static void removeFromHolderEnd(entity_t *e);
static void addToBubbleBegin(bubble_t *b, entity_t *e);
static void addToBubbleBegin2(bubble_t *b, entity_t *e);
static void addToBubbleStep(bubble_t *b, entity_t *e, float step);
static void addToBubbleEnd(bubble_t *b, entity_t *e);

/*******************************************************************************
 * Runqueue
 */

rq_t *norq;

static void switchRunqueuesBegin(rq_t *rq2, entity_t *e);
static void switchRunqueuesBegin2(rq_t *rq2, entity_t *e);
static void switchRunqueuesStep(rq_t *rq2, entity_t *e, float step);
static void switchRunqueuesEnd(rq_t *rq2, entity_t *e);

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
		(*rqs)[i].entity.bubble_holder = NULL;
		(*rqs)[i].entity.gasp = 0;
		INIT_LIST_HEAD(&(*rqs)[i].entities);
		(*rqs)[i].nextX = (*rqs)[i].entity.x + RQ_XMARGIN;
	}
}

/* draw a runqueue */
static void setRunqueue(BubbleShape shape, rq_t *rq) {
	float width = rq->entity.width;
	float height = rq->entity.height;
	BubbleShape_setLine(shape,rq->entity.thick,rq->entity.gasp?255:0,0,255,255);
	BubbleShape_movePenTo(shape,0,height-RQ_YMARGIN);
	BubbleShape_drawLineTo(shape,width,height-RQ_YMARGIN);
}

/*******************************************************************************
 * Bubble
 */

static void bubbleMorphStep(bubble_t *b, float ratio);

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
	b->entity.id = -1;
	b->entity.gasp = 0;
	INIT_LIST_HEAD(&b->heldentities);
	b->exploded = 0;
	b->morph = NULL;
	b->morphRecurse = 0;
	b->insertion = NULL;
	if (initrq)
		addToRunqueue(initrq, &b->entity);
	return b;
}

/*******************************************************************************
 * Thread
 */

static void setThread(BubbleShape shape, thread_t *t) {
	unsigned thick = t->entity.thick;
	float width = t->entity.width;
	float height = t->entity.height;
	int prio = t->entity.prio;
	state_t state = t->state;
	char *name = t->name;
	float xStep = width/3, yStep = height/9;

	switch (state) {
	case THREAD_BLOCKED:
		BubbleShape_setLine(shape,thick,0,0,0,255);
		break;
	case THREAD_SLEEPING:
		BubbleShape_setLine(shape,thick,0,255,0,255);
		break;
	case THREAD_RUNNING:
		BubbleShape_setLine(shape,thick,255,0,0,255);
		break;
	case THREAD_DEAD:
		BubbleShape_setLine(shape,thick,200,200,200,255);
		break;
	}
	if (t->entity.gasp)
		BubbleShape_setLine(shape,thick,255,0,255,255);
	BubbleShape_movePen(shape,    xStep,0);
	BubbleShape_drawCurve(shape, 2*xStep,2*yStep,-xStep,yStep);
	BubbleShape_drawCurve(shape,-2*xStep,2*yStep, xStep,yStep);
	BubbleShape_drawCurve(shape, 2*xStep,2*yStep,-xStep,yStep);
	BubbleShape_setLine(shape,2,0,0,0,255);
	if (DISPPRIO && prio) {
		BubbleShape_movePen(shape,xStep,CURVE);
		BubbleShape_drawSizedGlyph(shape,'0'+prio,CURVE);
	}
	if (DISPNAME && name) {
		BubbleShape_movePen(shape,0,CURVE);
		while (*name) {
			BubbleShape_drawSizedGlyph(shape,*name,CURVE);
			BubbleShape_movePen(shape,CURVE,0);
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
	t->entity.id = -1;
	t->entity.gasp = 0;
	t->state = THREAD_BLOCKED;
	t->name = NULL;
	t->number = -1;
	t->initrq = initrq;
	return t;
}

#ifdef BUBBLES
static void setBubble(BubbleShape shape, bubble_t *b) {
	float width = b->width;
	float height = b->height;
	int prio = b->prio;
	if (b->gasp)
		BubbleShape_setLine(shape,b->thick,255,0,255,255);
	BubbleShape_movePen(shape,CURVE,0);
	BubbleShape_drawLine(shape,width-2*CURVE,0);
	BubbleShape_drawCurve(shape,CURVE,0,0,CURVE);
	BubbleShape_drawLine(shape,0,height-2*CURVE);
	BubbleShape_drawCurve(shape,0,CURVE,-CURVE,0);
	BubbleShape_drawLine(shape,-width+2*CURVE,0);
	BubbleShape_drawCurve(shape,-CURVE,0,0,-CURVE);
	BubbleShape_drawLine(shape,0,-height+2*CURVE);
	BubbleShape_drawCurve(shape,0,-CURVE,CURVE,0);
	if (DISPPRIO && prio) {
		BubbleShape_movePen(shape,-2*CURVE,-CURVE);
		BubbleShape_drawSizedGlyph(shape,'0'+prio,CURVE);
	}
}

static void setPlainBubble(BubbleShape shape, bubble_t *b) {
	BubbleShape_setLine(shape,b->thick,0,0,0,255);
	setBubble(shape,b);
}

static void setExplodedBubble(BubbleShape shape, bubble_t *b) {
	BubbleShape_setLine(shape,b->thick,0,0,0,128);
	setBubble(shape,b);
}

static void setBubbleRecur(BubbleShape shape, bubble_t *b) {
	if (BubbleSetBubble)
		BubbleSetBubble(shape, b->entity.id, b->entity.x, b->entity.y, b->entity.width, b->entity.height);
	if (b->exploded)
		setExplodedBubble(shape,b);
	else
		setPlainBubble(shape,b);
	BubbleDisplayItem_moveTo(b->entity.lastitem,b->entity.x,b->entity.y);
}
static void setThreadRecur(BubbleShape shape, thread_t *t) {
	if (BubbleSetThread)
		BubbleSetThread(shape, t->entity.id, t->entity.x, t->entity.y, t->entity.width, t->entity.height);
	setThread(shape,t);
}


#endif /* BUBBLES */
#ifdef TREES
/* in the case of trees, only the root bubble gets drawn, and should draw its
 * children */
static void setBubbleRecur(BubbleShape shape, bubble_t *b) {
	entity_t *e;
	if (!showEmptyBubbles && list_empty(&b->heldentities))
		return;
	BubbleShape_setLine(shape,b->entity.thick,0,0,0,255);
	BubbleShape_movePenTo(shape,b->entity.x+CURVE/2,b->entity.y);
	BubbleShape_drawCircle(shape,CURVE/2);
	if (DISPPRIO && b->entity.prio) {
		BubbleShape_movePenTo(shape,b->entity.x-CURVE,b->entity.y-CURVE);
		BubbleShape_drawSizedGlyph(shape,'0'+b->entity.prio,CURVE);
	}
	list_for_each_entry(e,&b->heldentities,entity_list) {
		BubbleShape_setLine(shape,b->entity.thick,0,0,0,255);
		BubbleShape_movePenTo(shape,b->entity.x+CURVE/2,b->entity.y);
		BubbleShape_drawLineTo(shape,e->x+CURVE/2,e->y);
		BubbleShape_movePen(shape,-CURVE/2,0);
		setEntityRecur(shape,e);
	}
	if (b->insertion) {
		/* fake entity */
		BubbleShape_setLine(shape,b->entity.thick,0,0,0,255);
		BubbleShape_movePenTo(shape,b->entity.x+CURVE/2,b->entity.y);
		BubbleShape_drawLineTo(shape,b->insertion->x+CURVE/2,b->insertion->y);
		BubbleShape_movePen(shape,-CURVE/2,0);
		setEntityRecur(shape,b->insertion);
	}
}
static void setThreadRecur(BubbleShape shape, thread_t *t) {
	setThread(shape, t);
}
static void setEntityRecur(BubbleShape shape, entity_t *e) {
	switch(e->type) {
		case THREAD: return setThreadRecur(shape,thread_of_entity(e));
		case BUBBLE: return setBubbleRecur(shape,bubble_of_entity(e));
		case RUNQUEUE: return setRunqueue(shape,rq_of_entity(e));
	}
}
#endif

static void doEntity(entity_t *e) {
	BubbleShape shape = e->lastshape = newBubbleShape();
	switch(e->type) {
		case BUBBLE: {
			bubble_t *b = bubble_of_entity(e);
#ifndef TREES
			entity_t *el;
#endif

			e->lastitem = BubbleMovie_add(movie,(BubbleBlock)shape);
			setBubbleRecur(shape,b);
#ifndef TREES
			list_for_each_entry(el,&b->heldentities,entity_list)
				showEntity(el);
#endif
#ifdef BUBBLES
			BubbleDisplayItem_moveTo(e->lastitem,e->x,e->y);
#endif
		}
		break;
		case THREAD: {
			thread_t *t = thread_of_entity(e);
			setThreadRecur(shape,t);
			e->lastitem = BubbleMovie_add(movie,(BubbleBlock)shape);
			BubbleDisplayItem_moveTo(e->lastitem,e->x,e->y);
		}
		break;
		case RUNQUEUE: {
			rq_t *rq = rq_of_entity(e);
			setRunqueue(shape, rq);
			e->lastitem = BubbleMovie_add(movie,(BubbleBlock)shape);
			BubbleDisplayItem_moveTo(e->lastitem,e->x,e->y);
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
	BubbleDisplayItem_remove(e->lastitem);
	doEntity(e);
}

static void hideEntity(entity_t *e) {
	if (!e->lastitem) gasp1(e, "hiding already hidden entity");
	BubbleDisplayItem_remove(e->lastitem);
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
	BubbleMovie_pause(movie, DELAYTIME);
	list_del(&t->entity.rq);
	t->entity.holder = NULL;
	if (t->entity.bubble_holder) {
		list_del(&t->entity.entity_list);
		updateEntity(&t->entity.bubble_holder->entity);
		t->entity.bubble_holder = NULL;
	} else {
		hideEntity(&t->entity);
	}
	// XXX: berk. ça marche pour l'instant car on est le dernier sur la liste
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
	if (b->entity.lastitem) {
		BubbleDisplayItem_remove(b->entity.lastitem);
		b->entity.lastitem = NULL;
	}
	// XXX: berk. ça marche pour l'instant car on est le dernier sur la liste
	norq->nextX -= b->entity.width+RQ_XMARGIN;
	// TODO: animation
}

/*******************************************************************************
 * Bubble morph
 */

static void bubbleMorphBegin(bubble_t *b) {
	if (b->entity.lastitem) {
		b->morphRecurse++;
		if (!b->morph) {
			BubbleShape shape;
			BubbleDisplayItem_remove(b->entity.lastitem);
			b->morph = newBubbleMorphShape();
			b->entity.lastitem = BubbleMovie_add(movie,(BubbleBlock)b->morph);
			shape = BubbleMorph_getShape1(b->morph);
			setBubbleRecur(shape,b);
		}
	}
#ifdef TREES
	else
		if (b->entity.bubble_holder)
			bubbleMorphBegin(b->entity.bubble_holder);
#endif
}

static void bubbleMorphBegin2(bubble_t *b) {
	if (b->morphRecurse) {
		if (!(--b->morphRecurse))
			setBubbleRecur(BubbleMorph_getShape2(b->morph),b);
	}
#ifdef TREES
	else
		if (b->entity.bubble_holder)
			bubbleMorphBegin2(b->entity.bubble_holder);
#endif
}

static int shown(entity_t *e) {
	return (!!e->lastitem)
#ifdef TREES
		|| (e->bubble_holder && shown(&e->bubble_holder->entity))
#endif
		;
}

static void bubbleMorphStep(bubble_t *b, float ratio) {
	if (b->entity.lastitem)
		BubbleDisplayItem_setRatio(b->entity.lastitem,ratio);
#ifdef TREES
	else
		if (b->entity.bubble_holder)
			bubbleMorphStep(b->entity.bubble_holder,ratio);
#endif
}

static void bubbleMorphEnd(bubble_t *b) {
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

static void entityMoveDeltaBegin(entity_t *e, float dx, float dy) {
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

static void entityMoveToBegin(entity_t *e, float x, float y) {
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

static void entityMoveBegin2(entity_t *e) {
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

static void entityMoveStep(entity_t *e, float step) {
	if (e->lastitem)
#ifdef TREES
	if (e->type == THREAD)
#endif
		BubbleDisplayItem_moveTo(e->lastitem,
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

static void entityMoveEnd(entity_t *e) {
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

static void addToRunqueueBegin(rq_t *rq, entity_t *e) {
	entityMoveToBegin(e, rq->nextX, rq->entity.y+rq->entity.height-e->height);
	rq->nextX += e->width+RQ_XMARGIN;
}
static void addToRunqueueBegin2(rq_t *rq, entity_t *e) {
	entityMoveBegin2(e);
}

#ifdef BUBBLES
static void addToRunqueueAtBegin(rq_t *rq, entity_t *e, float x) {
	entityMoveToBegin(e, x, rq->entity.y+rq->entity.height-e->height);
}

static void addToRunqueueAtBegin2(rq_t *rq, entity_t *e) {
	entityMoveBegin2(e);
}
#endif

static void addToRunqueueEnd(rq_t *rq, entity_t *e) {
	entityMoveEnd(e);
	if (e->holder) gasp2(rq, e, "adding non-free entity to runqueue");
	list_add_tail(&e->rq,&rq->entities);
	e->holder = &rq->entity;
}

#ifdef BUBBLES
static void addToRunqueueAtEnd(rq_t *rq, entity_t *e, entity_t *after) {
	entityMoveEnd(e);
	if (e->holder) gasp(rq, e, "adding non-free entity to runqueue");
	list_add(&e->rq,&after->rq);
	e->holder = &rq->entity;
}
#endif

static void addToRunqueueStep(rq_t *rq, entity_t *e, float step) {
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

static void removeFromRunqueueBegin(rq_t *rq, entity_t *e) {
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

static void removeFromRunqueueBegin2(rq_t *rq, entity_t *e) {
	entity_t *el;
	if (!e->nospace) {
		list_for_each_entry_after(el,&rq->entities,rq,e)
			entityMoveBegin2(el);
	}
}

static void removeFromRunqueueEnd(rq_t *rq, entity_t *e) {
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

static void switchRunqueuesBegin(rq_t *rq2, entity_t *e) {
	removeFromHolderBegin(e);
	addToRunqueueBegin(rq2, e);
}

static void switchRunqueuesBegin2(rq_t *rq2, entity_t *e) {
	removeFromHolderBegin2(e);
	addToRunqueueBegin2(rq2, e);
}

static void switchRunqueuesStep(rq_t *rq2, entity_t *e, float step) {
	removeFromHolderStep(e,step);
	addToRunqueueStep(rq2, e, step);
}

static void switchRunqueuesEnd(rq_t *rq2, entity_t *e) {
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

static void switchBubbleBegin(bubble_t *b2, entity_t *e) {
	removeFromHolderBegin(e);
	addToBubbleBegin(b2, e);
}

static void switchBubbleBegin2(bubble_t *b2, entity_t *e) {
	removeFromHolderBegin2(e);
	addToBubbleBegin2(b2, e);
}

static void switchBubbleStep(bubble_t *b2, entity_t *e, float step) {
	removeFromHolderStep(e,step);
	addToBubbleStep(b2, e, step);
}

static void switchBubbleEnd(bubble_t *b2, entity_t *e) {
	removeFromHolderEnd(e);
	addToBubbleEnd(b2, e);
	e->nospace = 0;
}

void switchBubble(bubble_t *b2, entity_t *e) {
	switchBubbleBegin(b2,e);
	switchBubbleBegin2(b2,e);
	if (shown(e))
		doStepsBegin(j)
			switchBubbleStep(b2,e,j);
		doStepsEnd();
	switchBubbleEnd(b2,e);
}

static void growInRunqueueBegin(rq_t *rq, entity_t *e, float dx, float dy) {
	entity_t *el;

	list_for_each_entry_after(el,&rq->entities,rq,e)
		if (!el->leaving_holder)
			entityMoveDeltaBegin(el, dx, 0);
	rq->nextX += dx;
}

static void growInRunqueueBegin2(rq_t *rq, entity_t *e) {
	entity_t *el;

	list_for_each_entry_after(el,&rq->entities,rq,e)
		if (!el->leaving_holder)
			entityMoveBegin2(el);
}

static void changeInRunqueueStep(rq_t *rq, entity_t *e, float step) {
	if (!e->nospace) {
		entity_t *el;
		list_for_each_entry_after(el,&rq->entities,rq,e)
			if (!el->leaving_holder)
				entityMoveStep(el,step);
	}
}

static void changeInRunqueueEnd(rq_t *rq, entity_t *e) {
	entity_t *el;

	list_for_each_entry_after(el,&rq->entities,rq,e)
		if (!el->leaving_holder)
			entityMoveEnd(el);
}

/*******************************************************************************
 * Bubble grow
 */

static void growInBubbleBegin(bubble_t *b, entity_t *e, float dx, float dy) {
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

static void growInBubbleBegin2(bubble_t *b, entity_t *e) {
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

static void growInBubbleStep(bubble_t *b, entity_t *e, float step) {
	entity_t *el;

	bubbleMorphStep(b, step);

#ifdef BUBBLES
	BubbleDisplayItem_moveTo(b->entity.lastitem,
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

static void growInBubbleEnd(bubble_t *b, entity_t *e) {
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

static void removeFromBubbleBegin(bubble_t *b, entity_t *e) {
	e->lastholder = &b->entity;
	e->leaving_holder = 1;
	if (!e->nospace)
		growInBubbleBegin(b,e,-e->width-CURVE,0);
}

static void removeFromBubbleBegin2(bubble_t *b, entity_t *e) {
	if (!e->nospace)
		growInBubbleBegin2(b,e);
}

static void removeFromBubbleStep(bubble_t *b, entity_t *e, float step) {
	if (!e->nospace)
		growInBubbleStep(b,e,step);
}

static void removeFromBubbleEnd(bubble_t *b, entity_t *e) {
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

static void addToBubbleBegin(bubble_t *b, entity_t *e) {
	float dx = 0;
	float dy = 0;

#ifdef BUBBLES
#warning not implemented yet
	gasp("not implemented yet");
#endif

	bubbleMorphBegin(b);

	b->entity.lastx = b->entity.x;
	b->entity.lasty = b->entity.y;

	entityMoveToBegin(e, b->entity.x+b->nextX, b->entity.y+
			OVERLAP+CURVE);

	b->nextX += e->width+CURVE;

	b->entity.lastwidth = b->entity.width;
	if (b->nextX-OVERLAP>b->entity.width) {
		growInHolderBegin(&b->entity,dx=(b->nextX-OVERLAP-b->entity.width),dy);
		b->entity.width = b->nextX-OVERLAP;
	}

#ifdef TREES
	if (e->bubble_holder != b) gasp2(b,e,"adding entity to bubble that doesn't hold it");
#endif
}

static void addToBubbleBegin2(bubble_t *b, entity_t *e) {
	float dx = 0;
	if (b->nextX-OVERLAP>b->entity.lastwidth) 
		dx = b->nextX-OVERLAP-b->entity.lastwidth;
	bubbleMorphBegin2(b);
	entityMoveBegin2(e);
	if (dx)
		growInHolderBegin2(&b->entity);
}

static void addToBubbleStep(bubble_t *b, entity_t *e, float step) {
	float dx = 0;
	if (b->nextX-OVERLAP>b->entity.lastwidth) 
		dx = b->nextX-OVERLAP-b->entity.lastwidth;
	bubbleMorphStep(b,step);
	entityMoveStep(e,step);
	if (dx) growInHolderStep(&b->entity,step);
}

static void addToBubbleEnd(bubble_t *b, entity_t *e) {
	float dx = 0;
	if (b->nextX-OVERLAP>b->entity.lastwidth) 
		dx = b->nextX-OVERLAP-b->entity.lastwidth;
	entityMoveEnd(e);
	if (dx)
		growInHolderEnd(&b->entity);
	bubbleMorphEnd(b);
	/* Put back at end of list */
	list_del(&e->entity_list);
	list_add_tail(&e->entity_list,&b->heldentities);
	e->holder = &b->entity;
#ifdef TREES
	if (e->lastitem) {
		BubbleDisplayItem_remove(e->lastitem);
		e->lastitem = NULL;
	}
#endif
}

void bubbleInsertEntity(bubble_t *b, entity_t *e) {
	float dx = 0;
	float dy = 0;
#ifdef BUBBLES
	rq_t *rq=NULL;

	if (b->exploded) {
		if (b->entity.holder->type != RUNQUEUE) {
			gasp2(b, e, "while inserting, bubble exploded on something else than runqueue");
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
		if (e->bubble_holder) gasp3(e, e->bubble_holder, b, "inserting entity that is already inserted in a bubble");
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
				BubbleDisplayItem_moveTo(b->entity.lastitem,
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
		if (e->bubble_holder) gasp3(e, e->bubble_holder, b, "inserting entity that is already inserted in a bubble");
		list_add_tail(&e->entity_list,&b->heldentities);
		e->bubble_holder = b;
#endif
		e->holder = &b->entity;
	}
#ifdef TREES
	if (e->lastitem) {
		BubbleDisplayItem_remove(e->lastitem);
		e->lastitem = NULL;
	}
#endif
}

/*******************************************************************************
 * Bubble remove
 */

void bubbleRemoveEntity(bubble_t *b, entity_t *e) {
#ifndef TREES
	gasp("not useful");
#endif
	switchRunqueues(norq,e);
	list_del(&e->entity_list);
	if (e->bubble_holder != b) gasp3(e, e->bubble_holder, b, "removing entity from a bubble that isn't its holder");
	e->bubble_holder = NULL;
	updateEntity(&b->entity);
	showEntity(e);
	BubbleMovie_nextFrame(movie);
}

/*******************************************************************************
 * Bubble explode
 */

#ifdef BUBBLES
static void bubbleExplodeBegin(bubble_t *b) {
	entity_t *el;
	rq_t *rq;

	if (b->entity.holder->type != RUNQUEUE)
		gasp1(b, "bubble exploding on something else than runqueue");
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
static void bubbleExplodeBegin2(bubble_t *b) {
	entity_t *el;
	rq_t *rq;

	b->exploded = 1;
	bubbleMorphBegin2(b);
	list_for_each_entry(el,&b->heldentities,entity_list)
		addToRunqueueAtBegin2(rq,el);
}

static void bubbleExplodeStep(bubble_t *b, float step) {
	entity_t *el;
	rq_t *rq;

	if (b->entity.holder->type != RUNQUEUE)
		gasp1(b,"bubble exploding on something else than runqueue\n");

	rq = rq_of_entity(b->entity.holder);
	bubbleMorphStep(b,step);
	BubbleDisplayItem_moveTo(b->entity.lastitem,
			b->entity.lastx+(b->entity.x-b->entity.lastx)*step,
			b->entity.lasty+(b->entity.y-b->entity.lasty)*step
			);
	list_for_each_entry(el,&b->heldentities,entity_list)
		entityMoveStep(el,step);
	//changeInRunqueueStep(rq,&b->entity,step);
}

static void bubbleExplodeEnd(bubble_t *b) {
	entity_t *el;
	rq_t *rq;

	if (b->entity.holder->type != RUNQUEUE)
		gasp1(b,"bubble exploding on something else than runqueue\n");

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

static void removeFromHolderBegin(entity_t *e) {
	if (!e->holder) return;
	switch (e->holder->type) {
	case RUNQUEUE:
		removeFromRunqueueBegin(rq_of_entity(e->holder),e);
		break;
	case BUBBLE:
		removeFromBubbleBegin(bubble_of_entity(e->holder),e);
		break;
	default:
		gasp1(e,"uuh, %p's holder of type %d ?!",e,e->holder->type);
	}
}

static void removeFromHolderBegin2(entity_t *e) {
	if (!e->holder) return;
	switch (e->holder->type) {
	case RUNQUEUE:
		removeFromRunqueueBegin2(rq_of_entity(e->holder),e);
		break;
	case BUBBLE:
		removeFromBubbleBegin2(bubble_of_entity(e->holder),e);
		break;
	default:
		gasp1(e,"uuh, %p's holder of type %d ?!",e,e->holder->type);
	}
}

static void removeFromHolderStep(entity_t *e, float step) {
	if (!e->holder) return;
	switch (e->lastholder->type) {
	case RUNQUEUE:
		changeInRunqueueStep(rq_of_entity(e->lastholder),e,step);
		break;
	case BUBBLE:
		removeFromBubbleStep(bubble_of_entity(e->holder),e,step);
		break;
	default:
		gasp1(e,"uuh, %p's holder of type %d ?!",e,e->holder->type);
	}
}

static void removeFromHolderEnd(entity_t *e) {
	if (!e->holder) return;
	switch (e->holder->type) {
	case RUNQUEUE:
		removeFromRunqueueEnd(rq_of_entity(e->holder),e);
		break;
	case BUBBLE:
		removeFromBubbleEnd(bubble_of_entity(e->holder),e);
		break;
	default:
		gasp1(e,"uuh, %p's holder of type %d ?!",e,e->holder->type);
	}
}

static void growInHolderBegin(entity_t *e, float dx, float dy) {
	switch (e->holder->type) {
	case RUNQUEUE:
		growInRunqueueBegin(rq_of_entity(e->holder),e,dx,dy);
		break;
	case BUBBLE:
		growInBubbleBegin(bubble_of_entity(e->holder), e, dx, dy);
		break;
	default:
		gasp1(e,"uuh, %p's holder of type %d ?!",e,e->holder->type);
	}
}

static void growInHolderBegin2(entity_t *e) {
	switch (e->holder->type) {
	case RUNQUEUE:
		growInRunqueueBegin2(rq_of_entity(e->holder),e);
		break;
	case BUBBLE:
		growInBubbleBegin2(bubble_of_entity(e->holder), e);
		break;
	default:
		gasp1(e,"uuh, %p's holder of type %d ?!",e,e->holder->type);
	}
}

static void growInHolderStep(entity_t *e, float step) {
	switch (e->holder->type) {
	case RUNQUEUE:
		changeInRunqueueStep(rq_of_entity(e->holder),e,step);
		break;
	case BUBBLE:
		growInBubbleStep(bubble_of_entity(e->holder), e, step);
		break;
	default:
		gasp1(e,"uuh, %p's holder of type %d ?!",e,e->holder->type);
	}
}

static void growInHolderEnd(entity_t *e) {
	switch (e->holder->type) {
	case RUNQUEUE:
		changeInRunqueueEnd(rq_of_entity(e->holder),e);
		break;
	case BUBBLE:
		growInBubbleEnd(bubble_of_entity(e->holder), e);
		break;
	default:
		gasp1(e,"uuh, %p's holder of type %d ?!",e,e->holder->type);
	}
}

