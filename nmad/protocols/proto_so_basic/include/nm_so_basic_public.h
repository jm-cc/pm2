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


struct nm_basic_rq;

int
nm_so_basic_isend(struct nm_proto		 *p_proto,
                  nm_gate_id_t			  gate_id,
                  uint8_t			  tag_id,
                  void			 *ptr,
                  uint64_t			  len,
                  struct nm_basic_rq	**pp_rq);

int
nm_so_basic_irecv(struct nm_proto		 *p_proto,
                  nm_gate_id_t			  gate_id,
                  uint8_t			  tag_id,
                  void			 *ptr,
                  uint64_t			  len,
                  struct nm_basic_rq	**pp_rq);

int
nm_so_basic_irecv_any(struct nm_proto	 *p_proto,
                      uint8_t		  tag_id,
                      void			 *ptr,
                      uint64_t		  len,
                      struct nm_basic_rq	**pp_rq);

int
nm_so_basic_wait(struct nm_basic_rq	 *p_rq);

int
nm_so_basic_test(struct nm_basic_rq	 *p_rq);

int
nm_so_basic_load(struct nm_proto_ops	 *p_ops);
