
#include <stdio.h>
#include <marcel.h>
#include "mailbox.h"

void init_mailbox(mailbox *b)
{
   b->first = NULL;
   marcel_sem_init(&b->mutex, 1);
   marcel_sem_init(&b->sem_get, 0);
}

int nb_of_mails(mailbox *b)
{ int n;

   marcel_sem_P(&b->mutex);
   n = b->sem_get.value;
   marcel_sem_V(&b->mutex);
   return n;
}

void get_mail(mailbox *b, char **msg, int *size, long timeout)
{ mail *p;

   if(timeout == NO_TIME_OUT)
      marcel_sem_P(&b->sem_get);
   else
      marcel_sem_timed_P(&b->sem_get, timeout);
   marcel_sem_P(&b->mutex);
   p = b->first;
   *msg = p->msg;
   *size = p->size;
   b->first = b->first->next;
   tfree(p);
   marcel_sem_V(&b->mutex);
}

void send_mail(mailbox *b, char *msg, int size)
{ mail *m;

   marcel_sem_P(&b->mutex);
   m = (mail *)tcalloc(1, sizeof(mail));
   m->msg = msg;
   m->size = size;
   if(b->first == NULL)
      b->first = b->last = m;
   else {
      b->last->next = m;
      b->last = m;
   }
   marcel_sem_V(&b->sem_get);
   marcel_sem_V(&b->mutex);
}

