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

#ifndef MADELEINE_EST_DEF
#define MADELEINE_EST_DEF

#include <sys/types.h>
#include <pointers.h>
#include <mad_types.h>
#include <mad_timing.h>
#include <sys/debug.h>

#ifdef PM2

#include <marcel.h>
extern exception ALIGN_ERROR;

#else

#ifndef FALSE
typedef enum { FALSE, TRUE } boolean;
#else
typedef int boolean;
#endif

#endif

typedef enum {
  MAD_IN_HEADER,
  MAD_IN_PLACE,
  MAD_IN_PLACE_N_FREE,
  MAD_BY_COPY
} madeleine_part;


void mad_init(int *argc, char **argv, int nb_proc, int *tids, int *nb, int *whoami);

void mad_buffers_init(void);

void mad_exit(void);

char *mad_arch_name(void);

boolean mad_can_send_to_self(void);

void mad_sendbuf_init(int dest_node);

void mad_sendbuf_send(void);

void mad_receive(void);

void mad_recvbuf_receive(void);

void old_mad_pack_byte(madeleine_part where, char *data, size_t nb);
void old_mad_unpack_byte(madeleine_part where, char *data, size_t nb);

void old_mad_pack_short(madeleine_part where, short *data, size_t nb);
void old_mad_unpack_short(madeleine_part where, short *data, size_t nb);

void old_mad_pack_int(madeleine_part where, int *data, size_t nb);
void old_mad_unpack_int(madeleine_part where, int *data, size_t nb);

void old_mad_pack_long(madeleine_part where, long *data, size_t nb);
void old_mad_unpack_long(madeleine_part where, long *data, size_t nb);

void old_mad_pack_float(madeleine_part where, float *data, size_t nb);
void old_mad_unpack_float(madeleine_part where, float *data, size_t nb);

void old_mad_pack_double(madeleine_part where, double *data, size_t nb);
void old_mad_unpack_double(madeleine_part where, double *data, size_t nb);

void old_mad_pack_pointer(madeleine_part where, pointer *data, size_t nb);
void old_mad_unpack_pointer(madeleine_part where, pointer *data, size_t nb);

void old_mad_pack_str(madeleine_part where, char *data);
void old_mad_unpack_str(madeleine_part where, char *data);

char *mad_aligned_malloc(int size);
void mad_aligned_free(void *ptr);

#endif
