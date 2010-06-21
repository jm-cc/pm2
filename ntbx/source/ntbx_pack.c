
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

/*
 * ntbx_pack.c
 * -----------
 */
/* #define DEBUG */
#include "ntbx.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

/*
 * Integer
 * -------
 */
void
ntbx_pack_int(int                  data,
	      p_ntbx_pack_buffer_t pack_buffer)
{
  void *ptr = pack_buffer->buffer;
  int   i   = 0;

  PM2_LOG_IN();
  for (i = 0; i < NTBX_PACK_BUFFER_TAG_LEN; i++)
    {
      *(char*)ptr++ = 'i';
    }

  sprintf(ptr, "%*d-", NTBX_PACK_INT_LEN, data);
  PM2_LOG_OUT();
}

int
ntbx_unpack_int(p_ntbx_pack_buffer_t pack_buffer)
{
  void *ptr     = pack_buffer->buffer;
  char *end_ptr = NULL;
  int   data    = 0;
  int   i       = 0;

  PM2_LOG_IN();
  for (i = 0; i < NTBX_PACK_BUFFER_TAG_LEN; i++)
    {
      if (*(char*)ptr++ != 'i')
	TBX_FAILURE("synchronisation error");
    }

  data = (int)strtol(ptr, &end_ptr, 10);
  if (*end_ptr != '-')
    TBX_FAILURE("synchronisation error");
  PM2_LOG_OUT();

  return data;
}

/*
 * Long integer
 * ------------
 */
void
ntbx_pack_long(long                 data,
	       p_ntbx_pack_buffer_t pack_buffer)
{
  void *ptr = pack_buffer->buffer;
  int   i   = 0;

  PM2_LOG_IN();
  for (i = 0; i < NTBX_PACK_BUFFER_TAG_LEN; i++)
    {
      *(char*)ptr++ = 'l';
    }

  sprintf(ptr, "%*ld-", NTBX_PACK_LONG_LEN, data);
  PM2_LOG_OUT();
}

long
ntbx_unpack_long(p_ntbx_pack_buffer_t pack_buffer)
{
  void *ptr     = pack_buffer->buffer;
  char *end_ptr = NULL;
  long  data    = 0;
  int   i       = 0;

  PM2_LOG_IN();
  for (i = 0; i < NTBX_PACK_BUFFER_TAG_LEN; i++)
    {
      if (*(char*)ptr++ != 'l')
	TBX_FAILURE("synchronisation error");
    }

  data = strtol(ptr, &end_ptr, 10);
  if (*end_ptr != '-')
    TBX_FAILURE("synchronisation error");
  PM2_LOG_OUT();

  return data;
}

/*
 * Unsigned integer
 * ----------------
 */
void
ntbx_pack_unsigned_int(unsigned int         data,
		       p_ntbx_pack_buffer_t pack_buffer)
{
  void *ptr = pack_buffer->buffer;
  int   i   = 0;

  PM2_LOG_IN();
  for (i = 0; i < NTBX_PACK_BUFFER_TAG_LEN; i++)
    {
      *(char*)ptr++ = 'I';
    }

  sprintf(ptr, "%*u-", NTBX_PACK_UINT_LEN, data);
  PM2_LOG_OUT();
}

unsigned int
ntbx_unpack_unsigned_int(p_ntbx_pack_buffer_t pack_buffer)
{
  void         *ptr     = pack_buffer->buffer;
  char         *end_ptr = NULL;
  unsigned int  data    = 0;
  int           i       = 0;

  PM2_LOG_IN();
  for (i = 0; i < NTBX_PACK_BUFFER_TAG_LEN; i++)
    {
      if (*(char*)ptr++ != 'I')
	TBX_FAILURE("synchronisation error");
    }

  data = (unsigned int)strtoul(ptr, &end_ptr, 10);
  if (*end_ptr != '-')
    TBX_FAILURE("synchronisation error");
  PM2_LOG_OUT();

  return data;
}

/*
 * Unsigned long integer
 * ---------------------
 */
void
ntbx_pack_unsigned_long(unsigned long        data,
			p_ntbx_pack_buffer_t pack_buffer)
{
  void *ptr = pack_buffer->buffer;
  int   i   = 0;

  PM2_LOG_IN();
  for (i = 0; i < NTBX_PACK_BUFFER_TAG_LEN; i++)
    {
      *(char*)ptr++ = 'L';
    }

  sprintf(ptr, "%*ld-", NTBX_PACK_ULONG_LEN, data);
  PM2_LOG_OUT();
}

unsigned long
ntbx_unpack_unsigned_long(p_ntbx_pack_buffer_t pack_buffer)
{
  void          *ptr     = pack_buffer->buffer;
  char          *end_ptr = NULL;
  unsigned long  data    = 0;
  int            i       = 0;

  PM2_LOG_IN();
  for (i = 0; i < NTBX_PACK_BUFFER_TAG_LEN; i++)
    {
      if (*(char*)ptr++ != 'L')
	TBX_FAILURE("synchronisation error");
    }

  data = strtoul(ptr, &end_ptr, 10);
  if (*end_ptr != '-')
    TBX_FAILURE("synchronisation error");
  PM2_LOG_OUT();

  return data;
}

/*
 * Double
 * ------
 */
void
ntbx_pack_double(double               data,
		 p_ntbx_pack_buffer_t pack_buffer)
{
  void   *ptr      = pack_buffer->buffer;
  int     i        = 0;
  int     exponent = 0;
  double  mantissa = 0.0;

  PM2_LOG_IN();
  for (i = 0; i < NTBX_PACK_BUFFER_TAG_LEN; i++)
    {
      *(char*)ptr++ = 'D';
    }

  mantissa = frexp(data, &exponent);
  sprintf(ptr, "%*d-", NTBX_PACK_INT_LEN, exponent);

  ptr += NTBX_PACK_INT_LEN + 1;

  sprintf(ptr, "%*.*f-", NTBX_PACK_MANTISSA_LEN,
	  NTBX_PACK_MANTISSA_LEN - 3, mantissa);
  PM2_LOG_OUT();
}

double
ntbx_unpack_double(p_ntbx_pack_buffer_t pack_buffer)
{
  void   *ptr      = pack_buffer->buffer;
  char   *end_ptr  = NULL;
  int     i        = 0;
  int     exponent = 0;
  double  mantissa = 0.0;
  double  data     = 0.0;

  PM2_LOG_IN();
  for (i = 0; i < NTBX_PACK_BUFFER_TAG_LEN; i++)
    {
      if (*(char*)ptr++ != 'D')
	TBX_FAILURE("synchronisation error");
    }

  exponent = (int)strtol(ptr, &end_ptr, 10);
  if (*end_ptr != '-')
    TBX_FAILURE("synchronisation error");

  end_ptr++;

  mantissa = strtod(end_ptr, &end_ptr);
  if (*end_ptr != '-')
    TBX_FAILURE("synchronisation error");

  data = ldexp(mantissa, exponent);
  PM2_LOG_OUT();

  return data;
}
