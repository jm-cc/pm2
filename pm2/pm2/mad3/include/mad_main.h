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
 * Mad_main.h
 * ===========
 */

#ifndef MAD_MAIN_H
#define MAD_MAIN_H

typedef enum e_mad_vrp_parameter_opcode
  {
    mad_op_uninitialized = 0,
    mad_op_optional_block,
  } mad_parameter_opcode_t, *p_mad_parameter_opcode_t;

typedef enum e_mad_status_opcode
  {
    mad_os_uninitialized = 0,
    mad_os_lost_block,
  } mad_status_opcode_t, *p_mad_status_opcode_t;

/*
 * Structures
 * ----------
 */

typedef struct s_mad_session
{
  p_ntbx_client_t      leonie_link;
  ntbx_process_grank_t process_rank;
} mad_session_t;

typedef struct s_mad_settings
{
#ifndef LEO_IP
  char *leonie_server_host_name;
#else
  unsigned long leonie_server_ip;
#endif // LEO_IP
  char         *leonie_server_port;
} mad_settings_t;


typedef struct s_mad_madeleine
{
  TBX_SHARED;
  p_mad_settings_t  settings;
  p_mad_session_t   session;
  p_mad_directory_t dir;
  p_tbx_htable_t    driver_htable;
  p_tbx_htable_t    channel_htable;
  p_tbx_slist_t     public_channel_slist;
} mad_madeleine_t;

/*
 * Functions
 * ---------
 */
p_mad_madeleine_t
mad_object_init(int   TBX_UNUSED   argc,
		char  TBX_UNUSED **argv);

void
mad_cmd_line_init(p_mad_madeleine_t   madeleine,
		  int                 argc,
		  char              **argv);

void
mad_leonie_link_init(p_mad_madeleine_t              madeleine,
		     int               TBX_UNUSED   argc,
		     char              TBX_UNUSED **argv);

void
mad_directory_init(p_mad_madeleine_t               madeleine,
		   int                TBX_UNUSED   argc,
		   char               TBX_UNUSED **argv);

void
mad_dir_driver_init(p_mad_madeleine_t   madeleine);

void
mad_dir_channel_init(p_mad_madeleine_t   madeleine);

void
mad_purge_command_line(p_mad_madeleine_t TBX_UNUSED madeleine,
		       int                         *_argc,
		       char                       **_argv);

p_mad_madeleine_t
mad_get_madeleine(void);

/*
 * Private part : these functions should not be called directly
 * --------------...............................................
 */

p_mad_madeleine_t
mad_init(int   *argc,
	 char **argv);

// NTBX data transmission wrappers
void
mad_ntbx_send_int(p_ntbx_client_t client,
		  const int       data);

int
mad_ntbx_receive_int(p_ntbx_client_t client);

void
mad_ntbx_send_unsigned_int(p_ntbx_client_t    client,
			   const unsigned int data);

unsigned int
mad_ntbx_receive_unsigned_int(p_ntbx_client_t client);

void
mad_ntbx_send_string(p_ntbx_client_t  client,
		     const char      *string);

char *
mad_ntbx_receive_string(p_ntbx_client_t client);


void
mad_leonie_send_int(const int data);

int
mad_leonie_receive_int(void);

void
mad_leonie_send_unsigned_int(const unsigned int data);

unsigned int
mad_leonie_receive_unsigned_int(void);

void
mad_leonie_send_string(const char *string);

char *
mad_leonie_receive_string(void);

void
mad_leonie_command_init(p_mad_madeleine_t   madeleine,
			int                 argc,
			char              **argv);

void
mad_leonie_command_exit(p_mad_madeleine_t   madeleine);

void
mad_leonie_print(char *fmt, ...)  __attribute__ ((format (printf, 1, 2)));

void
mad_leonie_barrier(void);

#define LDISP(str, args...)  mad_leonie_print(str , ## args)
#define LDISP_IN()           mad_leonie_print(__FUNCTION__": -->")
#define LDISP_OUT()          mad_leonie_print(__FUNCTION__": <--")
#define LDISP_VAL(str, val)  mad_leonie_print(str " = %d" , (int)(val))
#define LDISP_CHAR(val)      mad_leonie_print("%c" , (char)(val))
#define LDISP_PTR(str, ptr)  mad_leonie_print(str " = %p" , (void *)(ptr))
#define LDISP_STR(str, str2) mad_leonie_print(str ": %s" , (char *)(str2))

// Directory transmission
void
mad_dir_directory_get(p_mad_madeleine_t madeleine);

#endif /* MAD_MAIN_H */
