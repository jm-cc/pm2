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
#include <stdio.h>
#include <unistd.h>

static int tube[2];

any_t f(any_t arg)
{
  char chaine[1024];

  marcel_detach(marcel_self());

  marcel_delay(100);

  printf("I'm waiting for a keyboard input\n");
  marcel_read(STDIN_FILENO, chaine, 1024);
  printf("Input from [keyboard] detected on LWP %d: %s",
	 marcel_current_vp(), chaine);
   
  return NULL;
}

any_t g(any_t arg)
{
  unsigned i;

  marcel_detach(marcel_self());

  marcel_delay(100);

  printf("I'm waiting for a pipe input\n");
  marcel_read(tube[0], &i, sizeof(i));
  printf("Input from [pipe] detected on LWP %d: %x\n",
	 marcel_current_vp(), i);

  return NULL;
}

int marcel_main(int argc, char *argv[])
{
  unsigned long i;
  unsigned magic = 0xdeadbeef;

  marcel_init(&argc, argv);

  pipe(tube);

  marcel_create(NULL, NULL, f, NULL);
  marcel_create(NULL, NULL, g, NULL);

  fprintf(stderr, "busy waiting...\n");

  i = 200000000L; while(--i);

  fprintf(stderr, "marcel_delay...\n");

  marcel_delay(5000);

  write(tube[1], &magic, sizeof(magic));

  marcel_end();
  return 0;
}

