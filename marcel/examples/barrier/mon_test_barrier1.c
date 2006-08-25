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

/* Test of POSIX barriers.  */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define NBTHREADS 10
pthread_t thr[NBTHREADS];

pthread_barrier_t barrier;
#define COUNT 5

void * dosleep(void *arg)
{
  fprintf(stderr,"thread %lx dans dosleep pour %d\n",pthread_self(),(int)arg);
  sleep((int)arg);
  pthread_barrier_wait(&barrier);
  fprintf(stderr,"reveil du thread %lx\n",pthread_self());
  
  pthread_exit(0);
}

int main(void)
{
  int i;
  void *retour;
  pthread_barrier_init(&barrier,NULL,COUNT);

  fprintf(stderr,"count = %d\n",COUNT);

  for (i=1;i<=NBTHREADS;i++)
  {
	 pthread_create(&thr[NBTHREADS],NULL,dosleep,(void *)i);  
  }
  

  for (i=1;i<=NBTHREADS;i++)
  {
    pthread_join(thr[NBTHREADS],&retour);  
    fprintf(stderr,"le  thread %lx a fini\n",thr[NBTHREADS]); 
  }

  pthread_barrier_destroy(&barrier);

  return 0;
}



