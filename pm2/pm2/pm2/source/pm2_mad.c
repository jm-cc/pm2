
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

#include "pm2_mad.h"

#ifdef MAD1

/* Madeleine I */

void pm2_pack_byte(mad_send_mode_t sm,
		   mad_receive_mode_t rm,
		   char *data,
		   size_t nb)
{
  switch(sm) {
  case SEND_CHEAPER : {
    switch(rm) {
    case RECV_EXPRESS : {
      old_mad_pack_byte(MAD_IN_HEADER, data, nb);
      break;
    }
    case RECV_CHEAPER : {
      old_mad_pack_byte(MAD_IN_PLACE, data, nb);
      break;
    }
    default:
      fprintf(stderr, "Oups! This packing mode is not supported by Madeleine I.\n");
      exit(1);
    }
    break;
  }
  case SEND_SAFER : {
    old_mad_pack_byte(MAD_IN_HEADER, data, nb);
    break;
  }
  case SEND_LATER : {
    switch(rm) {
    case RECV_CHEAPER : {
      old_mad_pack_byte(MAD_IN_PLACE, data, nb);
      break;
    }
    default:
      fprintf(stderr, "Oups! This packing mode is not supported by Madeleine I.\n");
      exit(1);
    }
    break;
  }
  default:
    fprintf(stderr, "Oups! This packing mode is not supported by Madeleine I.\n");
    exit(1);
  }
}

void pm2_unpack_byte(mad_send_mode_t sm,
		     mad_receive_mode_t rm,
		     char *data,
		     size_t nb)
{
  switch(sm) {
  case SEND_CHEAPER : {
    switch(rm) {
    case RECV_EXPRESS : {
      old_mad_unpack_byte(MAD_IN_HEADER, data, nb);
      break;
    }
    case RECV_CHEAPER : {
      old_mad_unpack_byte(MAD_IN_PLACE, data, nb);
      break;
    }
    default:
      fprintf(stderr, "Oups! This packing mode is not supported by Madeleine I.\n");
      exit(1);
    }
    break;
  }
  case SEND_SAFER : {
    old_mad_unpack_byte(MAD_IN_HEADER, data, nb);
    break;
  }
  case SEND_LATER : {
    switch(rm) {
    case RECV_CHEAPER : {
      old_mad_unpack_byte(MAD_IN_PLACE, data, nb);
      break;
    }
    default:
      fprintf(stderr, "Oups! This packing mode is not supported by Madeleine I.\n");
      exit(1);
    }
    break;
  }
  default:
    fprintf(stderr, "Oups! This packing mode is not supported by Madeleine I.\n");
    exit(1);
  }
}

void pm2_pack_str(mad_send_mode_t sm,
		  mad_receive_mode_t rm,
		  char *data)
{
  switch(sm) {
  case SEND_CHEAPER : {
    switch(rm) {
    case RECV_EXPRESS : {
      old_mad_pack_str(MAD_IN_HEADER, data);
      break;
    }
    case RECV_CHEAPER : {
      old_mad_pack_str(MAD_IN_PLACE, data);
      break;
    }
    default:
      fprintf(stderr, "Oups! This packing mode is not supported by Madeleine I.\n");
      exit(1);
    }
    break;
  }
  case SEND_SAFER : {
    old_mad_pack_str(MAD_IN_HEADER, data);
    break;
  }
  case SEND_LATER : {
    switch(rm) {
    case RECV_CHEAPER : {
      old_mad_pack_str(MAD_IN_PLACE, data);
      break;
    }
    default:
      fprintf(stderr, "Oups! This packing mode is not supported by Madeleine I.\n");
      exit(1);
    }
    break;
  }
  default:
    fprintf(stderr, "Oups! This packing mode is not supported by Madeleine I.\n");
    exit(1);
  }
}

void pm2_unpack_str(mad_send_mode_t sm,
		    mad_receive_mode_t rm,
		    char *data)
{
  switch(sm) {
  case SEND_CHEAPER : {
    switch(rm) {
    case RECV_EXPRESS : {
      old_mad_unpack_str(MAD_IN_HEADER, data);
      break;
    }
    case RECV_CHEAPER : {
      old_mad_unpack_str(MAD_IN_PLACE, data);
      break;
    }
    default:
      fprintf(stderr, "Oups! This packing mode is not supported by Madeleine I.\n");
      exit(1);
    }
    break;
  }
  case SEND_SAFER : {
    old_mad_unpack_str(MAD_IN_HEADER, data);
    break;
  }
  case SEND_LATER : {
    switch(rm) {
    case RECV_CHEAPER : {
      old_mad_unpack_str(MAD_IN_PLACE, data);
      break;
    }
    default:
      fprintf(stderr, "Oups! This packing mode is not supported by Madeleine I.\n");
      exit(1);
    }
    break;
  }
  default:
    fprintf(stderr, "Oups! This packing mode is not supported by Madeleine I.\n");
    exit(1);
  }
}

