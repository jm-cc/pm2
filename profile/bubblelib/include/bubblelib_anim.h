
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

#ifndef BUBBLELIB_ANIM_H
#define BUBBLELIB_ANIM_H

#include "bubblelib_output.h"
#include "tbx_fast_list.h"

/*******************************************************************************
 * Parameters
 */

extern int MOVIEX, MOVIEY;
extern int DISPPRIO;
extern int DISPNAME;
extern int showEmptyBubbles;

extern float thick;
extern float CURVE;
extern float OPTIME; /* Operation time */
#define DELAYTIME (OPTIME/6.) /* FXT delay between switches */
#define BUBBLETHICK thick
#define THREADTHICK thick
#define RQTHICK thick

/* choose between tree and bubble representation */
#undef BUBBLES

#if 1
#define TREES
#else
#define BUBBLES
#endif

/*******************************************************************************
 * Color
 */
typedef struct bl_color {
  unsigned char r;
  unsigned char g;
  unsigned char b;
} bl_color_t;

/*******************************************************************************
 * Entity
 */

/* Hack to avoid enumerator redeclaration errors in bubblegum. */
#ifndef BUBBLELIB_PRIV_ENTITIES_TYPE
enum entity_type {
	BUBBLE,
	THREAD,
	RUNQUEUE,
};
#else
enum entity_type {
    BUBBLELIB_ENTITIES_TYPE_UNUSED
};
#endif

typedef struct entity_s {
	float x, y;		/* position */
	float lastx, lasty;	/* position just before move */
	float width, height;	/* size */
	float lastwidth, lastheight;	/* size just before move */
	unsigned thick;		/* drawing thickness */
	int prio;		/* scheduling priority */
	struct tbx_fast_list_head rq;	/* position in rq */
	struct tbx_fast_list_head entity_list;	/* position in bubble */
	enum entity_type type;		/* type of entity */
	BubbleDisplayItem lastitem;	/* last item shown in movie */
	BubbleShape lastshape;		/* last shape used in movie */
	struct entity_s *holder, *lastholder;	/* current holder, holder just before move */
	struct bubble_s *bubble_holder;	/* holding bubble */
	int leaving_holder;	/* are we leaving our holder (special case for animations) */
	int nospace;		/* in the case of explosion, space used by an entity might actually be already reserved by the holding bubble */
	int id;			/* ID of the entity */
	int gasp;		/* there's a problem with this entity */
	uint64_t ptr;		/* Marcel ptr */
} entity_t;

/*******************************************************************************
 * Runqueue
 */

typedef struct {
	entity_t entity;
	struct tbx_fast_list_head entities;	/* held entities */
	float nextX;			/* position of next enqueued entity */
} rq_t;

extern rq_t *norq;
void setRqs(rq_t **rqs, int nb, float x, float y, float width, float height);
void addToRunqueue(rq_t *rq, entity_t *e);
static inline rq_t * rq_of_entity(entity_t *e) {
	return tbx_container_of(e,rq_t,entity);
}

/*******************************************************************************
 * Bubble
 */

typedef struct bubble_s {
	entity_t entity;
	struct tbx_fast_list_head heldentities;	/* held entities */
	float nextX;			/* position of next enqueued entity */
	int exploded;			/* whether it is exploded */
	BubbleMorph morph;			/* current bubble morph */
	int morphRecurse;		/* an animation can involve a bubble several times, count that */
	entity_t *insertion;		/* entity being inserted */
        bl_color_t color;               /* current bubble color */
} bubble_t;
bubble_t *newBubble (uint64_t ptr, int prio, rq_t *initrq);
void delBubble(bubble_t *b);
static inline bubble_t * bubble_of_entity(entity_t *e) {
	return tbx_container_of(e,bubble_t,entity);
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
	int number;
	rq_t *initrq;
} thread_t;
thread_t *newThread (uint64_t ptr, int prio, rq_t *initrq);
void delThread(thread_t *t);
static inline thread_t * thread_of_entity(entity_t *e) {
	return tbx_container_of(e,thread_t,entity);
}

/*******************************************************************************
 * Functions
 */
void showEntity(entity_t *e);
void updateEntity(entity_t *e);

void bubbleInsertEntity(bubble_t *b, entity_t *e);
void bubbleInsertBubble(bubble_t *bubble, bubble_t *little_bubble);
#define bubbleInsertBubble(b,lb) bubbleInsertEntity(b,&lb->entity)
void bubbleInsertThread(thread_t *bubble, thread_t *thread);
#define bubbleInsertThread(b,t) bubbleInsertEntity(b,&t->entity)

void bubbleRemoveEntity(bubble_t *b, entity_t *e);
void bubbleRemoveBubble(bubble_t *bubble, bubble_t *little_bubble);
#define bubbleRemoveBubble(b,lb) bubbleRemoveEntity(b,&lb->entity)
void bubbleRemoveThread(thread_t *bubble, thread_t *thread);
#define bubbleRemoveThread(b,t) bubbleRemoveEntity(b,&t->entity)

void bubbleExplode(bubble_t *b);

void switchRunqueues(rq_t *rq2, entity_t *e);
void switchBubble(bubble_t *rq2, entity_t *e);

/* The one SWF movie.  */
extern BubbleMovie movie;

#endif /* BUBBLELIB_ANIM_H */
