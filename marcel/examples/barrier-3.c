/* Copyright (C) 2004 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@redhat.com>, 2004.

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

/* This is a test for behavior not guaranteed by POSIX.  */
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

#ifndef MARCEL_VERSION
# error "We're not using Marcel's <pthread.h>."
#endif

static pthread_barrier_t b1;
static pthread_barrier_t b2;


#define N 20

static void *
tf (void *arg)
{
  int round = 0;

  while (round++ < 30)
    {
      if (pthread_barrier_wait (&b1) == PTHREAD_BARRIER_SERIAL_THREAD)
	{
	  pthread_barrier_destroy (&b1);
	  if (pthread_barrier_init (&b1, NULL, N) != 0)
	    {
	      fprintf(stderr,"tf: 1st barrier_init failed");
	      pthread_exit((void *)1);
	    }
	}

      if (pthread_barrier_wait (&b2) == PTHREAD_BARRIER_SERIAL_THREAD)
	{
	  pthread_barrier_destroy (&b2);
	  if (pthread_barrier_init (&b2, NULL, N) != 0)
	    {
	      fprintf(stderr,"tf: 2nd barrier_init failed");
	      pthread_exit((void *)1);
	    }
	}
    }

  return NULL;
}


int
main (int argc, char *argv[])
{
  pthread_attr_t at;
  int cnt;

  if (pthread_attr_init (&at) != 0)
    {
      fprintf(stderr,"attr_init failed");
      return 1;
    }

  if (pthread_attr_setstacksize (&at, 1 * 1024 * 1024) != 0)
    {
      fprintf(stderr,"attr_setstacksize failed");
      return 1;
    }

  if (pthread_barrier_init (&b1, NULL, N) != 0)
    {
      fprintf(stderr,"1st barrier_init failed");
      return 1;
    }

  if (pthread_barrier_init (&b2, NULL, N) != 0)
    {
      fprintf(stderr,"2nd barrier_init failed");
      return 1;
    }

  pthread_t th[N - 1];
  for (cnt = 0; cnt < N - 1; ++cnt)
    if (pthread_create (&th[cnt], &at, tf, NULL) != 0)
      {
	fprintf(stderr,"pthread_create failed");
	return 1;
      }

  if (pthread_attr_destroy (&at) != 0)
    {
      fprintf(stderr,"attr_destroy failed");
      return 1;
    }

  tf (NULL);

  for (cnt = 0; cnt < N - 1; ++cnt)
    if (pthread_join (th[cnt], NULL) != 0)
      {
	fprintf(stderr,"pthread_join failed");
	return 1;
      }

  return 0;
}
