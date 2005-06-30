
/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 "the PM2 team" (see AUTHORS file)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

#ifndef PM2_MAD_EST_DEF
#define PM2_MAD_EST_DEF

#include "madeleine.h"
#define __MAD_ALIGNED__      MAD_ALIGNED 

#define to_pointer(arg, p_addr)  *((void **)p_addr) = arg
#define to_any_t(p_addr) (*((void **)p_addr))


/* Valeurs arbitraires */
#if 0
#define MAX_MODULES  64
#define MAX_HEADER   (64*1024)
#define MAX_IOVECS   128
#endif // 0

typedef union 
{
  char zone[8];
  long bidon;
} pointer;

void
pm2_mad_init(p_mad_madeleine_t madeleine);

void
pm2_begin_packing(p_mad_channel_t channel,
		  int             dest_node);

void
pm2_end_packing(void);

void
pm2_begin_unpacking(p_mad_channel_t channel);

void
pm2_end_unpacking(void);


#define SEND_CHEAPER  mad_send_CHEAPER
#define SEND_SAFER    mad_send_SAFER
#define SEND_LATER    mad_send_LATER
#define RECV_EXPRESS  mad_receive_EXPRESS
#define RECV_CHEAPER  mad_receive_CHEAPER

void
pm2_pack_byte(mad_send_mode_t     sm,
	      mad_receive_mode_t  rm,
	      void               *data,
	      size_t              nb);
void
pm2_unpack_byte(mad_send_mode_t     sm,
		mad_receive_mode_t  rm,
		void               *data,
		size_t              nb);

void
pm2_pack_short(mad_send_mode_t     sm,
	       mad_receive_mode_t  rm,
	       short              *data,
	       size_t              nb);
void
pm2_unpack_short(mad_send_mode_t     sm,
		 mad_receive_mode_t  rm,
		 short              *data,
		 size_t nb);

void
pm2_pack_int(mad_send_mode_t     sm,
	     mad_receive_mode_t  rm,
	     int                *data,
	     size_t              nb);
void
pm2_unpack_int(mad_send_mode_t     sm,
	       mad_receive_mode_t  rm,
	       int                *data,
	       size_t              nb);

void
pm2_pack_long(mad_send_mode_t      sm,
	      mad_receive_mode_t   rm,
	      long                *data,
	      size_t               nb);
void
pm2_unpack_long(mad_send_mode_t     sm,
		mad_receive_mode_t  rm,
		long               *data,
		size_t              nb);

void
pm2_pack_float(mad_send_mode_t     sm,
	       mad_receive_mode_t  rm,
	       float              *data,
	       size_t              nb);
void
pm2_unpack_float(mad_send_mode_t     sm,
		 mad_receive_mode_t  rm,
		 float              *data,
		 size_t              nb);

void
pm2_pack_double(mad_send_mode_t     sm,
		mad_receive_mode_t  rm,
		double             *data,
		size_t              nb);
void
pm2_unpack_double(mad_send_mode_t     sm,
		  mad_receive_mode_t  rm,
		  double             *data,
		  size_t              nb);

void
pm2_pack_pointer(mad_send_mode_t     sm,
		 mad_receive_mode_t  rm,
		 pointer            *data,
		 size_t              nb);
void
pm2_unpack_pointer(mad_send_mode_t     sm,
		   mad_receive_mode_t  rm,
		   pointer            *data,
		   size_t              nb);

/* !!! Use with great care: send_LATER mode unsupported */
void
pm2_pack_str(mad_send_mode_t     sm,
	     mad_receive_mode_t  rm,
	     char               *data);
void
pm2_unpack_str(mad_send_mode_t     sm,
	       mad_receive_mode_t  rm,
	       char               *data);

/* Previous MAD1 compatibility layer */
typedef enum 
{
  MAD_IN_HEADER,
  MAD_IN_PLACE,
  MAD_IN_PLACE_N_FREE,
  MAD_BY_COPY
} madeleine_part;

void
old_mad_pack_byte(madeleine_part where, void *data, size_t nb);
void
old_mad_unpack_byte(madeleine_part where, void *data, size_t nb);

void
old_mad_pack_short(madeleine_part where, short *data, size_t nb);
void
old_mad_unpack_short(madeleine_part where, short *data, size_t nb);

void
old_mad_pack_int(madeleine_part where, int *data, size_t nb);
void
old_mad_unpack_int(madeleine_part where, int *data, size_t nb);

void
old_mad_pack_long(madeleine_part where, long *data, size_t nb);
void
old_mad_unpack_long(madeleine_part where, long *data, size_t nb);

void
old_mad_pack_float(madeleine_part where, float *data, size_t nb);
void
old_mad_unpack_float(madeleine_part where, float *data, size_t nb);

void
old_mad_pack_double(madeleine_part where, double *data, size_t nb);
void
old_mad_unpack_double(madeleine_part where, double *data, size_t nb);

void
old_mad_pack_pointer(madeleine_part where, pointer *data, size_t nb);
void
old_mad_unpack_pointer(madeleine_part where, pointer *data, size_t nb);

void
old_mad_pack_str(madeleine_part where, char *data);
void
old_mad_unpack_str(madeleine_part where, char *data);

#endif
