
/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 "the PM2 team" (see AUTHORS file)
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

#ifndef LRPC_DEFS_EST_DEF
#define LRPC_DEFS_EST_DEF

#include <pm2.h>
#include "mailbox.h"

typedef struct {
	int tid;
	pointer pid;
	pointer box;
} actor_ref;

#ifndef PM2_RPC_DEFS_C
extern
#endif
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
