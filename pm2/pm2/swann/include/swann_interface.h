
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
 * swann_interface.h
 * -----------------
 */

#ifndef SWANN_INTERFACE_H
#define SWANN_INTERFACE_H


// Status
void
swann_usage(void) TBX_NORETURN;

void
swann_help(void) TBX_NORETURN;

void
swann_failure_cleanup(void);

void
swann_terminate(const char *msg) TBX_NORETURN;

void
swann_error(const char *command) TBX_NORETURN;

void
swann_processes_cleanup(void);


// Network
void
swann_ntbx_send_int(p_ntbx_client_t client,
		    const int       data);

int
swann_ntbx_receive_int(p_ntbx_client_t client);

void
swann_ntbx_send_unsigned_int(p_ntbx_client_t    client,
			     const unsigned int data);

unsigned int
swann_ntbx_receive_unsigned_int(p_ntbx_client_t client);

void
swann_ntbx_send_string(p_ntbx_client_t  client,
		       const char      *string);

char *
swann_ntbx_receive_string(p_ntbx_client_t client);


// Objects
p_swann_settings_t  
swann_settings_init(void);

p_swann_session_t  
swann_session_init(void);

p_swann_t  
swann_init(void);

#endif /* SWANN_INTERFACE_H */
