
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

/*
 * Definitions
 * -----------
 */
#define ALIGNMASK (MAD_ALIGNMENT - 1)


/*
 * Static variables
 * ----------------
 */
static p_mad_madeleine_t pm2_mad_madeleine = NULL;

/*
 * Exported variables
 * ------------------
 */
marcel_key_t pm2_mad_send_key = -1;
marcel_key_t pm2_mad_recv_key = -1;

/*
 * Functions
 * ---------
 */
#ifdef MAD3
int
mad_can_send_to_self()
{
  return 0;
}

void
pm2_begin_packing(p_mad_channel_t channel,
		  int             global_rank)
{
  p_ntbx_process_container_t pc         = NULL;
  ntbx_process_lrank_t       local_rank =   -1;
  p_mad_connection_t         out        = NULL;
  
  LOG_IN();
  pc         = channel->pc;
  local_rank = ntbx_pc_global_to_local(pc, global_rank);

  if (local_rank == -1)
    TBX_FAILURE("connection unavailable");
  
  out = mad_begin_packing(channel, local_rank);
  if (!out)
    TBX_FAILURE("invalid connection");
  marcel_setspecific(pm2_mad_send_key, out);
  LOG_OUT();
}
#endif // MADIII


void
pm2_mad_init(p_mad_madeleine_t madeleine)
{
  LOG_IN();
  pm2_mad_madeleine = madeleine;

  marcel_key_create(&pm2_mad_send_key, NULL);
  marcel_key_create(&pm2_mad_recv_key, NULL);
  LOG_OUT();
}

void
pm2_end_packing(void)
{
  p_mad_connection_t out = NULL;

  LOG_IN();
  out = marcel_getspecific(pm2_mad_send_key);
  mad_end_packing(out);
  LOG_OUT();
}

void
pm2_begin_unpacking(p_mad_channel_t channel)
{
  p_mad_connection_t in = NULL;
  
  LOG_IN();
  in = mad_begin_unpacking(channel);
  marcel_setspecific(pm2_mad_recv_key, in);
  LOG_OUT();
}

void
pm2_end_unpacking(void)
{
  p_mad_connection_t in = NULL;
  
  LOG_IN();
  in = marcel_getspecific(pm2_mad_recv_key);
  mad_end_unpacking(in);
  LOG_OUT();
}

void
pm2_pack_byte(mad_send_mode_t     send_mode,
	      mad_receive_mode_t  receive_mode,
	      void               *data,
	      size_t              len)
{
  p_mad_connection_t out = NULL;

  LOG_IN();
  out = marcel_getspecific(pm2_mad_send_key);
  mad_pack(out, data, len, send_mode, receive_mode);
  LOG_OUT();
}

void
pm2_unpack_byte(mad_send_mode_t     send_mode,
		mad_receive_mode_t  receive_mode,
		void               *data,
		size_t              len)
{
  p_mad_connection_t in = NULL;

  LOG_IN();
  in = marcel_getspecific(pm2_mad_recv_key);
  mad_unpack(in, data, len, send_mode, receive_mode);
  LOG_OUT();
}

void
pm2_pack_short(mad_send_mode_t     send_mode,
	       mad_receive_mode_t  receive_mode,
	       short              *data,
	       size_t              len)
{
  LOG_IN();
  pm2_pack_byte(send_mode, receive_mode, data, len * sizeof(short));
  LOG_OUT();
}

void
pm2_unpack_short(mad_send_mode_t     send_mode,
		 mad_receive_mode_t  receive_mode,
		 short              *data,
		 size_t              len)
{
  LOG_IN();
  pm2_unpack_byte(send_mode, receive_mode, data, len * sizeof(short));
  LOG_OUT();
}

void
pm2_pack_int(mad_send_mode_t     send_mode,
	     mad_receive_mode_t  receive_mode,
	     int                *data,
	     size_t              len)
{
  LOG_IN();
  pm2_pack_byte(send_mode, receive_mode, data, len * sizeof(int));
  LOG_OUT();
}

void
pm2_unpack_int(mad_send_mode_t     send_mode,
	       mad_receive_mode_t  receive_mode,
	       int                *data,
	       size_t              len)
{
  LOG_IN();
  pm2_unpack_byte(send_mode, receive_mode, data, len * sizeof(int));
  LOG_OUT();
}

