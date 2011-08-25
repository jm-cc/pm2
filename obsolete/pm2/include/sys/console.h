
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


/*******************************************************/

#ifndef PM2_CONSOLE_C
extern
#endif
BEGIN_LRPC_LIST
        LRPC_CONSOLE_THREADS,
        LRPC_CONSOLE_PING,
	LRPC_CONSOLE_USER,
	LRPC_CONSOLE_MIGRATE,
	LRPC_CONSOLE_FREEZE
END_LRPC_LIST


/*******************************************************/
LRPC_DECL_REQ(LRPC_CONSOLE_MIGRATE, int destination; char *thread_id; marcel_t thread;);
LRPC_DECL_RES(LRPC_CONSOLE_MIGRATE,);


/*******************************************************/
LRPC_DECL_REQ(LRPC_CONSOLE_FREEZE, char *thread_id; marcel_t thread;);
LRPC_DECL_RES(LRPC_CONSOLE_FREEZE,);


/*******************************************************/

typedef char _user_args_tab[1024];

LRPC_DECL_REQ(LRPC_CONSOLE_USER, _user_args_tab tab;);
LRPC_DECL_RES(LRPC_CONSOLE_USER, _user_args_tab tab; int tid;);
 

/*******************************************************/
	
LRPC_DECL_REQ(LRPC_CONSOLE_PING, int version;);
LRPC_DECL_RES(LRPC_CONSOLE_PING, int version; int tid;);
 

/*******************************************************/

LRPC_DECL_RES(LRPC_CONSOLE_THREADS,);
LRPC_DECL_REQ(LRPC_CONSOLE_THREADS,);

/*******************************************************/


void pm2_console_init_rpc(void);

