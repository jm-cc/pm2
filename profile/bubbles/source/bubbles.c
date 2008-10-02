
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

#include "bubblelib_output.h"
#include "bubblelib_anim.h"
#include "bubblelib_fxt.h"

/* else build movie by hand */

//#define SHOWBUILD
#ifndef SHOWBUILD
#define SHOWEVOLUTION
#endif
//#define SHOWPRIO


#include <getopt.h>
#include <libgen.h>
#include <stdlib.h>

#ifdef __ia64
#define _SYS_UCONTEXT_H 1
#define _BITS_SIGCONTEXT_H 1
#endif

/* several useful macros */

/*******************************************************************************
 * SWF Stuff
 */
#define bubble_pause(sec) BubbleMovie_pause(movie, sec)


/*******************************************************************************
 * Main
 */

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
	char c;
	int delayed = 0;

	curBubbleOps = &SWFBubbleOps;
	curBubbleOps->init();

	while((c=getopt(argc,argv,":fdvx:y:t:c:o:hpnsPe")) != EOF)
		switch(c) {
		case 'f':
			SWF_fontfile = optarg;
			break;
		case 'd':
			delayed = 1;
			break;
		case 'v':
			FxT_verbose = 1;
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
			FxT_showSystem = 1;
			break;
		case 'P':
			FxT_showPauses = 1;
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

	movie = newBubbleMovie();

	if (delayed)
		BubbleMovie_startPlaying(movie,0);

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
if (optind != argc) {
	if (optind != argc-1) {
		fprintf(stderr,"too many files, only one accepted\n");
		usage(argv[0]);
		exit(1);
	}

	BubbleFromFxT(movie, argv[optind]);

	printf("saving to autobulles.swf\n");
	BubbleMovie_save(movie,"autobulles.swf");
	printf("saved as autobulles.swf\n");
} else {


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
		/* évolution de vol */
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
		/* évolution d'explosion */
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
		bubble_pause(1);
		b[0]->exploded = 0;
		bubbleInsertThread(b[0],t[0]);
		bubbleInsertThread(b[0],t[1]);
		switchRunqueues(&rqs[0][0],&b[0]->entity);
		bubble_pause(1);
#endif /* SHOWPRIO */
	}
	BubbleMovie_save(movie,"bulles.swf");
}
	curBubbleOps->fini();

	return 0;
}
