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
$Log: marcel_io.h,v $
Revision 1.2  2000/01/31 15:56:21  oaumage
- ajout du Log CVS


______________________________________________________________________________
*/

#ifndef MARCEL_IO_EST_DEF
#define MARCEL_IO_EST_DEF

#include <unistd.h>
#include <sys/time.h>
#include <sys/uio.h>

void marcel_io_init(void);

int marcel_read(int fildes, void *buf, size_t nbytes);

int marcel_write(int fildes, void *buf, size_t nbytes);

int marcel_select(int nfds, fd_set *rfds, fd_set *wfds);

int marcel_readv(int fildes, const struct iovec *iov, int iovcnt);

int marcel_writev(int fildes, const struct iovec *iov, int iovcnt);

/* To force reading/writing an exact number of bytes */

int marcel_read_exactly(int fildes, void *buf, size_t nbytes);

int marcel_write_exactly(int fildes, void *buf, size_t nbytes);

int marcel_readv_exactly(int fildes, const struct iovec *iov, int iovcnt);

int marcel_writev_exactly(int fildes, const struct iovec *iov, int iovcnt);

#endif
