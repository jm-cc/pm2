
#ifndef LRPC_DEFS_EST_DEF
#define LRPC_DEFS_EST_DEF

#include "pm2.h"

BEGIN_LRPC_LIST
	SAMPLE,
        INFO,
	LRPC_PING,
	LRPC_PING_ASYNC,
	LRPC_PING_QUICK_ASYNC
END_LRPC_LIST


/* SAMPLE */

typedef char sample_tab[64];

LRPC_DECL_REQ(SAMPLE, sample_tab tab;)
LRPC_DECL_RES(SAMPLE,)


/* INFO */

extern unsigned BUF_SIZE;
extern unsigned NB_PING;

LRPC_DECL_REQ(INFO,);
LRPC_DECL_RES(INFO,);

/* PING */

#define MAX_BUF_SIZE    16384

#define BUF_PACKING     MAD_IN_PLACE

LRPC_DECL_REQ(LRPC_PING,)
LRPC_DECL_RES(LRPC_PING,)


/* PING_ASYNC */

LRPC_DECL_REQ(LRPC_PING_ASYNC,)
LRPC_DECL_RES(LRPC_PING_ASYNC,)


#endif
