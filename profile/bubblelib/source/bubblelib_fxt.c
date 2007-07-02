
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

#include <stdint.h>
#include <stdlib.h>
#include <inttypes.h>
#include <search.h>
#include <string.h>
#include "bubblelib_anim.h"
#include "bubblelib_output.h"


/* XXX TO MOVE */
rq_t **rqs = NULL;
#define FUT

#include <fxt/fxt.h>

#include <fxt/fut.h>

struct fxt_code_name fut_code_table [] =
{
#include "fut_print.h"
	{0, NULL }
};

int FxT_showSystem = 0;
int FxT_verbose = 0;
#define verbprintf(fmt,...) do { if (FxT_verbose) printf(fmt, ## __VA_ARGS__); } while(0);

/*******************************************************************************
 * Pointers Stuff
 *
 * Association between application pointers and our own pointers
 */
static void newPtr(uint64_t ptr, void *data) {
	char *buf=malloc(32);
	ENTRY *item = malloc(sizeof(*item)), *found;
	item->key = buf;
	item->data = data;
	snprintf(buf,32,"%"PRIx64,ptr);
	found=hsearch(*item, ENTER);
	if (!found) {
		perror("hsearch");
		fprintf(stderr,"table pleine !\n");
		abort();
	}
	found->data=data;
	//fprintf(stderr,"%p -> %p\n",ptr,data);
}

static void *getPtr (uint64_t ptr) {
	char buf[32];
	ENTRY item = {.key = buf, .data=NULL}, *found;
	snprintf(buf,sizeof(buf),"%"PRIx64,ptr);
	found = hsearch(item, FIND);
	//fprintf(stderr,"%p -> %p\n",ptr,found?found->data:NULL);
	return found ? found->data : NULL;
}
static void delPtr(uint64_t ptr) {
	char buf[32];
	ENTRY item = {.key = buf, .data=NULL}, *found;
	snprintf(buf,sizeof(buf),"%"PRIx64,ptr);
	found = hsearch(item, ENTER);
	if (!found) abort();
	found->data = NULL;
}

static entity_t *getEntity (uint64_t ptr) {
	return (entity_t *) getPtr(ptr);
}

static rq_t *getRunqueue (uint64_t ptr) {
	rq_t *rq = (rq_t *) getPtr(ptr);
	return rq;
}

static bubble_t *newBubblePtr (uint64_t ptr, rq_t *initrq) {
	bubble_t *b = newBubble(0, initrq);
	newPtr(ptr,b);
	return b;
}

static bubble_t *getBubble (uint64_t ptr) {
	bubble_t *b = (bubble_t *) getPtr(ptr);
	if (!b)
		return newBubblePtr(ptr, NULL);
	else
		return b;
}

static thread_t *newThreadPtr (uint64_t ptr, rq_t *initrq) {
	thread_t *t = newThread(0, initrq);
	newPtr(ptr,t);
	return t;
}

static thread_t *getThread (uint64_t ptr) {
	thread_t *t = (thread_t *) getPtr(ptr);
	if (!t)
		return newThreadPtr(ptr, NULL);
	else
		return t;
}

void printfThread(uint64_t ptr, thread_t *t) {
	verbprintf("thread ");
	if (t->name)
		verbprintf("%s ",t->name);
	verbprintf("(%"PRIx64":%p)",ptr,t);
}


/*******************************************************************************
 * FXT: automatically generated from trace
 */