#else

/* Madeleine II */

marcel_key_t         mad2_send_key, mad2_recv_key;
#define ALIGNMASK        (MAD_ALIGNMENT-1)
/* semaphore used to block sends during slot negociations: */
_PRIVATE_ extern marcel_key_t        _pm2_isomalloc_nego_key;
                 marcel_sem_t         sem_nego;

static p_mad_madeleine_t main_madeleine;
#if 0
void mad_init(int *argc, char *argv[])
{
  p_mad_adapter_set_t   adapter_set;
  
  LOG_IN();

  adapter_set =
    mad_adapter_set_init(1, MAD2_MAD1_MAIN_PROTO, MAD2_MAD1_MAIN_PROTO_PARAM);

  main_madeleine = mad2_init(argc, argv, NULL, adapter_set);

  LOG_OUT();
}
#endif

void pm2_mad_init(p_mad_madeleine_t madeleine)
{
  main_madeleine = madeleine;

  marcel_key_create(&mad2_send_key, NULL);
  marcel_key_create(&mad2_recv_key, NULL);
  marcel_sem_init(&sem_nego, 1);
}

void
pm2_mad_exit(void)
{
  LOG_IN();
  mad_exit(main_madeleine);
  LOG_OUT();
}

char *
mad_arch_name(void)
{
  LOG_IN();
  LOG_OUT();
  return main_madeleine->adapter->name;
}

boolean
mad_can_send_to_self(void)
{
  LOG_IN();
  LOG_OUT();
  return 0;
}

void
mad_sendbuf_init(p_mad_channel_t channel, int dest_node)
{
  LOG_IN();
  marcel_setspecific(mad2_send_key, mad_begin_packing(channel, dest_node));
  LOG_OUT();
}

void
mad_sendbuf_send(void)
{
  LOG_IN();
  mad_end_packing(marcel_getspecific(mad2_send_key));
  LOG_OUT();
}

void
mad_sendbuf_free(void)
{
  /* rien a priori */
}

void
mad_receive(p_mad_channel_t channel)
{
  LOG_IN();
  marcel_setspecific(mad2_recv_key, mad_begin_unpacking(channel));
  LOG_OUT();
}

void
mad_recvbuf_receive(void)
{
  LOG_IN();
  mad_end_unpacking(marcel_getspecific(mad2_recv_key));
  LOG_OUT();
}

void
pm2_pack_byte(mad_send_mode_t     sm,
	      mad_receive_mode_t  rm,
	      char               *data,
	      size_t              nb)
{
  LOG_IN();  
  mad_pack(marcel_getspecific(mad2_send_key), data, nb, sm, rm);
  LOG_OUT();
}

void
pm2_unpack_byte(mad_send_mode_t     sm,
		mad_receive_mode_t  rm,
		char               *data,
		size_t              nb)
{
  LOG_IN();
  mad_unpack(marcel_getspecific(mad2_recv_key), data, nb, sm, rm);
  LOG_OUT();
}

void
pm2_pack_short(mad_send_mode_t     sm,
	       mad_receive_mode_t  rm,
	       short              *data,
	       size_t              nb)
{
  LOG_IN();
  mad_pack(marcel_getspecific(mad2_send_key),
	   data,
	   nb*sizeof(short),
	   sm,
	   rm);
  LOG_OUT();
}

void
pm2_unpack_short(mad_send_mode_t     sm,
		 mad_receive_mode_t  rm,
		 short              *data,
		 size_t              nb)
{
  LOG_IN();
  mad_unpack(marcel_getspecific(mad2_recv_key),
	     data,
	     nb*sizeof(short),
	     sm,
	     rm);
  LOG_OUT();
}

void
pm2_pack_int(mad_send_mode_t     sm,
	     mad_receive_mode_t  rm,
	     int                *data,
	     size_t              nb)
{
  LOG_IN();
  mad_pack(marcel_getspecific(mad2_send_key),
	   data,
	   nb*sizeof(int),
	   sm,
	   rm);
  LOG_OUT();
}

void
pm2_unpack_int(mad_send_mode_t     sm,
	       mad_receive_mode_t  rm,
	       int                *data,
	       size_t              nb)
{
  LOG_IN();
  mad_unpack(marcel_getspecific(mad2_recv_key),
	   data,
	   nb*sizeof(int),
	   sm,
	   rm);
  LOG_OUT();
}

