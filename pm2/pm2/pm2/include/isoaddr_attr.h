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
$Log: isoaddr_attr.h,v $
Revision 1.2  2000/07/14 16:17:04  gantoniu
Merged with branch dsm3

Revision 1.1.6.1  2000/06/13 16:44:05  gantoniu
New dsm branch.

Revision 1.1.4.1  2000/06/07 09:19:35  gantoniu
Merging new dsm with current PM2 : first try.

Revision 1.1.2.1  2000/05/31 17:39:51  gantoniu
Added some files

______________________________________________________________________________
*/

#ifndef ISOADDR_ATTR_IS_DEF
#define ISOADDR_ATTR_IS_DEF

#define isoaddr_attr_set_status(attr, _status) (attr)->status = _status

#define isoaddr_attr_set_protocol(attr, _protocol) (attr)->protocol = _protocol

#define isoaddr_attr_set_alignment(attr, _align) (attr)->align = _align

#define isoaddr_attr_set_atomic(attr) (attr)->atomic = 1;

#define isoaddr_attr_set_non_atomic(attr) (attr)->atomic = 0;

#define isoaddr_attr_get_status(attr) ((attr)->status)

#define isoaddr_attr_get_protocol(attr) ((attr)->protocol)

#define isoaddr_attr_get_alignment(attr) ((attr)->align)

#define isoaddr_attr_atomic(attr) ((attr)->atomic)

#endif


