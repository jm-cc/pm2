
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