void
pm2_pack_long(mad_send_mode_t     sm,
	      mad_receive_mode_t  rm,
	      long               *data,
	      size_t              nb)
{
  LOG_IN();
  mad_pack(marcel_getspecific(mad2_send_key),
	   data,
	   nb*sizeof(long),
	   sm,
	   rm);
  LOG_OUT();
}

void
pm2_unpack_long(mad_send_mode_t     sm,
		mad_receive_mode_t  rm,
		long               *data,
		size_t              nb)
{
  LOG_IN();
  mad_unpack(marcel_getspecific(mad2_recv_key),
	   data,
	   nb*sizeof(long),
	   sm,
	   rm);
  LOG_OUT();
}

void
pm2_pack_float(mad_send_mode_t     sm,
	       mad_receive_mode_t  rm,
	       float              *data,
	       size_t              nb)
{
  LOG_IN();
  mad_pack(marcel_getspecific(mad2_send_key),
	   data,
	   nb*sizeof(float),
	   sm,
	   rm);
  LOG_OUT();
}

void
pm2_unpack_float(mad_send_mode_t     sm,
		 mad_receive_mode_t  rm,
		 float              *data,
		 size_t              nb)
{
  LOG_IN();
  mad_unpack(marcel_getspecific(mad2_recv_key),
	   data,
	   nb*sizeof(float),
	   sm,
	   rm);
  LOG_OUT();
}

void
pm2_pack_double(mad_send_mode_t     sm,
		mad_receive_mode_t  rm,
		double             *data,
		size_t              nb)
{
  LOG_IN();
  mad_pack(marcel_getspecific(mad2_send_key),
	   data,
	   nb*sizeof(double),
	   sm,
	   rm);
  LOG_OUT();
}

void
pm2_unpack_double(mad_send_mode_t     sm,
		  mad_receive_mode_t  rm,
		  double             *data,
		  size_t              nb)
{
  LOG_IN();
  mad_unpack(marcel_getspecific(mad2_recv_key),
	   data,
	   nb*sizeof(double),
	   sm,
	   rm);
  LOG_OUT();
}

void
pm2_pack_pointer(mad_send_mode_t     sm,
		 mad_receive_mode_t  rm,
		 pointer            *data,
		 size_t              nb)
{
  LOG_IN();
  mad_pack(marcel_getspecific(mad2_send_key),
	   data,
	   nb*sizeof(pointer),
	   sm,
	   rm);
  LOG_OUT();
}

void
pm2_unpack_pointer(mad_send_mode_t     sm,
		   mad_receive_mode_t  rm,
		   pointer            *data,
		   size_t              nb)
{
  LOG_IN();
  mad_unpack(marcel_getspecific(mad2_recv_key),
	   data,
	   nb*sizeof(pointer),
	   sm,
	   rm);
  LOG_OUT();
}

void
pm2_pack_str(mad_send_mode_t     sm,
	     mad_receive_mode_t  rm,
	     char               *data)
{
  int len = strlen(data);
  
  LOG_IN();
  if ((sm != mad_send_SAFER) && (sm != mad_send_CHEAPER))
    FAILURE("unimplemented feature");
  mad_pack(marcel_getspecific(mad2_send_key),
	   &len,
	   sizeof(int),
	   sm,
	   mad_receive_EXPRESS);
  mad_pack(marcel_getspecific(mad2_send_key),
	   data,
	   len + 1,
	   sm,
	   rm);
  LOG_OUT();
}

void
pm2_unpack_str(mad_send_mode_t     sm,
	       mad_receive_mode_t  rm,
	       char               *data)
{
  int len;

  LOG_IN();
  if ((sm != mad_send_SAFER) && (sm != mad_send_CHEAPER))
    FAILURE("unimplemented feature");
  
  mad_unpack(marcel_getspecific(mad2_recv_key),
	     &len,
	     sizeof(int),
	     sm,
	     mad_receive_EXPRESS);
  mad_unpack(marcel_getspecific(mad2_recv_key),
	     data,
	     len + 1,
	     sm,
	     rm);
  LOG_OUT();
}

void
old_mad_pack_byte(madeleine_part where, char *data, size_t nb)
{
  LOG_IN();
  
   switch(where)
    {
       case MAD_IN_HEADER :
	 {
	   mad_pack(marcel_getspecific(mad2_send_key), data, nb,
		    mad_send_SAFER, mad_receive_EXPRESS);
	   break;
	 }
       case MAD_IN_PLACE :
	 {
	   mad_pack(marcel_getspecific(mad2_send_key), data, nb,
		    mad_send_CHEAPER, mad_receive_CHEAPER);
	   break;
	 }
       case MAD_BY_COPY :
	 {
	   mad_pack(marcel_getspecific(mad2_send_key), data, nb,
		    mad_send_SAFER, mad_receive_CHEAPER);
	   break;
	 }
    default: FAILURE("Unknown pack mode");
    }

  LOG_OUT();
}

