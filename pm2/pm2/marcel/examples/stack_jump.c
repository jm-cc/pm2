
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

#include "marcel.h"
#include "marcel_alloc.h"

#ifdef ENABLE_STACK_JUMPING

static void *stack;

any_t thread_func(any_t arg)
{
  marcel_detach(marcel_self());

  printf("marcel_self = %p, sp = %p\n", marcel_self(), (void *)get_sp());

  /* Allocation d'une pile annexe, *obligatoirement* de taille THREAD_SLOT_SIZE.  */
  stack = marcel_slot_alloc();
  printf("New stack goes from %p to %p\n", stack, stack + THREAD_SLOT_SIZE - 1);

  /* Pr�paration de la nouvelle pile pour accueillir le thread */
  marcel_prepare_stack_jump(stack);

  /* basculement sur la pile annexe */
  set_sp(stack + THREAD_SLOT_SIZE - 1024);

  printf("marcel_self = %p, sp = %p\n", marcel_self(), (void *)get_sp());

  printf("Here we are!!\n");

  /* on ne peut pas retourner avec "return", donc : */
  marcel_exit(NULL);

  return NULL;
}

#endif

int marcel_main(int argc, char *argv[])
{
#ifndef ENABLE_STACK_JUMPING

  fprintf(stderr,
	  "Please compile this program (and the Marcel library)\n"
	  "with the -DENABLE_STACK_JUMPING flag.\n");
  exit(1);

#else

  marcel_init(&argc, argv);

  marcel_create(NULL, NULL, thread_func, NULL);

  marcel_end();

#endif
  return 0;
}



