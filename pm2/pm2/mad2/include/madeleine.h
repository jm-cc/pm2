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

/*
 * Madeleine.h
 * ===========
 */

#ifndef MAD_H
#define MAD_H

/*
 * Headers
 * -------
 */

#ifdef PM2
#include <marcel.h>
#endif /* PM2 */
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

/* Timing */
#include <mad_timing.h>

/* Protocol registration */
#include <mad_registration.h>

/* Macros */
#include <mad_macros.h>

/* Structure pointers */
#include <mad_pointers.h>

/* Fundamental data types */
#include <mad_types.h>
#include <mad_malloc.h>
#include <mad_modes.h>
#include <mad_list.h>
#include <mad_buffers.h>
#include <mad_link.h>
#include <mad_connection.h>
#include <mad_channel.h>
#include <mad_adapter.h>
#include <mad_driver_interface.h>
#include <mad_driver.h>
#include <mad_adapter_description.h>
#include <mad_configuration.h>

/* Function prototypes */
#include <mad_malloc_interface.h>
#include <mad_memory_interface.h>
#include <mad_buffer_interface.h>
#include <mad_list_interface.h>
#include <mad_communication_interface.h>
#include <mad_channel_interface.h>

/* connection interfaces */
#ifdef DRV_TCP
#include <connection_interfaces/mad_tcp.h>
#endif /* DRV_TCP */
#ifdef DRV_VIA
#include <connection_interfaces/mad_via.h>
#endif /* DRV_VIA */
#ifdef DRV_SISCI
#include <connection_interfaces/mad_sisci.h>
#endif /* DRV_SISCI */
#ifdef DRV_SBP
#include <connection_interfaces/mad_sbp.h>
#endif /* DRV_SISCI */
#ifdef DRV_MPI
#include <connection_interfaces/mad_mpi.h>
#endif /* DRV_MPI */

#include <mad_main.h>

#ifdef MAD2_MAD1
/* Interfacage de compatibilite ascendante avec Mad1 */
#define MAD_ALIGNMENT         32

#ifdef __GNUC__
#define __MAD_ALIGNED__       __attribute__ ((aligned (MAD_ALIGNMENT)))
#else
#define __MAD_ALIGNED__
#endif

/* Valeurs arbitraires */
#define MAX_MODULES           64
#define MAX_HEADER            (64*1024)
#define MAX_IOVECS            128

/* Non supporte par Mad2 (pas de header)*/
#define PIGGYBACK_AREA_LEN    0

typedef union {
   char zone[8];
   long bidon;
} pointer;

#define to_pointer(arg, p_addr)  *((void **)p_addr) = arg

#define to_any_t(p_addr) (*((void **)p_addr))

void mad_init(int *argc, char **argv, int nb_proc, int *tids, int *nb, int *whoami);

void mad_buffers_init(void);

void mad_exit(void);

char *mad_arch_name(void);

boolean mad_can_send_to_self(void);

void mad_sendbuf_init(p_mad_channel_t channel, int dest_node);

void mad_sendbuf_send(void);

void mad_sendbuf_free(void);

void mad_receive(p_mad_channel_t channel);

void mad_recvbuf_receive(void);

typedef enum {
  MAD_IN_HEADER,
  MAD_IN_PLACE,
  MAD_IN_PLACE_N_FREE,
  MAD_BY_COPY
} madeleine_part;

void mad_pack_byte(madeleine_part where, char *data, size_t nb);
void mad_unpack_byte(madeleine_part where, char *data, size_t nb);

void mad_pack_short(madeleine_part where, short *data, size_t nb);
void mad_unpack_short(madeleine_part where, short *data, size_t nb);

void mad_pack_int(madeleine_part where, int *data, size_t nb);
void mad_unpack_int(madeleine_part where, int *data, size_t nb);

void mad_pack_long(madeleine_part where, long *data, size_t nb);
void mad_unpack_long(madeleine_part where, long *data, size_t nb);

void mad_pack_float(madeleine_part where, float *data, size_t nb);
void mad_unpack_float(madeleine_part where, float *data, size_t nb);

void mad_pack_double(madeleine_part where, double *data, size_t nb);
void mad_unpack_double(madeleine_part where, double *data, size_t nb);

void mad_pack_pointer(madeleine_part where, pointer *data, size_t nb);
void mad_unpack_pointer(madeleine_part where, pointer *data, size_t nb);

void mad_pack_str(madeleine_part where, char *data);
void mad_unpack_str(madeleine_part where, char *data);

p_mad_madeleine_t mad_get_madeleine();

#endif MAD2_MAD1

#endif /* MAD_H */
