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

#include "rpc_defs.h"

/* CREATE_ACTOR */

PACK_REQ_STUB(CREATE_ACTOR)
END_STUB

UNPACK_REQ_STUB(CREATE_ACTOR)
END_STUB

PACK_RES_STUB(CREATE_ACTOR)
   mad_pack_pointer(MAD_IN_HEADER, &arg->pid, 1);
   mad_pack_pointer(MAD_IN_HEADER, &arg->box, 1);
END_STUB

UNPACK_RES_STUB(CREATE_ACTOR)
   mad_unpack_pointer(MAD_IN_HEADER, &arg->pid, 1);
   mad_unpack_pointer(MAD_IN_HEADER, &arg->box, 1);
END_STUB


void create_actor(int site, actor_ref *act)
{
  LRPC_RES(CREATE_ACTOR) res;

   act->tid = site;
   QUICK_LRPC(site, CREATE_ACTOR, NULL, &res);

   act->pid = res.pid;
   act->box = res.box;
}

BEGIN_SERVICE(CREATE_ACTOR)
   marcel_t pid;
   marcel_attr_t attr;
   mailbox *box;

   marcel_attr_init(&attr);
   marcel_attr_setdetachstate(&attr, TRUE);
   marcel_attr_setuserspace_np(&attr, sizeof(mailbox));
#ifdef GLOBAL_ADDRESSING
  {
    unsigned granted;

    marcel_attr_setstackaddr(&attr, slot_general_alloc(NULL, 64000, &granted, NULL));
    marcel_attr_setstacksize(&attr, granted);
  }
#endif

   marcel_create(&pid, &attr, (marcel_func_t)body, NULL);

   marcel_getuserspace_np(pid, (void **)&box);

   init_mailbox(box);

   marcel_run_np(pid, box);

   to_pointer(pid, &res.pid);
   to_pointer(box, &res.box);
END_SERVICE(CREATE_ACTOR)


/* SEND_MESSAGE */

PACK_REQ_STUB(SEND_MESSAGE)
  int size;

   mad_pack_pointer(MAD_IN_HEADER, &arg->pid, 1);
   mad_pack_pointer(MAD_IN_HEADER, &arg->box, 1);
   size = strlen(arg->mess)+1;
   mad_pack_int(MAD_IN_HEADER, &size, 1);
   mad_pack_str(MAD_IN_HEADER, arg->mess);
END_STUB

UNPACK_REQ_STUB(SEND_MESSAGE)
   int size;

   mad_unpack_pointer(MAD_IN_HEADER, &arg->pid, 1);
   mad_unpack_pointer(MAD_IN_HEADER, &arg->box, 1);
   mad_unpack_int(MAD_IN_HEADER, &size, 1);
   arg->mess = tmalloc(size);
   mad_unpack_str(MAD_IN_HEADER, arg->mess);
END_STUB

PACK_RES_STUB(SEND_MESSAGE)
END_STUB

UNPACK_RES_STUB(SEND_MESSAGE)
END_STUB


void send_message(actor_ref *act, char *mess)
{
  LRPC_REQ(SEND_MESSAGE) req;

   req.pid = act->pid;
   req.box = act->box;
   req.mess = mess;

   QUICK_ASYNC_LRPC(act->tid, SEND_MESSAGE, &req);
}

BEGIN_SERVICE(SEND_MESSAGE)
   marcel_t pid;
   mailbox *box;

   pid = (marcel_t)to_any_t(&req.pid);
   box = (mailbox *)to_any_t(&req.box);

   send_mail(box, req.mess, strlen(req.mess));

   marcel_deviate_np(pid, (handler_func_t)message_handler, box);

END_SERVICE(SEND_MESSAGE)


/* DESTROY_ACTOR */

PACK_REQ_STUB(DESTROY_ACTOR)
   mad_pack_pointer(MAD_IN_HEADER, &arg->pid, 1);
END_STUB

UNPACK_REQ_STUB(DESTROY_ACTOR)
   mad_unpack_pointer(MAD_IN_HEADER, &arg->pid, 1);
END_STUB

PACK_RES_STUB(DESTROY_ACTOR)
END_STUB

UNPACK_RES_STUB(DESTROY_ACTOR)
END_STUB


void destroy_actor(actor_ref *act)
{
  LRPC_REQ(DESTROY_ACTOR) req;

   req.pid = act->pid;
   QUICK_LRPC(act->tid, DESTROY_ACTOR, &req, NULL);
}

BEGIN_SERVICE(DESTROY_ACTOR)
   marcel_t pid;

   pid = (marcel_t)to_any_t(&req.pid);
   marcel_cancel(pid);

END_SERVICE(DESTROY_ACTOR)