void
old_mad_unpack_byte(madeleine_part where, char *data, size_t nb)
{
  LOG_IN();
  
  switch(where)
    {
       case MAD_IN_HEADER :
	 {
	   mad_unpack(marcel_getspecific(mad2_recv_key), data, nb,
		      mad_send_SAFER, mad_receive_EXPRESS);
	   break;
	 }
       case MAD_IN_PLACE :
	 {
	   mad_unpack(marcel_getspecific(mad2_recv_key), data, nb,
		      mad_send_CHEAPER, mad_receive_CHEAPER);
	   break;
	 }
       case MAD_BY_COPY :
	 {
	   mad_unpack(marcel_getspecific(mad2_recv_key), data, nb,
		      mad_send_SAFER, mad_receive_CHEAPER);
	   break;
	 }
    default: FAILURE("Unknown pack mode");
    }

  LOG_OUT();
}

void
old_mad_pack_short(madeleine_part where, short *data, size_t nb)
{
  LOG_IN();
  old_mad_pack_byte(where, (char *)data, nb*sizeof(short));
  LOG_OUT();
}

void
old_mad_unpack_short(madeleine_part where, short *data, size_t nb)
{
  LOG_IN();
  old_mad_unpack_byte(where, (char *)data, nb*sizeof(short));
  LOG_OUT();
}

void
old_mad_pack_int(madeleine_part where, int *data, size_t nb)
{
  LOG_IN();
  old_mad_pack_byte(where, (char *)data, nb*sizeof(int));
  LOG_OUT();
}

void
old_mad_unpack_int(madeleine_part where, int *data, size_t nb)
{
  LOG_IN();
  old_mad_unpack_byte(where, (char *)data, nb*sizeof(int));
  LOG_OUT();
}

void
old_mad_pack_long(madeleine_part where, long *data, size_t nb)
{
  LOG_IN();
  old_mad_pack_byte(where, (char *)data, nb*sizeof(long));
  LOG_OUT();
}

void
old_mad_unpack_long(madeleine_part where, long *data, size_t nb)
{
  LOG_IN();
  old_mad_unpack_byte(where, (char *)data, nb*sizeof(long));
  LOG_OUT();
}

void
old_mad_pack_float(madeleine_part where, float *data, size_t nb)
{
  LOG_IN();
  old_mad_pack_byte(where, (char *)data, nb*sizeof(float));
  LOG_OUT();
}

void
old_mad_unpack_float(madeleine_part where, float *data, size_t nb)
{
  LOG_IN();
  old_mad_unpack_byte(where, (char *)data, nb*sizeof(float));
  LOG_OUT();
}

void
old_mad_pack_double(madeleine_part where, double *data, size_t nb)
{
  LOG_IN();
  old_mad_pack_byte(where, (char *)data, nb*sizeof(double));
  LOG_OUT();
}

void
old_mad_unpack_double(madeleine_part where, double *data, size_t nb)
{
  LOG_IN();
  old_mad_unpack_byte(where, (char *)data, nb*sizeof(double));
  LOG_OUT();
}

void
old_mad_pack_pointer(madeleine_part where, pointer *data, size_t nb)
{
  LOG_IN();
  old_mad_pack_byte(where, (char *)data, nb*sizeof(pointer));
  LOG_OUT();
}

void
old_mad_unpack_pointer(madeleine_part where, pointer *data, size_t nb)
{
  LOG_IN();
  old_mad_unpack_byte(where, (char *)data, nb*sizeof(pointer));
  LOG_OUT();
}

void
old_mad_pack_str(madeleine_part where, char *data)
{
  int len = strlen(data);
  LOG_IN();
  mad_pack(marcel_getspecific(mad2_send_key), (char *)&len, sizeof(int),
	   mad_send_SAFER, mad_receive_EXPRESS);
  old_mad_pack_byte(where, data, len + 1);
  LOG_OUT();
}

void
old_mad_unpack_str(madeleine_part where, char *data)
{
  int len;
  LOG_IN();
  mad_unpack(marcel_getspecific(mad2_recv_key), (char *)&len, sizeof(int),
	     mad_send_SAFER, mad_receive_EXPRESS);
  old_mad_unpack_byte(where, data, len + 1);
  LOG_OUT();
}

p_mad_madeleine_t mad_get_madeleine(void)
{
  return main_madeleine;
}

#endif