int BubbleFromFxT(BubbleMovie movie, const char *traceFile) {
	fxt_t fut;
	struct fxt_ev ev;
	unsigned nrqlevels = 0, *rqnums = NULL;
	fxt_blockev_t block;
	unsigned keymask = 0;
	int ret, i;

	if (!(fut = fxt_open(traceFile))) {
		perror("fxt_open");
		return -1;
	}
	block = fxt_blockev_enter(fut);
	hcreate(1024);

	while (!(ret = fxt_next_ev(block, FXT_EV_TYPE_64, &ev))) {
		{
			static int num;
			num++;
			if (!(num%1000)) {
				fprintf(stderr,"\r%d",num);
				fflush(stdout);
			}
			if (num > 1<<16) {
				fprintf(stderr,"Phew! Long trace! Stopping at %d events or else the movie will probably be boring\n", num);
				break;
			}
		}
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
			case BUBBLE_SETID: {
				uint64_t bu = ev.ev64.param[0];
				bubble_t *b = getBubble(bu);
				int id = ev.ev64.param[1];
				verbprintf("bubble %p(%p) id %d\n",(void*)(intptr_t)bu,b,id);
				b->entity.id=id;
				break;
			}
			case SCHED_SETPRIO: {
				entity_t *e = getEntity(ev.ev64.param[0]);
				if (e) {
					e->prio = ev.ev64.param[1];
					verbprintf("%s %p(%p) priority set to %"PRIi64"\n", e->type==BUBBLE?"bubble":"thread",(void *)(intptr_t)ev.ev64.param[0],e,ev.ev64.param[1]);
					updateEntity(e);
					BubbleMovie_nextFrame(movie);
				} else
					verbprintf("unknown %p priority set to %"PRIi64"\n", (void *)(intptr_t)ev.ev64.param[0],ev.ev64.param[1]);
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
#endif
			case BUBBLE_SCHED_BUBBLE_GOINGBACK: {
				bubble_t *bb = getBubble(ev.ev64.param[0]);
				bubble_t *b = getBubble(ev.ev64.param[1]);
				verbprintf(" bubble %p(%p) going back in bubble %p(%p)\n", (void *)(intptr_t)ev.ev64.param[0], bb, (void *)(intptr_t)ev.ev64.param[1], b);
				switchBubble(b,&bb->entity);
				break;
			}
			case BUBBLE_SCHED_GOINGBACK: {
				uint64_t t = ev.ev64.param[0];
				thread_t *e = getThread(t);
				bubble_t *b = getBubble(ev.ev64.param[1]);
				printfThread(t,e);
				verbprintf(" going back in bubble %p(%p)\n", (void *)(intptr_t)ev.ev64.param[1], b);
				switchBubble(b,&e->entity);
				break;
			}
			case BUBBLE_SCHED_SWITCHRQ: {
				entity_t *e = getEntity(ev.ev64.param[0]);
				rq_t *rq = getRunqueue(ev.ev64.param[1]);
				if (!e) break;
				verbprintf("%s %p(%p) switching to %p (%p)\n", e->type==BUBBLE?"bubble":"thread",(void *)(intptr_t)ev.ev64.param[0], e, (void *)(intptr_t)ev.ev64.param[1], rq);
				if (FxT_showSystem || e->type!=THREAD || thread_of_entity(e)->number >= 0)
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
				if (FxT_showSystem || e->number>=0) {
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
				if (FxT_showSystem || e->number>=0) {
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
				/* eux peuvent être dans le désordre */
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
				BubbleMovie_startPlaying(movie, 1);
				verbprintf("start playing\n");
				break;
			}
#endif
#ifdef SCHED_STATUS
			case SCHED_STATUS: {
				char str[FXT_MAX_DATA+1];
				memcpy(&str, ev.ev64.raw, FXT_MAX_DATA);
				str[sizeof(str)-1] = 0;
				verbprintf("sched status %s\n",str);
				BubbleMovie_status(movie, str);
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
				case GHOST_THREAD_BIRTH: {
					uint64_t th = ev.ev64.param[0];
					thread_t *t = newThreadPtr(th, norq);
					t->entity.thick = (THREADTHICK*2)/3;
					t->number = 0;
					verbprintf("new ghost ");
					printfThread(th,t);
					verbprintf("\n");
					addToRunqueue(norq, &t->entity);
					showEntity(&t->entity);
					BubbleMovie_nextFrame(movie);
					break;
				}
				case FUT_THREAD_DEATH_CODE: {
					uint64_t th = ev.ev64.param[0];
					thread_t *t = getThread(th);
					printfThread(th,t);
					verbprintf(" death\n");
					if (FxT_showSystem || t->number >= 0)
						delThread(t);
					delPtr(ev.ev64.param[0]);
					// on peut en avoir encore besoin... free(t);
					// penser à free(t->name) aussi
					break;
				}
				case GHOST_THREAD_RUN: {
					uint64_t th = ev.ev64.user.tid;
					thread_t *t = getThread(th);
					uint64_t thnext = ev.ev64.param[0];
					thread_t *tnext = getThread(thnext);
					if (t->entity.type!=THREAD) abort();
					if (tnext->entity.type!=THREAD) abort();
					printfThread(th,t);
					verbprintf("running ");
					printfThread(thnext,tnext);
					verbprintf("\n");
					if (t->state != THREAD_RUNNING) {
						t->state = THREAD_RUNNING;
						updateEntity(&t->entity);
					}
					if (tnext->state != THREAD_RUNNING) {
						tnext->state = THREAD_RUNNING;
						updateEntity(&tnext->entity);
					}
					if (FxT_showSystem || tnext->number>=0 || tnext->number>=0) {
						BubbleMovie_nextFrame(movie);
						delThread(tnext);
					}
					delPtr(ev.ev64.param[0]);
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
					if (FxT_showSystem || t->number >= 0) {
						BubbleMovie_nextFrame(movie);
					}
					break;
				}
				case SET_THREAD_ID: {
					uint64_t th = ev.ev64.param[0];
					thread_t *t = getThread(th);
					int id = ev.ev64.param[1];
					printfThread(th,t);
					verbprintf(" id %d\n", id);
					t->entity.id=id;
					break;
				}
				case SET_THREAD_NUMBER: {
					uint64_t th = ev.ev64.param[0];
					thread_t *t = getThread(th);
					int number = ev.ev64.param[1];
					printfThread(th,t);
					verbprintf(" number %d\n", number);
					t->number=number;
					if (FxT_showSystem || number >= 0) {
						if (t->initrq && !t->entity.holder)
							addToRunqueue(t->initrq, &t->entity);
						showEntity(&t->entity);
						BubbleMovie_nextFrame(movie);
					} else
						updateEntity(&t->entity);
					break;
				}
				case SCHED_TICK: {
					BubbleMovie_nextFrame(movie);
					BubbleMovie_pause(movie, DELAYTIME);
					break;
				}
				case SCHED_THREAD_BLOCKED: {
					uint64_t th = ev.ev64.user.tid;
					thread_t *t = getThread(th);
					if (t->entity.type!=THREAD) abort();
					printfThread(th,t);
					verbprintf(" going to sleep\n");
					if (t->state == THREAD_BLOCKED) break;
					t->state = THREAD_BLOCKED;
					updateEntity(&t->entity);
					if (FxT_showSystem || t->number >= 0) {
						BubbleMovie_nextFrame(movie);
						BubbleMovie_pause(movie, DELAYTIME);
					}
					break;
				}
				case SCHED_THREAD_WAKE: {
					uint64_t th = ev.ev64.user.tid;
					thread_t *t = getThread(th);
					uint64_t thw = ev.ev64.param[0];
					thread_t *tw = getThread(thw);
					if (t->entity.type!=THREAD) abort();
					printfThread(th,t);
					verbprintf(" waking up\n");
					printfThread(thw,tw);
					verbprintf("\n");
					if (t->state == THREAD_SLEEPING) break;
					t->state = THREAD_SLEEPING;
					updateEntity(&t->entity);
					if (FxT_showSystem || t->number >= 0) {
						BubbleMovie_nextFrame(movie);
						BubbleMovie_pause(movie, DELAYTIME);
					}
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
					//if (tprev==tnext) abort();
					if (tprev->entity.type!=THREAD) abort();
					if (tnext->entity.type!=THREAD) abort();
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
					if (FxT_showSystem || tprev->number>=0 || tnext->number>=0) {
						BubbleMovie_nextFrame(movie);
						BubbleMovie_pause(movie, DELAYTIME);
					}
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
		// Sauvegarde réguliére, pour détecter rapidement les problèmes
		SWFMovie_save(movie,(pair^=1)?"autobulles.swf":"autobulles2.swf");
		}
#endif
	}
	switch(ret) {
		case FXT_EV_EOT: break;
		case FXT_EV_ERROR: perror("fxt_next_ev"); return -1;
		case FXT_EV_TYPEERROR: fprintf(stderr,"wrong trace word size\n"); return -1;
	}
	return 0;
}
