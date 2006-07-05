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
 * Mad_ntbx_communication.c
 * ========================
 */
// #define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include "madeleine.h"

/*
 * Variables
 * ---------
 */
static size_t           mad_print_buffer_size =    0;
static char            *mad_print_buffer      = NULL;
static p_ntbx_client_t  mad_leo_client        = NULL;

#ifdef MARCEL
static marcel_sem_t    *barrier_passed        = NULL;
#endif /* MARCEL */

static TBX_CRITICAL_SECTION(mad_ntbx_cs_print);
static TBX_CRITICAL_SECTION(mad_ntbx_cs_barrier);

/*
 * Ntbx wrappers functions
 * -----------------------
 */
void
mad_ntbx_send_int(p_ntbx_client_t client,
		  const int       data)
{
  int                status = ntbx_failure;
  ntbx_pack_buffer_t buffer;

  LOG_IN();
  memset(&buffer, 0, sizeof(ntbx_pack_buffer_t));
  ntbx_pack_int(data, &buffer);
  status = ntbx_tcp_write_pack_buffer(client, &buffer);

  if (status == ntbx_failure)
    TBX_FAILURE("control link failure");

  LOG_OUT();
}

int
mad_ntbx_receive_int(p_ntbx_client_t client)
{
  int                status = ntbx_failure;
  int                data   = 0;
  ntbx_pack_buffer_t buffer;

  LOG_IN();
  memset(&buffer, 0, sizeof(ntbx_pack_buffer_t));
  status = ntbx_tcp_read_pack_buffer(client, &buffer);

  if (status == ntbx_failure)
   TBX_FAILURE("control link failure");

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
  memset(&buffer, 0, sizeof(ntbx_pack_buffer_t));
  ntbx_pack_unsigned_int(data, &buffer);
  status = ntbx_tcp_write_pack_buffer(client, &buffer);

  if (status == ntbx_failure)
    TBX_FAILURE("control link failure");

  LOG_OUT();
}

unsigned int
mad_ntbx_receive_unsigned_int(p_ntbx_client_t client)
{
  int                status = ntbx_failure;
  unsigned int       data   = 0;
  ntbx_pack_buffer_t buffer;

  LOG_IN();
  memset(&buffer, 0, sizeof(ntbx_pack_buffer_t));
  status = ntbx_tcp_read_pack_buffer(client, &buffer);

  if (status == ntbx_failure)
    TBX_FAILURE("control link failure");

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
  status = ntbx_tcp_write_string(client, string);

  if (status == ntbx_failure)
    TBX_FAILURE("control link failure");

  LOG_OUT();
}

char *
mad_ntbx_receive_string(p_ntbx_client_t client)
{
  int   status = ntbx_failure;
  char *result = NULL;

  LOG_IN();
  status = ntbx_tcp_read_string(client, &result);

  if (status == ntbx_failure)
    TBX_FAILURE("control link failure");

  LOG_OUT();

  return result;
}

/*
 * Leonie communication functions
 * -----------------------
 */
void
mad_leonie_send_int(const int data)
{
  int                status = ntbx_failure;
  ntbx_pack_buffer_t buffer;

  LOG_IN();
  memset(&buffer, 0, sizeof(ntbx_pack_buffer_t));
  if (!mad_leo_client)
    TBX_FAILURE("leonie command module uninitialized");

  ntbx_pack_int(data, &buffer);
  TBX_LOCK_SHARED(mad_leo_client);
  status = ntbx_tcp_write_pack_buffer(mad_leo_client, &buffer);
  TBX_UNLOCK_SHARED(mad_leo_client);

  if (status == ntbx_failure)
    TBX_FAILURE("control link failure");

  LOG_OUT();
}

int
mad_leonie_receive_int(void)
{
  int                status = ntbx_failure;
  int                data   = 0;
  ntbx_pack_buffer_t buffer;

  LOG_IN();
  memset(&buffer, 0, sizeof(ntbx_pack_buffer_t));
  if (!mad_leo_client)
    TBX_FAILURE("leonie command module uninitialized");

  TBX_LOCK_SHARED(mad_leo_client);
  status = ntbx_tcp_read_pack_buffer(mad_leo_client, &buffer);
  TBX_UNLOCK_SHARED(mad_leo_client);

  if (status == ntbx_failure)
    TBX_FAILURE("control link failure");

  data = ntbx_unpack_int(&buffer);
  LOG_OUT();

  return data;
}

