/*
                      PM2 HIGH-PERF/ISOMALLOC
           High Performance Parallel Multithreaded Machine
                           version 3.0

     Gabriel Antoniu, Olivier Aumage, Luc Bouge, Vincent Danjean,
       Christian Perez, Jean-Francois Mehaut, Raymond Namyst

            Laboratoire de l'Informatique du Parallelisme
                        UMR 5668 CNRS-INRIA
                 Ecole Normale Superieure de Lyon

                      External Contributors:
                 Yves Denneulin (LMC - Grenoble),
                 Benoit Planquelle (LIFL - Lille)

                    1998 All Rights Reserved


                             NOTICE

 Permission to use, copy, modify, and distribute this software and
 its documentation for any purpose and without fee is hereby granted
 provided that the above copyright notice appear in all copies and
 that both the copyright notice and this permission notice appear in
 supporting documentation.

 Neither the institutions (Ecole Normale Superieure de Lyon,
 Laboratoire de L'informatique du Parallelisme, Universite des
 Sciences et Technologies de Lille, Laboratoire d'Informatique
 Fondamentale de Lille), nor the Authors make any representations
 about the suitability of this software for any purpose. This
 software is provided ``as is'' without express or implied warranty.
*/

#include <marcel.h>
#include <marcel_alloc.h>

static void *stack;

any_t thread_func(any_t arg)
{
  marcel_detach(marcel_self());

  printf("marcel_self = %p, sp = %p\n", marcel_self(), (void *)get_sp());

  /* Allocation d'une pile annexe, *obligatoirement* de taille SLOT_SIZE.  */
  stack = marcel_slot_alloc();
  printf("New stack goes from %p to %p\n", stack, stack + SLOT_SIZE - 1);

  /* Préparation de la nouvelle pile pour accueillir le thread */
  marcel_prepare_stack_jump(stack);

  /* basculement sur la pile annexe */
  set_sp(stack + SLOT_SIZE - 1024);

  printf("marcel_self = %p, sp = %p\n", marcel_self(), (void *)get_sp());

  printf("Here we are!!\n");

  /* on ne peut pas retourner avec "return", donc : */
  marcel_exit(NULL);

  return NULL;
}

int marcel_main(int argc, char *argv[])
{
  marcel_init(&argc, argv);

  marcel_create(NULL, NULL, thread_func, NULL);

  marcel_end();
  return 0;
}



