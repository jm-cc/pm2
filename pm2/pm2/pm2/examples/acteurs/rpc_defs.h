/*
                      PM2 HIGH-PERF/ISOMALLOC
           High Performance Parallel Multithreaded Machine
                           version 3.0

     Gabriel Antoniu, Olivier Aumage, Luc Bouge, Vincent Danjean,
       Christian Perez, Jean-Francois Mehaut, Raymond Namyst

            Laboratoire de l'Informatique du Parallelisme
                        UMR 5668 CNRS-INRIA
                 Ecole Normale Superieure de Lyon

                      External Contributors:
                 Yves Denneulin (LMC - Grenoble),
                 Benoit Planquelle (LIFL - Lille)

                    1998 All Rights Reserved


                             NOTICE

 Permission to use, copy, modify, and distribute this software and
 its documentation for any purpose and without fee is hereby granted
 provided that the above copyright notice appear in all copies and
 that both the copyright notice and this permission notice appear in
 supporting documentation.

 Neither the institutions (Ecole Normale Superieure de Lyon,
 Laboratoire de L'informatique du Parallelisme, Universite des
 Sciences et Technologies de Lille, Laboratoire d'Informatique
 Fondamentale de Lille), nor the Authors make any representations
 about the suitability of this software for any purpose. This
 software is provided ``as is'' without express or implied warranty.
*/

#ifndef LRPC_DEFS_EST_DEF
#define LRPC_DEFS_EST_DEF

#include <pm2.h>
#include "mailbox.h"

typedef struct {
	int tid;
	pointer pid;
	pointer box;
} actor_ref;

BEGIN_LRPC_LIST
	CREATE_ACTOR,
	SEND_MESSAGE,
	DESTROY_ACTOR
END_LRPC_LIST


/* CREATE_ACTOR */

LRPC_DECL_REQ(CREATE_ACTOR,)
LRPC_DECL_RES(CREATE_ACTOR, pointer pid; pointer box;)

EXTERN_LRPC_SERVICE(CREATE_ACTOR);


/* SEND_MESSAGE */

LRPC_DECL_REQ(SEND_MESSAGE, pointer pid; pointer box; char *mess;)
LRPC_DECL_RES(SEND_MESSAGE,)

EXTERN_LRPC_SERVICE(SEND_MESSAGE);


/* DESTROY_ACTOR */

LRPC_DECL_REQ(DESTROY_ACTOR, pointer pid;)
LRPC_DECL_RES(DESTROY_ACTOR,)

EXTERN_LRPC_SERVICE(DESTROY_ACTOR);


/* Fonctions pratiques : */

extern void message_handler(mailbox *box); /* A implanter dans l'application */
extern void body(mailbox *box); /* A inplanter dans l'application */

void create_actor(int site, actor_ref *act);

void send_message(actor_ref *act, char *mess);

void destroy_actor(actor_ref *act);

#endif
