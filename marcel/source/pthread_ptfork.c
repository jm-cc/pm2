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

#include "marcel.h"
#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include "pthread.h"
#include <unistd.h>

extern int __register_atfork(void (*prepare)(void),void (*parent)(void),void (*child)(void), void * dso);

DEF_POSIX(int,atfork,(void (*prepare)(void),void (*parent)(void),void (*child)(void)),(prepare,parent,child),
{
        return __register_atfork(prepare, parent, child, NULL); 
})

DEF_PTHREAD(int,atfork,(void (*prepare)(void),void (*parent)(void),void (*child)(void)),(prepare,parent,child));
DEF___PTHREAD(int,atfork,(void (*prepare)(void),void (*parent)(void),void (*child)(void)),(prepare,parent,child));

#ifdef MA__LIBPTHREAD
extern pid_t __libc_fork(void);

pid_t __fork(void)
{
        return __libc_fork();
}

weak_alias (__fork, fork);
weak_alias (__fork, vfork);
#endif
