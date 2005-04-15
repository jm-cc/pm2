/* Linuxthreads - a simple clone()-based implementation of Posix        */
/* threads for Linux.                                                   */
/* Copyright (C) 1996 Xavier Leroy (Xavier.Leroy@inria.fr)              */
/*                                                                      */
/* This program is free software; you can redistribute it and/or        */
/* modify it under the terms of the GNU Library General Public License  */
/* as published by the Free Software Foundation; either version 2       */
/* of the License, or (at your option) any later version.               */
/*                                                                      */
/* This program is distributed in the hope that it will be useful,      */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of       */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        */
/* GNU Library General Public License for more details.                 */

/* The "atfork" stuff */

#include "marcel.h" //VD: 
#ifdef MA__PTHREAD_FUNCTIONS
#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include "pthread.h"
//VD: #include "internals.h"
//VD: #include <bits/libc-lock.h>

struct handler_list {
  void (*handler)(void);
  struct handler_list * next;
};

static pmarcel_mutex_t pmarcel_atfork_lock = PMARCEL_MUTEX_INITIALIZER;
static struct handler_list * pmarcel_atfork_prepare = NULL;
static struct handler_list * pmarcel_atfork_parent = NULL;
static struct handler_list * pmarcel_atfork_child = NULL;

static void pmarcel_insert_list(struct handler_list ** list,
                                void (*handler)(void),
                                struct handler_list * newlist,
                                int at_end)
{
  if (handler == NULL) return;
  if (at_end) {
    while(*list != NULL) list = &((*list)->next);
  }
  newlist->handler = handler;
  newlist->next = *list;
  *list = newlist;
}

struct handler_list_block {
  struct handler_list prepare, parent, child;
};

int __pmarcel_atfork(void (*prepare)(void),
		     void (*parent)(void),
		     void (*child)(void))
{
  struct handler_list_block * block =
    (struct handler_list_block *) malloc(sizeof(struct handler_list_block));
  if (block == NULL) return ENOMEM;
  pmarcel_mutex_lock(&pmarcel_atfork_lock);
  /* "prepare" handlers are called in LIFO */
  pmarcel_insert_list(&pmarcel_atfork_prepare, prepare, &block->prepare, 0);
  /* "parent" handlers are called in FIFO */
  pmarcel_insert_list(&pmarcel_atfork_parent, parent, &block->parent, 1);
  /* "child" handlers are called in FIFO */
  pmarcel_insert_list(&pmarcel_atfork_child, child, &block->child, 1);
  pmarcel_mutex_unlock(&pmarcel_atfork_lock);
  return 0;
}
strong_alias (__pmarcel_atfork, pmarcel_atfork)
DEF_PTHREAD(int, atfork, (void (*prepare)(void),
			void (*parent)(void),
			void (*child)(void)),
		(prepare, parent, child))
DEF___PTHREAD(int, atfork, (void (*prepare)(void),
			void (*parent)(void),
			void (*child)(void)),
		(prepare, parent, child))

static inline void pmarcel_call_handlers(struct handler_list * list)
{
  for (/*nothing*/; list != NULL; list = list->next) (list->handler)();
}

extern int __libc_fork(void);

pid_t __fork(void)
{
  pid_t pid;

  pmarcel_mutex_lock(&pmarcel_atfork_lock);

  pmarcel_call_handlers(pmarcel_atfork_prepare);
  __pmarcel_once_fork_prepare();
  __flockfilelist();

  pid = __libc_fork();

  if (pid == 0) {
    //VD__pmarcel_reset_main_thread();

    __fresetlockfiles();
    __pmarcel_once_fork_child();
    pmarcel_call_handlers(pmarcel_atfork_child);

    pmarcel_mutex_init(&pmarcel_atfork_lock, NULL);
  } else {
    __funlockfilelist();
    __pmarcel_once_fork_parent();
    pmarcel_call_handlers(pmarcel_atfork_parent);

    pmarcel_mutex_unlock(&pmarcel_atfork_lock);
  }

  return pid;
}

weak_alias (__fork, fork);

pid_t __vfork(void)
{
  return __fork();
}
weak_alias (__vfork, vfork);

#endif /* MA__PTHREAD_FUNCTIONS */
