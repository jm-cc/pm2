/* Copyright (C) 2002 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@redhat.com>, 2002.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

#include "pthread-abi.h"

#include <error.h>
#include <stdio.h>
#include <stdlib.h>


static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;


static void *
tf (void *p)
{
  int err;

  err = pthread_mutex_lock (&mut);
  if (err != 0)
    error (EXIT_FAILURE, err, "child: cannot get mutex");

  puts ("child: got mutex; signalling");

  pthread_cond_signal (&cond);

  puts ("child: unlock");

  err = pthread_mutex_unlock (&mut);
  if (err != 0)
    error (EXIT_FAILURE, err, "child: cannot unlock");

  puts ("child: done");

  return NULL;
}


static int
do_test (void)
{
  pthread_t th;
  int err;

  //  printf ("&cond = %p\n&mut = %p\n", &cond, &mut);

  puts ("parent: get mutex");

  err = pthread_mutex_lock (&mut);
  if (err != 0)
    error (EXIT_FAILURE, err, "parent: cannot get mutex");

  puts ("parent: create child");

  err = pthread_create (&th, NULL, tf, NULL);
  if (err != 0)
    error (EXIT_FAILURE, err, "parent: cannot create thread");

  puts ("parent: wait for condition");

  err = pthread_cond_wait (&cond, &mut);
  if (err != 0)
    error (EXIT_FAILURE, err, "parent: cannot wait fir signal");

  puts ("parent: got signal");

  err = pthread_join (th, NULL);
  if (err != 0)
    error (EXIT_FAILURE, err, "parent: failed to join");

  puts ("done");

  return 0;
}


int
main (int argc, char *argv[])
{
  assert_running_marcel ();

  return do_test();
}
