
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
