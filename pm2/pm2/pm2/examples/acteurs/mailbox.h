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

#ifndef MAILBOX_EST_DEF
#define MAILBOX_EST_DEF

#include <marcel_sem.h>

_PRIVATE_ struct mail_struct {
   char *msg;
   int size;
   struct mail_struct *next;
};

typedef struct mail_struct mail;

_PRIVATE_ struct mailbox_struct {
   mail *first, *last;
   marcel_sem_t mutex, sem_get;
};

typedef struct mailbox_struct mailbox;

void init_mailbox(mailbox *b);
int nb_of_mails(mailbox *b);
void get_mail(mailbox *b, char **msg, int *size, long timeout);
void send_mail(mailbox *b, char *msg, int size);

#endif
