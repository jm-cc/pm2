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
 * leo_communication.c
 * ===================
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "leonie.h"

void
leo_send_int(p_ntbx_client_t client,
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
leo_receive_int(p_ntbx_client_t client)
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
leo_send_unsigned_int(p_ntbx_client_t    client,
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
leo_receive_unsigned_int(p_ntbx_client_t client)
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
leo_send_string(p_ntbx_client_t  client,
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
leo_receive_string(p_ntbx_client_t client)
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
