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
$Log: pm2_mad.h,v $
Revision 1.3  2000/07/14 16:17:07  gantoniu
Merged with branch dsm3

Revision 1.2.10.1  2000/07/12 15:10:48  gantoniu
*** empty log message ***

Revision 1.2  2000/02/28 11:18:00  rnamyst
Changed #include <> into #include "".

Revision 1.1  2000/02/02 10:29:38  rnamyst
Sorry, I forgot to ad it...

______________________________________________________________________________
*/

#ifndef PM2_MAD_EST_DEF
#define PM2_MAD_EST_DEF

#include "madeleine.h"

#ifdef MAD1

/* Madeleine I */

typedef enum
{
  mad_send_SAFER,
  mad_send_LATER,
  mad_send_CHEAPER
} mad_send_mode_t;

#define SEND_SAFER	mad_send_SAFER
#define SEND_LATER	mad_send_LATER
#define SEND_CHEAPER	mad_send_CHEAPER

typedef enum
{
  mad_receive_EXPRESS,
  mad_receive_CHEAPER
} mad_receive_mode_t;

#define RECV_EXPRESS	mad_receive_EXPRESS
#define RECV_CHEAPER	mad_receive_CHEAPER

void pm2_pack_byte(mad_send_mode_t sm,
		   mad_receive_mode_t rm,
		   char *data,
		   size_t nb);
void pm2_unpack_byte(mad_send_mode_t sm,
		     mad_receive_mode_t rm,
		     char *data,
		     size_t nb);

void pm2_pack_str(mad_send_mode_t sm,
		  mad_receive_mode_t rm,
		  char *data);
void pm2_unpack_str(mad_send_mode_t sm,
		    mad_receive_mode_t rm,
		    char *data);

static void __inline__ pm2_pack_int(mad_send_mode_t sm,
				    mad_receive_mode_t rm,
				    int *data,
				    size_t nb) __attribute__ ((unused));
static void __inline__ pm2_pack_int(mad_send_mode_t sm,
				    mad_receive_mode_t rm,
				    int *data,
				    size_t nb)
{
  pm2_pack_byte(sm, rm, (char *)data, nb*sizeof(int));
}



static void __inline__ pm2_unpack_int(mad_send_mode_t sm,
				      mad_receive_mode_t rm,
				      int *data,
				      size_t nb) __attribute__ ((unused));
static void __inline__ pm2_unpack_int(mad_send_mode_t sm,
				      mad_receive_mode_t rm,
				      int *data,
				      size_t nb)
{
  pm2_unpack_byte(sm, rm, (char *)data, nb*sizeof(int));
}

#else

/* Madeleine II */

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


#define SEND_CHEAPER  mad_send_CHEAPER
#define SEND_SAFER    mad_send_SAFER
#define SEND_LATER    mad_send_LATER
#define RECV_EXPRESS  mad_receive_EXPRESS
#define RECV_CHEAPER  mad_receive_CHEAPER

void pm2_pack_byte(mad_send_mode_t     sm,
		   mad_receive_mode_t  rm,
		   char               *data,
		   size_t              nb);
void pm2_unpack_byte(mad_send_mode_t     sm,
		     mad_receive_mode_t  rm,
		     char               *data,
		     size_t              nb);

void pm2_pack_short(mad_send_mode_t     sm,
		    mad_receive_mode_t  rm,
		    short              *data,
		    size_t              nb);
void pm2_unpack_short(mad_send_mode_t     sm,
		      mad_receive_mode_t  rm,
		      short              *data,
		      size_t nb);

void pm2_pack_int(mad_send_mode_t     sm,
		  mad_receive_mode_t  rm,
		  int                *data,
		  size_t              nb);
void pm2_unpack_int(mad_send_mode_t     sm,
		    mad_receive_mode_t  rm,
		    int                *data,
		    size_t              nb);

void pm2_pack_long(mad_send_mode_t      sm,
		   mad_receive_mode_t   rm,
		   long                *data,
		   size_t               nb);
void pm2_unpack_long(mad_send_mode_t     sm,
		     mad_receive_mode_t  rm,
		     long               *data,
		     size_t              nb);

void pm2_pack_float(mad_send_mode_t     sm,
		    mad_receive_mode_t  rm,
		    float              *data,
		    size_t              nb);
void pm2_unpack_float(mad_send_mode_t     sm,
		      mad_receive_mode_t  rm,
		      float              *data,
		      size_t              nb);

void pm2_pack_double(mad_send_mode_t     sm,
		     mad_receive_mode_t  rm,
		     double             *data,
		     size_t              nb);
void pm2_unpack_double(mad_send_mode_t     sm,
		       mad_receive_mode_t  rm,
		       double             *data,
		       size_t              nb);

void pm2_pack_pointer(mad_send_mode_t     sm,
		      mad_receive_mode_t  rm,
		      pointer            *data,
		      size_t              nb);
void pm2_unpack_pointer(mad_send_mode_t     sm,
			mad_receive_mode_t  rm,
			pointer            *data,
			size_t              nb);

/* !!! Use with great care: send_LATER mode unsupported */
void pm2_pack_str(mad_send_mode_t     sm,
		  mad_receive_mode_t  rm,
		  char               *data);
void pm2_unpack_str(mad_send_mode_t     sm,
		    mad_receive_mode_t  rm,
		    char               *data);

/* Previous MAD1 compatibility layer */
typedef enum {
  MAD_IN_HEADER,
  MAD_IN_PLACE,
  MAD_IN_PLACE_N_FREE,
  MAD_BY_COPY
} madeleine_part;

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

p_mad_madeleine_t mad_get_madeleine(void);

#endif

#endif