void
mad_leonie_send_unsigned_int(const unsigned int data)
{
  int                status = ntbx_failure;
  ntbx_pack_buffer_t buffer;

  LOG_IN();
  memset(&buffer, 0, sizeof(ntbx_pack_buffer_t));
  if (!mad_leo_client)
    TBX_FAILURE("leonie command module uninitialized");

  ntbx_pack_unsigned_int(data, &buffer);
  TBX_LOCK_SHARED(mad_leo_client);
  status = ntbx_tcp_write_pack_buffer(mad_leo_client, &buffer);
  TBX_UNLOCK_SHARED(mad_leo_client);

  if (status == ntbx_failure)
    TBX_FAILURE("control link failure");

  LOG_OUT();
}

unsigned int
mad_leonie_receive_unsigned_int(void)
{
  int                status = ntbx_failure;
  unsigned int       data   = 0;
  ntbx_pack_buffer_t buffer;

  LOG_IN();
  memset(&buffer, 0, sizeof(ntbx_pack_buffer_t));
  if (!mad_leo_client)
    TBX_FAILURE("leonie command module uninitialized");

  TBX_LOCK_SHARED(mad_leo_client);
  status = ntbx_tcp_read_pack_buffer(mad_leo_client, &buffer);
  TBX_UNLOCK_SHARED(mad_leo_client);

  if (status == ntbx_failure)
    TBX_FAILURE("control link failure");

  data = ntbx_unpack_unsigned_int(&buffer);
  LOG_OUT();

  return data;
}

void
mad_leonie_send_string(const char *string)
{
  int status = ntbx_failure;

  LOG_IN();
  if (!mad_leo_client)
    TBX_FAILURE("leonie command module uninitialized");

  TBX_LOCK_SHARED(mad_leo_client);
  status = ntbx_tcp_write_string(mad_leo_client, string);
  TBX_UNLOCK_SHARED(mad_leo_client);

  if (status == ntbx_failure)
    TBX_FAILURE("control link failure");

  LOG_OUT();
}

char *
mad_leonie_receive_string(void)
{
  int   status = ntbx_failure;
  char *result = NULL;

  LOG_IN();
  if (!mad_leo_client)
    TBX_FAILURE("leonie command module uninitialized");

  TBX_LOCK_SHARED(mad_leo_client);
  status = ntbx_tcp_read_string(mad_leo_client, &result);
  TBX_UNLOCK_SHARED(mad_leo_client);

  if (status == ntbx_failure)
    TBX_FAILURE("control link failure");

  LOG_OUT();

  return result;
}

// ---

void
mad_leonie_send_int_nolock(const int data)
{
  int                status = ntbx_failure;
  ntbx_pack_buffer_t buffer;

  LOG_IN();
  memset(&buffer, 0, sizeof(ntbx_pack_buffer_t));
  if (!mad_leo_client)
    TBX_FAILURE("leonie command module uninitialized");

  ntbx_pack_int(data, &buffer);
  status = ntbx_tcp_write_pack_buffer(mad_leo_client, &buffer);

  if (status == ntbx_failure)
    TBX_FAILURE("control link failure");

  LOG_OUT();
}

int
mad_leonie_receive_int_nolock(void)
{
  int                status = ntbx_failure;
  int                data   = 0;
  ntbx_pack_buffer_t buffer;

  LOG_IN();
  memset(&buffer, 0, sizeof(ntbx_pack_buffer_t));
  if (!mad_leo_client)
    TBX_FAILURE("leonie command module uninitialized");

  status = ntbx_tcp_read_pack_buffer(mad_leo_client, &buffer);

  if (status == ntbx_failure)
    TBX_FAILURE("control link failure");

  data = ntbx_unpack_int(&buffer);
  LOG_OUT();

  return data;
}

void
mad_leonie_send_unsigned_int_nolock(const unsigned int data)
{
  int                status = ntbx_failure;
  ntbx_pack_buffer_t buffer;

  LOG_IN();
  memset(&buffer, 0, sizeof(ntbx_pack_buffer_t));
  if (!mad_leo_client)
    TBX_FAILURE("leonie command module uninitialized");

  ntbx_pack_unsigned_int(data, &buffer);
  status = ntbx_tcp_write_pack_buffer(mad_leo_client, &buffer);

  if (status == ntbx_failure)
    TBX_FAILURE("control link failure");

  LOG_OUT();
}

unsigned int
mad_leonie_receive_unsigned_int_nolock(void)
{
  int                status = ntbx_failure;
  unsigned int       data   = 0;
  ntbx_pack_buffer_t buffer;

  LOG_IN();
  memset(&buffer, 0, sizeof(ntbx_pack_buffer_t));
  if (!mad_leo_client)
    TBX_FAILURE("leonie command module uninitialized");

  status = ntbx_tcp_read_pack_buffer(mad_leo_client, &buffer);

  if (status == ntbx_failure)
    TBX_FAILURE("control link failure");

  data = ntbx_unpack_unsigned_int(&buffer);
  LOG_OUT();

  return data;
}

