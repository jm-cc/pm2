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

#if defined(SUN4) || defined(SUN4SOL2) || defined(SUNMP) || defined(LINUX) || defined(RS6K) || defined(SGI5) || defined(X86SOL2)

extern long get_sp();

long pile()
{
	return get_sp();
}

#endif

#if defined(ALPHA)

extern void get_sp(long *);

long pile()
{ long p;

	get_sp(&p);
	return p;
}

#endif

void f()
{ int magic_number = 0x12345678;

   marcel_detach(marcel_self());

   tprintf("Au debut, marcel_self() = %lx, sp = %lx\n",
		marcel_self(), pile());
   tprintf("Pile restante = %ld\n", marcel_usablestack());
   { int i = 5000000; while(i--); }
/*   marcel_delay(500); */
   marcel_yield();
   tprintf("f est terminee\n");
   tprintf("En fin, marcel_self() = %lx, sp = %lx\n",
		marcel_self(), pile());
   tprintf("Pile restante = %ld\n", marcel_usablestack());
}

void effectuer_copie(unsigned long blk1, unsigned long depl2, unsigned long blk2, void *arg)
{ marcel_t new, old = (marcel_t)arg;

   tprintf("blk1=%d, depl2=%d, blk2=%d\n", blk1, depl2, blk2);

   new = marcel_alloc_stack(old->stack_size);
   memcpy(new, old, blk1);
   memcpy((char *)new+depl2, (char *)old+depl2, blk2);

   tprintf("memcpy est passe...\n");

   marcel_end_hibernation(new, old, NULL, NULL);
   printf("Copie effectuee\n");
}

int marcel_main(int argc, char *argv[])
{ marcel_t pid;

   marcel_init(&argc, argv);

   marcel_create(&pid, NULL, (marcel_func_t)f, NULL);

/*   marcel_delay(10); */
   marcel_yield();
   lock_task();
   if(pid->in_sighandler)
      tprintf("task IS in sig_handler...\n");
   else
      tprintf("task IS NOT in sig_handler...\n");
   marcel_freeze(&pid, 1);
   marcel_begin_hibernation(pid, effectuer_copie, pid, FALSE);
   unlock_task();

   marcel_end();
}
