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
 * mad_ntbx_communication.c
 * ========================
 */ 
// #define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <madeleine.h>


/*
 * Variables
 * ---------
 */
static size_t           mad_print_buffer_size =    0;
static char            *mad_print_buffer      = NULL;
static p_ntbx_client_t  mad_leo_client        = NULL;

/*
 * Functions
 * ---------
 */
void
mad_ntbx_send_int(p_ntbx_client_t client,
		  const int       data)
{
  int                status = ntbx_failure;
  ntbx_pack_buffer_t buffer;

  LOG_IN();
  ntbx_pack_int(data, &buffer);
  status = ntbx_btcp_write_pack_buffer(client, &buffer);
  
  if (status == ntbx_failure)
    FAILURE("control link failure");
  
  LOG_OUT();
}

int
mad_ntbx_receive_int(p_ntbx_client_t client)
{
  int                status = ntbx_failure;
  int                data   = 0;
  ntbx_pack_buffer_t buffer;

  LOG_IN();
  status = ntbx_btcp_read_pack_buffer(client, &buffer);

  if (status == ntbx_failure)
    FAILURE("control link failure");

  data = ntbx_unpack_int(&buffer);
  LOG_OUT();

  return data;
}

void
mad_ntbx_send_unsigned_int(p_ntbx_client_t    client,
			   const unsigned int data)
{
  int                status = ntbx_failure;
  ntbx_pack_buffer_t buffer;

  LOG_IN();
  ntbx_pack_unsigned_int(data, &buffer);
  status = ntbx_btcp_write_pack_buffer(client, &buffer);
  
  if (status == ntbx_failure)
    FAILURE("control link failure");
  
  LOG_OUT();
}

unsigned int
mad_ntbx_receive_unsigned_int(p_ntbx_client_t client)
{
  int                status = ntbx_failure;
  unsigned int       data   = 0;
  ntbx_pack_buffer_t buffer;

  LOG_IN();
  status = ntbx_btcp_read_pack_buffer(client, &buffer);

  if (status == ntbx_failure)
    FAILURE("control link failure");

  data = ntbx_unpack_unsigned_int(&buffer);
  LOG_OUT();

  return data;
}

void
mad_ntbx_send_string(p_ntbx_client_t  client,
		const char      *string)
{
  int status = ntbx_failure;

  LOG_IN();
  status = ntbx_btcp_write_string(client, string);
  
  if (status == ntbx_failure)
    FAILURE("control link failure");
  
  LOG_OUT();
}

char *
mad_ntbx_receive_string(p_ntbx_client_t client)
{
  int   status = ntbx_failure;
  char *result = NULL;

  LOG_IN();
  status = ntbx_btcp_read_string(client, &result);

  if (status == ntbx_failure)
    FAILURE("control link failure");

  LOG_OUT();

  return result;
}

void
mad_leonie_print_init(p_mad_madeleine_t madeleine,
		   int                 argc TBX_UNUSED,
		   char              **argv TBX_UNUSED)
{
  p_mad_session_t session = NULL;
  p_ntbx_client_t client  = NULL;

  LOG_IN();
  session = madeleine->session;
  client  = session->leonie_link;
  TBX_LOCK_SHARED(client);
  mad_leo_client        = client;
  mad_print_buffer_size = 16;
  mad_print_buffer      = TBX_MALLOC(mad_print_buffer_size);
  TBX_UNLOCK_SHARED(client);
  LOG_OUT();
}

void
mad_leonie_print(char *fmt, ...)
{
  va_list ap;
  int     len = 0;

  LOG_IN();
  if (!mad_leo_client)
    FAILURE("leonie print module uninitialized");
  
  TBX_LOCK_SHARED(mad_leo_client);
  len = strlen(fmt);

  if (mad_print_buffer_size < len)
    {
      mad_print_buffer_size = 1 + 2 * len;
      mad_print_buffer      =
	TBX_REALLOC(mad_print_buffer, mad_print_buffer_size);
    }

  while (1)
    {
      int status;
      
      va_start(ap, fmt);
      status = vsnprintf(mad_print_buffer, mad_print_buffer_size, fmt, ap);
      va_end(ap);
      
      if (status != -1)
	break;

      mad_print_buffer_size *= 2;
      mad_print_buffer       =
	TBX_REALLOC(mad_print_buffer, mad_print_buffer_size);
    }

  mad_ntbx_send_int(mad_leo_client, mad_leo_command_print);
  mad_ntbx_send_string(mad_leo_client, mad_print_buffer);

  TBX_UNLOCK_SHARED(mad_leo_client);
  LOG_OUT();
}