void
pm2_pack_long(mad_send_mode_t     send_mode,
	      mad_receive_mode_t  receive_mode,
	      long               *data,
	      size_t              len)
{
  LOG_IN();
  pm2_pack_byte(send_mode, receive_mode, data, len * sizeof(long));
  LOG_OUT();
}

void
pm2_unpack_long(mad_send_mode_t     send_mode,
		mad_receive_mode_t  receive_mode,
		long               *data,
		size_t              len)
{
  LOG_IN();
  pm2_unpack_byte(send_mode, receive_mode, data, len * sizeof(long));
  LOG_OUT();
}

void
pm2_pack_float(mad_send_mode_t     send_mode,
	       mad_receive_mode_t  receive_mode,
	       float              *data,
	       size_t              len)
{
  LOG_IN();
  pm2_pack_byte(send_mode, receive_mode, data, len * sizeof(float));
  LOG_OUT();
}

void
pm2_unpack_float(mad_send_mode_t     send_mode,
		 mad_receive_mode_t  receive_mode,
		 float              *data,
		 size_t              len)
{
  LOG_IN();
  mad_unpack(marcel_getspecific(pm2_mad_recv_key),
	   data,
	   len*sizeof(float),
	   send_mode,
	   receive_mode);
  LOG_OUT();
}

void
pm2_pack_double(mad_send_mode_t     send_mode,
		mad_receive_mode_t  receive_mode,
		double             *data,
		size_t              len)
{
  LOG_IN();
  pm2_pack_byte(send_mode, receive_mode, data, len * sizeof(double));
  LOG_OUT();
}

void
pm2_unpack_double(mad_send_mode_t     send_mode,
		  mad_receive_mode_t  receive_mode,
		  double             *data,
		  size_t              len)
{
  LOG_IN();
  pm2_unpack_byte(send_mode, receive_mode, data, len * sizeof(double));
  LOG_OUT();
}

void
pm2_pack_pointer(mad_send_mode_t     send_mode,
		 mad_receive_mode_t  receive_mode,
		 pointer            *data,
		 size_t              len)
{
  LOG_IN();
  pm2_pack_byte(send_mode, receive_mode, data, len * sizeof(pointer));
  LOG_OUT();
}

void
pm2_unpack_pointer(mad_send_mode_t     send_mode,
		   mad_receive_mode_t  receive_mode,
		   pointer            *data,
		   size_t              len)
{
  LOG_IN();
  pm2_unpack_byte(send_mode, receive_mode, data, len * sizeof(pointer));
  LOG_OUT();
}

void
pm2_pack_str(mad_send_mode_t     send_mode,
	     mad_receive_mode_t  receive_mode,
	     char               *data)
{
  size_t len = 0;
  
  LOG_IN();
  len = strlen(data);

  if (send_mode == mad_send_LATER)
    TBX_FAILURE("unimplemented feature");

  pm2_pack_byte(send_mode, mad_receive_EXPRESS, &len, sizeof(size_t));
  pm2_pack_byte(send_mode, receive_mode, data, len + 1);
  LOG_OUT();
}

void
pm2_unpack_str(mad_send_mode_t     send_mode,
	       mad_receive_mode_t  receive_mode,
	       char               *data)
{
  size_t len = 0;

  LOG_IN();
  if (send_mode == mad_send_LATER)
    TBX_FAILURE("unimplemented feature");
  
  pm2_unpack_byte(send_mode, mad_receive_EXPRESS, &len, sizeof(int));
  pm2_unpack_byte(send_mode, receive_mode, data, len + 1);
  LOG_OUT();
}

void
old_mad_pack_byte(madeleine_part  where, 
		  void           *data,
		  size_t          len)
{
  LOG_IN();
  switch(where)
    {
    case MAD_IN_HEADER :
      {
	pm2_pack_byte(mad_send_SAFER, mad_receive_EXPRESS, data, len);
	break;
      }
    case MAD_IN_PLACE :
      {
	pm2_pack_byte(mad_send_CHEAPER, mad_receive_CHEAPER, data, len);
	break;
      }
    case MAD_BY_COPY :
      {
	pm2_pack_byte(mad_send_SAFER, mad_receive_CHEAPER, data, len);
	break;
      }
    default: 
      TBX_FAILURE("Unknown pack mode");
    }
  LOG_OUT();
}

