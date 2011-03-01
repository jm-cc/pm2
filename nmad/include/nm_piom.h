/*
 * NewMadeleine
 * Copyright (C) 2006 (see AUTHORS file)
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

#ifdef PIOMAN_POLL

#ifndef NM_PIOM_H
#define NM_PIOM_H

#include "pioman.h"

int nm_piom_init(struct nm_core * p_core);


#ifdef PIOM_BLOCKING_CALLS

int 
nm_piom_block(piom_server_t server,
	      piom_op_t     _op,
	      piom_req_t    req, 
	      int            nb_ev, 
	      int            option);

int nm_piom_block_any(piom_server_t            server,
		     piom_op_t                _op,
		     piom_req_t               req, 
		     int                       nb_ev, 
		     int                       option);

int 
nm_piom_block_send(struct nm_pkt_wrap  *p_pw);

int 
nm_piom_block_recv(struct nm_pkt_wrap  *p_pw);

#endif /* PIOM_BLOCKING_CALLS */

#endif /* NM_PIOM_H */

#endif /* PIOMAN_POLL*/
