
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
