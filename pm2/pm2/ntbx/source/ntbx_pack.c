
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

/*
 * Integer
 * -------
 */
void
ntbx_pack_int(int                  data,
	      p_ntbx_pack_buffer_t pack_buffer)
{
  void *ptr = pack_buffer->buffer;
  int   i;
  
  LOG_IN();
  for (i = 0; i < NTBX_PACK_BUFFER_TAG_LEN; i++)
    {
      *(char*)ptr++ = 'i';
    }

  sprintf(ptr, "%*d", 
	  NTBX_PACK_BUFFER_LEN - NTBX_PACK_BUFFER_TAG_LEN - 1, data);
  LOG_OUT();
}

int
ntbx_unpack_int(p_ntbx_pack_buffer_t pack_buffer)
{
  void *ptr     = pack_buffer->buffer;
  char *end_ptr = NULL;
  int   data;
  int   i;
 
  LOG_IN();
  for (i = 0; i < NTBX_PACK_BUFFER_TAG_LEN; i++)
    {
      if (*(char*)ptr++ != 'i')
	FAILURE("synchronisation error");
    }

  data = (int)strtol(ptr, &end_ptr, 10);
  if (*end_ptr != '\0')
    FAILURE("synchronisation error");  
  LOG_OUT();

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
  int   i;
  
  LOG_IN();
  for (i = 0; i < NTBX_PACK_BUFFER_TAG_LEN; i++)
    {
      *(char*)ptr++ = 'l';
    }

  sprintf(ptr, "%*ld",
	  NTBX_PACK_BUFFER_LEN - NTBX_PACK_BUFFER_TAG_LEN - 1, data);
  LOG_OUT();
}

long
ntbx_unpack_long(p_ntbx_pack_buffer_t pack_buffer)
{
  void *ptr     = pack_buffer->buffer;
  char *end_ptr = NULL;
  long  data;
  int   i;
 
  LOG_IN();
  for (i = 0; i < NTBX_PACK_BUFFER_TAG_LEN; i++)
    {
      if (*(char*)ptr++ != 'l')
	FAILURE("synchronisation error");
    }

  data = strtol(ptr, &end_ptr, 10);
  if (*end_ptr != '\0')
    FAILURE("synchronisation error");  
  LOG_OUT();

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
  int   i;
  
  LOG_IN();
  for (i = 0; i < NTBX_PACK_BUFFER_TAG_LEN; i++)
    {
      *(char*)ptr++ = 'I';
    }

  sprintf(ptr, "%*u",
	  NTBX_PACK_BUFFER_LEN - NTBX_PACK_BUFFER_TAG_LEN - 1, data);
  LOG_OUT();
}

unsigned int
ntbx_unpack_unsigned_int(p_ntbx_pack_buffer_t pack_buffer)
{
  void         *ptr     = pack_buffer->buffer;
  char         *end_ptr = NULL;
  unsigned int  data;
  int           i;
 
  LOG_IN();
  for (i = 0; i < NTBX_PACK_BUFFER_TAG_LEN; i++)
    {
      if (*(char*)ptr++ != 'I')
	FAILURE("synchronisation error");
    }

  data = (unsigned int)strtoul(ptr, &end_ptr, 10);
  if (*end_ptr != '\0')
    FAILURE("synchronisation error");
  LOG_OUT();

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
  int   i;
  
  LOG_IN();
  for (i = 0; i < NTBX_PACK_BUFFER_TAG_LEN; i++)
    {
      *(char*)ptr++ = 'L';
    }

  sprintf(ptr, "%*ld",
	  NTBX_PACK_BUFFER_LEN - NTBX_PACK_BUFFER_TAG_LEN - 1, data);
  LOG_OUT();
}

unsigned long
ntbx_unpack_unsigned_long(p_ntbx_pack_buffer_t pack_buffer)
{
  void          *ptr = pack_buffer->buffer;
  char          *end_ptr = NULL;
  unsigned long  data;
  int            i;
 
  LOG_IN();
  for (i = 0; i < NTBX_PACK_BUFFER_TAG_LEN; i++)
    {
      if (*(char*)ptr++ != 'L')
	FAILURE("synchronisation error");
    }

  data = strtoul(ptr, &end_ptr, 10);
  if (*end_ptr != '\0')
    FAILURE("synchronisation error");  
  LOG_OUT();

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
  void *ptr    = pack_buffer->buffer;
  int   status = 0;
  int   i      = 0;
  
  LOG_IN();
  for (i = 0; i < NTBX_PACK_BUFFER_TAG_LEN; i++)
    {
      *(char*)ptr++ = 'D';
    }
  status =
    snprintf(ptr, NTBX_PACK_BUFFER_LEN - NTBX_PACK_BUFFER_TAG_LEN, "%*.*g",
	     NTBX_PACK_BUFFER_LEN - NTBX_PACK_BUFFER_TAG_LEN - 1,
	     NTBX_PACK_BUFFER_LEN - NTBX_PACK_BUFFER_TAG_LEN - 3, data);
  LOG_OUT();
}

int
ntbx_unpack_double(p_ntbx_pack_buffer_t pack_buffer)
{
  void   *ptr = pack_buffer->buffer;
  char   *end_ptr = NULL;
  double  data;
  int     i;
 
  LOG_IN();
  for (i = 0; i < NTBX_PACK_BUFFER_TAG_LEN; i++)
    {
      if (*(char*)ptr++ != 'D')
	FAILURE("synchronisation error");
    }

  data = strtod(ptr, &end_ptr);
  if (*end_ptr != '\0')
    FAILURE("synchronisation error");  
  LOG_OUT();

  return data;
}
