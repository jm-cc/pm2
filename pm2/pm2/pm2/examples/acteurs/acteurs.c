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

int *les_modules, nb_modules;

void message_handler(mailbox *box)
{ char *mess;
  int size;

   get_mail(box, &mess, &size, NO_TIME_OUT);

   pm2_printf("L'acteur %lx a recu le message : %s\n", marcel_self(), mess);

   tfree(mess); /* on libere le message */
}

void body(mailbox *box)
{
   while(1) {
      marcel_delay(500);
      pm2_printf("je suis un acteur (%lx) qui s'ennuie !\n", marcel_self());
   }
}

int pm2_main(int argc, char **argv)
{
  actor_ref act;

  pm2_init_rpc();

  DECLARE_LRPC(CREATE_ACTOR);
  DECLARE_LRPC(SEND_MESSAGE);
  DECLARE_LRPC(DESTROY_ACTOR);

  pm2_init(&argc, argv, 2, &les_modules, &nb_modules);

  if(pm2_self() == les_modules[0]) { /* master process */

    pm2_printf("Le maitre va creer un acteur...\n");
    create_actor(les_modules[1], &act);

    marcel_delay(2000);

    pm2_printf("Le maitre va envoyer un message...\n");
    send_message(&act, "Salut l'artiste !");

    marcel_delay(2000);

    pm2_printf("Le maitre va detruire l'acteur...\n");
    destroy_actor(&act);

    pm2_kill_modules(les_modules, nb_modules);

  }

  pm2_exit();

  return 0;
}
