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

______________________________________________________________________________
$Log: exception.h,v $
Revision 1.3  2000/04/11 09:07:09  rnamyst
Merged the "reorganisation" development branch.

Revision 1.2.2.1  2000/03/15 15:54:38  vdanjean
réorganisation de marcel : commit pour CVS

Revision 1.2  2000/01/31 15:56:14  oaumage
- ajout du Log CVS


______________________________________________________________________________
*/

#ifndef MARCEL_EXCEPTION_EST_DEF
#define MARCEL_EXCEPTION_EST_DEF

typedef char *exception;

extern exception
  TASKING_ERROR,
  DEADLOCK_ERROR,
  STORAGE_ERROR,
  CONSTRAINT_ERROR,
  PROGRAM_ERROR,
  STACK_ERROR,
  TIME_OUT,
  NOT_IMPLEMENTED,
  USE_ERROR,
  LOCK_TASK_ERROR;

#define BEGIN          { _exception_block _excep_blk; \
                       marcel_t __cur = marcel_self(); \
                       _excep_blk.old_blk=__cur->cur_excep_blk; \
                       __cur->cur_excep_blk=&_excep_blk; \
                       if (setjmp(_excep_blk.jb) == 0) {
#define EXCEPTION      __cur->cur_excep_blk=_excep_blk.old_blk; } \
                       else { __cur->cur_excep_blk=_excep_blk.old_blk; \
                       if(always_false) {
#define WHEN(ex)       } else if (__cur->cur_exception == ex) {
#define WHEN2(ex1,ex2) } else if (__cur->cur_exception == ex1 || \
                                  __cur->cur_exception == ex2) {
#define WHEN3(ex1,ex2,ex3) } else if (__cur->cur_exception == ex1 || \
                                      __cur->cur_exception == ex2 || \
                                      __cur->cur_exception == ex3) {
#define WHEN_OTHERS    } else if(!always_false) {
#define END            } else _raise(NULL); }}

#define RAISE(ex)      (marcel_self()->exfile=__FILE__,marcel_self()->exline=__LINE__,_raise(ex))
#define RRAISE         (marcel_self()->exfile=__FILE__,marcel_self()->exline=__LINE__,_raise(NULL))

/* =================== Quelques definitions utiles ==================== */

#define LOOP(name)	{ _exception_block _##name##_excep_blk; \
	int _##name##_val; \
	_##name##_excep_blk.old_blk=marcel_self()->cur_excep_blk; \
	marcel_self()->cur_excep_blk=&_##name##_excep_blk; \
	if ((_##name##_val = setjmp(_##name##_excep_blk.jb)) == 0) { \
	while(!always_false) {

#define EXIT_LOOP(name) longjmp(_##name##_excep_blk.jb, 2)

#define END_LOOP(name) } } \
	marcel_self()->cur_excep_blk=_##name##_excep_blk.old_blk; \
	if(_##name##_val == 1) _raise(NULL); }


_PRIVATE_ extern volatile boolean always_false;
_PRIVATE_ int _raise(exception ex);

#endif