void
old_mad_unpack_byte(madeleine_part  where,
		    void           *data,
		    size_t          len)
{
  LOG_IN();
  switch (where)
    {
    case MAD_IN_HEADER :
      {
	pm2_unpack_byte(mad_send_SAFER, mad_receive_EXPRESS, data, len);
	break;
      }
    case MAD_IN_PLACE :
      {
	pm2_unpack_byte(mad_send_CHEAPER, mad_receive_CHEAPER, data, len);
	break;
      }
    case MAD_BY_COPY :
      {
	pm2_unpack_byte(mad_send_SAFER, mad_receive_CHEAPER, data, len);
	break;
      }
    default:
      TBX_FAILURE("Unknown pack mode");
    }
  LOG_OUT();
}

void
old_mad_pack_short(madeleine_part  where, 
		   short          *data,
		   size_t          len)
{
  LOG_IN();
  old_mad_pack_byte(where, data, len * sizeof(short));
  LOG_OUT();
}

void
old_mad_unpack_short(madeleine_part  where,
		     short          *data,
		     size_t          len)
{
  LOG_IN();
  old_mad_unpack_byte(where, data, len * sizeof(short));
  LOG_OUT();
}

void
old_mad_pack_int(madeleine_part  where,
		 int            *data,
		 size_t          len)
{
  LOG_IN();
  old_mad_pack_byte(where, data, len * sizeof(int));
  LOG_OUT();
}

void
old_mad_unpack_int(madeleine_part  where,
		   int            *data,
		   size_t          len)
{
  LOG_IN();
  old_mad_unpack_byte(where, data, len * sizeof(int));
  LOG_OUT();
}

void
old_mad_pack_long(madeleine_part  where,
		  long           *data,
		  size_t          len)
{
  LOG_IN();
  old_mad_pack_byte(where, data, len * sizeof(long));
  LOG_OUT();
}

void
old_mad_unpack_long(madeleine_part  where,
		    long           *data,
		    size_t          len)
{
  LOG_IN();
  old_mad_unpack_byte(where, data, len * sizeof(long));
  LOG_OUT();
}

void
old_mad_pack_float(madeleine_part  where,
		   float          *data,
		   size_t          len)
{
  LOG_IN();
  old_mad_pack_byte(where, data, len * sizeof(float));
  LOG_OUT();
}

void
old_mad_unpack_float(madeleine_part  where,
		     float          *data,
		     size_t          len)
{
  LOG_IN();
  old_mad_unpack_byte(where, data, len * sizeof(float));
  LOG_OUT();
}

void
old_mad_pack_double(madeleine_part  where,
		    double         *data,
		    size_t          len)
{
  LOG_IN();
  old_mad_pack_byte(where, data, len * sizeof(double));
  LOG_OUT();
}

void
old_mad_unpack_double(madeleine_part  where,
		      double         *data,
		      size_t          len)
{
  LOG_IN();
  old_mad_unpack_byte(where, data, len * sizeof(double));
  LOG_OUT();
}

void
old_mad_pack_pointer(madeleine_part  where,
		     pointer        *data,
		     size_t          len)
{
  LOG_IN();
  old_mad_pack_byte(where, data, len * sizeof(pointer));
  LOG_OUT();
}

void
old_mad_unpack_pointer(madeleine_part  where,
		       pointer        *data,
		       size_t          len)
{
  LOG_IN();
  old_mad_unpack_byte(where, data, len * sizeof(pointer));
  LOG_OUT();
}

void
old_mad_pack_str(madeleine_part  where,
		 char           *data)
{
  int len = 0;

  LOG_IN();  
  len = strlen(data);

  old_mad_pack_int(MAD_IN_HEADER, &len, 1);
  old_mad_pack_byte(where, data, len + 1);
  LOG_OUT();
}

void
old_mad_unpack_str(madeleine_part  where,
		   char           *data)
{
  int len = 0;

  LOG_IN();
  old_mad_unpack_int(MAD_IN_HEADER, &len, 1);
  old_mad_unpack_byte(where, data, len + 1);
  LOG_OUT();
}

