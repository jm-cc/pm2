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
$Log: marcel_polling.h,v $
Revision 1.2  2000/04/11 09:07:12  rnamyst
Merged the "reorganisation" development branch.

Revision 1.1.2.1  2000/04/10 09:57:05  rnamyst
Moved the polling stuff into marcel_polling...

______________________________________________________________________________
*/

#ifndef MARCEL_POLLING_EST_DEF
#define MARCEL_POLLING_EST_DEF


#define MAX_POLL_IDS    16

_PRIVATE_ struct _poll_struct;
_PRIVATE_ struct _poll_cell_struct;

typedef struct _poll_struct *marcel_pollid_t;
typedef struct _poll_cell_struct *marcel_pollinst_t;

typedef void (*marcel_pollgroup_func_t)(marcel_pollid_t id);

typedef void *(*marcel_poll_func_t)(marcel_pollid_t id,
				    unsigned active,
				    unsigned sleeping,
				    unsigned blocked);

marcel_pollid_t marcel_pollid_create(marcel_pollgroup_func_t g,
				     marcel_poll_func_t f,
				     int divisor);

#define MARCEL_POLL_FAILED                NULL
#define MARCEL_POLL_SUCCESS(id)           ((id)->cur_cell)
#define MARCEL_POLL_SUCCESS_FOR(pollinst) (pollinst)

void marcel_poll(marcel_pollid_t id, any_t arg);

#define FOREACH_POLL(id, _arg) \
  for((id)->cur_cell = (id)->first_cell; \
      (id)->cur_cell != NULL && (_arg = (typeof(_arg))((id)->cur_cell->arg)); \
      (id)->cur_cell = (id)->cur_cell->next)

#define GET_CURRENT_POLLINST(id) ((id)->cur_cell)



_PRIVATE_ int marcel_check_polling(void);
_PRIVATE_ int marcel_polling_is_required(void);

_PRIVATE_ typedef struct _poll_struct {
  marcel_pollgroup_func_t gfunc;
  marcel_poll_func_t func;
  unsigned divisor, count;
  struct _poll_cell_struct *first_cell, *cur_cell;
  struct _poll_struct *prev, *next;
} poll_struct_t;

_PRIVATE_ typedef struct _poll_cell_struct {
  marcel_t task;
  boolean blocked;
  any_t arg;
  struct _poll_cell_struct *prev, *next;
} poll_cell_t;

#endif
