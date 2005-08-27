/* lockfile - Handle locking and unlocking of stream.
   Copyright (C) 1996, 1998, 2000 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public License as
   published by the Free Software Foundation; either version 2.1 of the
   License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

/* On récupère lpt_mutexattr_settype */
#define _GNU_SOURCE
#include <features.h>


#include "marcel.h" //VD:
#ifdef MA__PTHREAD_FUNCTIONS //VD:
//VD:#include <bits/libc-lock.h>
#include <stdio.h>
#include <pthread.h>
//VD:#include "internals.h"

#define SHARED
#ifdef USE_IN_LIBIO
//VD#include "../libio/libioP.h"
extern void _IO_list_lock __P ((void)); //VD
extern void _IO_list_unlock __P ((void)); //VD
extern void _IO_list_resetlock __P ((void)); //VD
typedef struct _IO_FILE *_IO_ITER; //VD
extern _IO_ITER _IO_iter_begin __P ((void));//VD
extern _IO_ITER _IO_iter_end __P ((void));//VD
extern _IO_ITER _IO_iter_next __P ((_IO_ITER));//VD
extern _IO_FILE *_IO_iter_file __P ((_IO_ITER));//VD
#endif

#ifndef SHARED
/* We need a hook to force this file to be linked in when static
   libpthread is used.  */
const int __pmarcel_provide_lockfile = 0;
#endif


void
__flockfile (FILE *stream)
{
#ifdef USE_IN_LIBIO
  lpt_mutex_lock (stream->_lock);
#else
#endif
}
#ifdef USE_IN_LIBIO
#undef _IO_flockfile
strong_alias (__flockfile, _IO_flockfile)
#endif
weak_alias (__flockfile, flockfile);


void
__funlockfile (FILE *stream)
{
#ifdef USE_IN_LIBIO
  lpt_mutex_unlock (stream->_lock);
#else
#endif
}
#ifdef USE_IN_LIBIO
#undef _IO_funlockfile
strong_alias (__funlockfile, _IO_funlockfile)
#endif
weak_alias (__funlockfile, funlockfile);


int
__ftrylockfile (FILE *stream)
{
#ifdef USE_IN_LIBIO
  return lpt_mutex_trylock (stream->_lock);
#else
#endif
}
#ifdef USE_IN_LIBIO
strong_alias (__ftrylockfile, _IO_ftrylockfile)
#endif
weak_alias (__ftrylockfile, ftrylockfile);

void
__flockfilelist(void)
{
#ifdef USE_IN_LIBIO
  _IO_list_lock();
#endif
}

void
__funlockfilelist(void)
{
#ifdef USE_IN_LIBIO
  _IO_list_unlock();
#endif
}

void
__fresetlockfiles (void)
{
#ifdef USE_IN_LIBIO
  _IO_ITER i;

  lpt_mutexattr_t attr;

  lpt_mutexattr_init (&attr);
  lpt_mutexattr_settype (&attr, PTHREAD_MUTEX_RECURSIVE_NP);

  for (i = _IO_iter_begin(); i != _IO_iter_end(); i = _IO_iter_next(i))
    lpt_mutex_init (_IO_iter_file(i)->_lock, &attr);

  lpt_mutexattr_destroy (&attr);

  _IO_list_resetlock();
#endif
}
#endif //VD:
