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
$Log: isoaddr_const.h,v $
Revision 1.2  2000/07/14 16:17:09  gantoniu
Merged with branch dsm3

Revision 1.1.6.1  2000/06/13 16:44:07  gantoniu
New dsm branch.

Revision 1.1.4.1  2000/06/07 09:19:37  gantoniu
Merging new dsm with current PM2 : first try.

Revision 1.1.2.1  2000/05/31 17:39:52  gantoniu
Added some files

______________________________________________________________________________
*/

#ifndef ISOADDR_CONST_IS_DEF
#define ISOADDR_CONST_IS_DEF

typedef int node_t;

typedef struct _isoaddr_attr_t {
  int status;
  int protocol;
  int atomic;
  int align;
}isoaddr_attr_t;

#endif
