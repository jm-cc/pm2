
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
 * Leo_net.h
 * =========
 */ 

#ifndef __LEO_NET_H
#define __LEO_NET_H

void
leo_send_block(p_leo_swann_module_t  module,
	       void                 *ptr,
	       size_t                length);

void
leo_receive_block(p_leo_swann_module_t  module,
		  void                 *ptr,
		  size_t                length);

void
leo_send_string(p_leo_swann_module_t  module,
		char                 *data);

char *
leo_receive_string(p_leo_swann_module_t module);

void
leo_send_raw_int(p_leo_swann_module_t module,
		 int                  data);

int
leo_receive_raw_int(p_leo_swann_module_t module);

void
leo_send_raw_long(p_leo_swann_module_t module,
		  long                 data);

long
leo_receive_raw_long(p_leo_swann_module_t module);

void
leo_send_int(p_leo_swann_module_t module,
	     int                  data);

int
leo_receive_int(p_leo_swann_module_t module);

void
leo_send_long(p_leo_swann_module_t module,
	      long                 data);

long
leo_receive_long(p_leo_swann_module_t module);

void
leo_send_command(p_leo_swann_module_t module,
		 ntbx_command_code_t  code);

p_ntbx_server_t
leo_net_server_init(void);

#endif /* __LEO_NET_H */