void
mad_leonie_send_string_nolock(const char *string)
{
  int status = ntbx_failure;

  LOG_IN();
  if (!mad_leo_client)
    TBX_FAILURE("leonie command module uninitialized");

  status = ntbx_tcp_write_string(mad_leo_client, string);

  if (status == ntbx_failure)
    TBX_FAILURE("control link failure");

  LOG_OUT();
}

char *
mad_leonie_receive_string_nolock(void)
{
  int   status = ntbx_failure;
  char *result = NULL;

  LOG_IN();
  if (!mad_leo_client)
    TBX_FAILURE("leonie command module uninitialized");

  status = ntbx_tcp_read_string(mad_leo_client, &result);

  if (status == ntbx_failure)
    TBX_FAILURE("control link failure");

  LOG_OUT();

  return result;
}



// ---

void
mad_leonie_command_init(p_mad_madeleine_t   madeleine,
			int                 argc TBX_UNUSED,
			char              **argv TBX_UNUSED)
{
  p_mad_session_t  session  = NULL;
  p_mad_settings_t settings = NULL;

  LOG_IN();
  session        = madeleine->session;
  settings       = madeleine->settings;
  mad_leo_client = session->leonie_link;

  TBX_LOCK_SHARED(mad_leo_client);
  mad_print_buffer_size = 16;
  mad_print_buffer      = TBX_MALLOC(mad_print_buffer_size);
  TBX_UNLOCK_SHARED(mad_leo_client);
  LOG_OUT();
}

void
mad_leonie_command_exit(p_mad_madeleine_t madeleine TBX_UNUSED)
{
  LOG_IN();
  TBX_FREE(mad_print_buffer);
  LOG_OUT();
}

void
mad_leonie_print(char *fmt, ...)
{
  va_list ap;
  int     len = 0;

  LOG_IN();
  TBX_CRITICAL_SECTION_ENTER(mad_ntbx_cs_print);
  if (!mad_leo_client)
    TBX_FAILURE("leonie command module uninitialized");

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

  TBX_LOCK_SHARED(mad_leo_client);
  mad_leonie_send_int_nolock(mad_leo_command_print);
  mad_leonie_send_int_nolock(1); // the string will be printed with a newline character
  mad_leonie_send_string_nolock(mad_print_buffer);
  TBX_UNLOCK_SHARED(mad_leo_client);
  TBX_CRITICAL_SECTION_LEAVE(mad_ntbx_cs_print);
  LOG_OUT();
}

void
mad_leonie_print_without_nl(char *fmt, ...)
{
  va_list ap;
  int     len = 0;

  LOG_IN();
  TBX_CRITICAL_SECTION_ENTER(mad_ntbx_cs_print);
  if (!mad_leo_client)
    TBX_FAILURE("leonie command module uninitialized");

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

  TBX_LOCK_SHARED(mad_leo_client);
  mad_leonie_send_int_nolock(mad_leo_command_print);
  mad_leonie_send_int_nolock(0); // the string will be printed without a newline character
  mad_leonie_send_string_nolock(mad_print_buffer);
  TBX_UNLOCK_SHARED(mad_leo_client);
  TBX_CRITICAL_SECTION_LEAVE(mad_ntbx_cs_print);
  LOG_OUT();
}

void
mad_leonie_barrier(void)
{
  int result = 0;

  LOG_IN();
  TBX_CRITICAL_SECTION_ENTER(mad_ntbx_cs_barrier);
  if (!mad_leo_client)
    TBX_FAILURE("leonie command module uninitialized");

  TBX_LOCK_SHARED(mad_leo_client);
  mad_ntbx_send_int(mad_leo_client, mad_leo_command_barrier);
  TBX_UNLOCK_SHARED(mad_leo_client);

#ifdef MARCEL
  if (barrier_passed) {
    DISP("marcel_sem_P");
    marcel_sem_P(barrier_passed);
    goto end;
  }
#endif /* MARCEL */
  TBX_LOCK_SHARED(mad_leo_client);
  result = mad_ntbx_receive_int(mad_leo_client);
  if (result != mad_leo_command_barrier_passed)
    TBX_FAILURE("synchronization error");

  TBX_UNLOCK_SHARED(mad_leo_client);
#ifdef MARCEL
  end:
#endif /* MARCEL */
  TBX_CRITICAL_SECTION_LEAVE(mad_ntbx_cs_barrier);
  LOG_OUT();
}

