
/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2006 "the PM2 team" (see AUTHORS file)
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

#include "marcel.h"

#include <stdio.h>
#include <stdlib.h>


#ifdef MARCEL_FORTRAN
extern int  iargc_();
extern void getarg_(int*,char*,int);


void marcel_init_(){
  int argc = 0;
  int argsize = 1024;
  char **argv = NULL;
  int i = 0;
  int j = 0;

  /* Recover the args with the Fortran routines iargc_ and getarg_ */
  argc = iargc_() + 1;
    
  argv = (char **) malloc (argc * sizeof (char *));

  for(i = 0; i < argc; i++){

    argv[i] = (char *) malloc (argsize * sizeof (char));
    getarg_ (&i, argv[i], argsize);
    argv[i][argsize-1] = 0;

    /* Trim trailing blanks */
    j = argsize - 2;
    while(j!= 0 && argv[i][j] == ' '){

      argv[i][j] = 0;
      j--;
    }

  }
  
  marcel_init (&argc, argv);

  for(i = 0; i < argc; i++){
    free (argv[i]);
  }

  free(argv);
}


void marcel_end_ (){
  marcel_end();
}


void marcel_self_(marcel_t *self){
  *self = marcel_self();
}

// a tester sur d'autres architectures et compilateurs
void marcel_create_(marcel_t *pid, void* (*func)(void*), void *arg){
  marcel_create (pid, NULL,  func, arg);
}


void marcel_join_(marcel_t *pid){
  marcel_join(*pid, NULL);
}


void marcel_yield_(){
  marcel_yield();
}


void marcel_set_load_(int *load) {
  *marcel_stats_get(marcel_self(), marcel_stats_load_offset) = *load;
}


void marcel_spread_() {
  marcel_bubble_spread(&marcel_root_bubble, marcel_machine_level);
}
#endif
