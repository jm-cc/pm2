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

#ifdef SUN4
#define SP_OFFSET	2
#endif

#if defined(SUN4SOL2) || defined(SUNMP)
#define SP_OFFSET	3
#define FP_OFFSET	1
#endif

#ifdef ALPHA
#define SP_OFFSET	27
#endif

#ifdef X86SOL2
#define SP_OFFSET	3
#endif

#ifdef RS6K
#define SP_OFFSET	3
#endif

#ifdef SGI5
#define SP_OFFSET	2
#endif

#ifdef LINUX

long jb_sp(marcel_t pid)
{
   return (long)pid->jb->__bp;
}

#else

long jb_sp(marcel_t pid)
{
   return (long)pid->jb[SP_OFFSET];
}

#endif

void dumper(long haut, long bas)
{ long ptr;

   for(ptr=haut; ptr >= bas; ptr -= sizeof(long))
      printf("Dump : (%lx) = %lx\n", ptr, *((long *)ptr) );
}

void f2()
{
      printf("f2'sp = %lx\n", pile());
      longjmp(marcel_self()->jb, 1);
}

void f()
{ int i;
  int magic_number = 0x12345678;

   lock_task();
   if(setjmp(marcel_self()->jb) == 0) {
      printf("f'sp = %lx\n", pile());
      f2();
   }
   printf("f'sp = %lx\n", pile());
   unlock_task();

   marcel_yield();
   i=5000000; while(i--);
/*   marcel_delay(500); */
   tprintf("f est terminee\n");
}

void print_jmp_buf(jmp_buf buf)
{ int i;

	for(i=0; i<sizeof(jmp_buf)/sizeof(long); i++)
		printf("jb[%d] = %lx\n", i, ((long *)buf)[i]);
}

int marcel_main(int argc, char *argv[])
{ marcel_t pid;
  any_t status;

   marcel_init(&argc, argv);

   marcel_create(&pid, NULL, (marcel_func_t)f, NULL);
   marcel_yield();
   marcel_delay(10);
   lock_task();
   dumper(pid->initial_sp, jb_sp(pid));
   print_jmp_buf(pid->jb);
   if(pid->in_sighandler)
      tprintf("task IS in sig_handler...\n");
   else
      tprintf("task IS NOT in sig_handler...\n");
   unlock_task();

   marcel_join(pid, &status);

   marcel_end();
}