void
mad_leonie_beat(void)
{
  int result = 0;

  LOG_IN();
  TBX_CRITICAL_SECTION_ENTER(mad_ntbx_cs_barrier);
  if (!mad_leo_client)
    TBX_FAILURE("leonie command module uninitialized");

  TBX_LOCK_SHARED(mad_leo_client);
  mad_ntbx_send_int(mad_leo_client, mad_leo_command_beat);
  result = mad_ntbx_receive_int(mad_leo_client);
  if (result != mad_leo_command_beat_ack)
    TBX_FAILURE("leonie heartbeat check failed");

  TBX_UNLOCK_SHARED(mad_leo_client);
  TBX_CRITICAL_SECTION_LEAVE(mad_ntbx_cs_barrier);
  LOG_OUT();
}

#ifdef MARCEL
static
void *
mad_command_thread(void *_madeleine)
{
  p_mad_madeleine_t madeleine   = _madeleine;
  p_mad_session_t   session     = NULL;
  p_mad_channel_t   mad_channel = NULL ;
  char             *name        = NULL;

  LOG_IN();
  session = madeleine->session;
  while (1) {
    int command = 0;

    command = mad_ntbx_receive_int(mad_leo_client);

    DISP_VAL("Leonie server command", command);

    switch (command) {

    case mad_leo_command_end_ack:
      {
        DISP("leaving command thread loop");
        goto end;
      }
      break;
    case mad_leo_command_barrier_passed:
      {
        DISP("marcel_sem_V");
        marcel_sem_V(barrier_passed);
      }
      break;
    case mad_leo_command_session_added:
      {
	 DISP("mad_leo_command_session_added");
	 if (madeleine->settings->leonie_dynamic_mode)
	   {
	      p_tbx_slist_t   slist   = madeleine->public_channel_slist;
	      p_mad_channel_t channel = NULL;
	      char           *name    = NULL;

	      if (!tbx_slist_is_nil(slist))
		{
		   tbx_slist_ref_to_head(slist);
		   do
		     {
			name    = tbx_slist_ref_get( slist );
			channel = tbx_htable_get( madeleine->channel_htable,name);
			if ( channel != NULL)
			  {
			     if (channel->mergeable == tbx_true)
			       madeleine->dynamic->mergeable = tbx_true ;
			  }
		     }
		   while (tbx_slist_ref_forward(slist));
		}
	   }
      }
      break;
    case mad_leo_command_update_dir:
      {
	 DISP("mad_leo_command_update_dir");
	 mad_new_directory_from_leony(madeleine);
	 mad_leonie_send_int(-1);
      }
      break;

    case mad_leo_command_shutdown_channel:
      {
	 DISP("mad_leo_command_shutdown_channel");
	 name = mad_leonie_receive_string();
	 //marcel_fprintf(stderr,"Channel name to shutdown :%s \n", name);
	 mad_channel = tbx_htable_extract(madeleine->channel_htable, name);
	 common_channel_exit2(mad_channel);
	 mad_leonie_send_int(-1);
      }
      break;
    case mad_leo_command_merge_channel:
      {
	 DISP("mad_leo_command_merge_channel");

         /* non compatible avec les pilotes nécessitant argc/argv */
	 mad_dir_driver_init(madeleine, NULL, NULL);
	 channel_reopen(madeleine);
	 mad_channel_merge_done(madeleine);
      }
      break;

     case mad_leo_command_split_channel:
      {
	 DISP("mad_leo_command_split_channel");
	 mad_leonie_send_int(-1);

         /* non compatible avec les pilotes nécessitant argc/argv */
	 mad_dir_driver_init(madeleine, NULL, NULL);
	 channel_reopen(madeleine);
	 mad_channel_split_done(madeleine);
      }
      break;
    default:
       TBX_FAILURE("invalid command from Leonie server");
    }
  }

 end:
  LOG_OUT();

  return NULL;
}

void
mad_command_thread_init(p_mad_madeleine_t madeleine)
{
  p_mad_session_t session = NULL;

  LOG_IN();
  session = madeleine->session;
  barrier_passed = TBX_MALLOC(sizeof(marcel_sem_t));
  marcel_sem_init(barrier_passed, 0);
  marcel_create(&(session->command_thread), NULL, mad_command_thread, madeleine);
  LOG_OUT();
}

void
mad_command_thread_exit(p_mad_madeleine_t madeleine)
{
  p_mad_session_t  session = NULL;
  void            *status  = NULL;

  LOG_IN();
  session = madeleine->session;
  marcel_join(session->command_thread, &status);
  LOG_OUT();
}

#endif /* MARCEL */
