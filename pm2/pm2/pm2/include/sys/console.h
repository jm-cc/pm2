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

______________________________________________________________________________
$Log: console.h,v $
Revision 1.3  2000/01/31 15:50:15  oaumage
- ajout du Log CVS


______________________________________________________________________________
*/


/*******************************************************/

BEGIN_LRPC_LIST
        LRPC_CONSOLE_THREADS,
        LRPC_CONSOLE_PING,
	LRPC_CONSOLE_USER,
	LRPC_CONSOLE_MIGRATE,
	LRPC_CONSOLE_FREEZE
END_LRPC_LIST


/*******************************************************/
LRPC_DECL_REQ(LRPC_CONSOLE_MIGRATE, int destination; char *thread_id; long thread;);
LRPC_DECL_RES(LRPC_CONSOLE_MIGRATE,);


/*******************************************************/
LRPC_DECL_REQ(LRPC_CONSOLE_FREEZE, char *thread_id; long thread;);
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

