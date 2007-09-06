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

#ifndef NM_PROTO_OPS_H
#define NM_PROTO_OPS_H

struct nm_proto;
struct nm_pkt_wrap;

/** Protocol commands.
 */
struct nm_proto_ops {

        /** Initialize the protocol module.
         */
        int (*init)		(struct nm_proto 	* const p_proto);

        /* handlers called when a packet is received for this protocol
           - scheduler call these handlers when it no longer has any
           processing to perform on the packet
           - the scheduler code is responsible for discarding the
           pkt_wrap structure when the handlers return, so the handlers
           must not free it */

        /* Process a successful outgoing request.
         */
        int (*out_success)	(struct nm_proto 	* const p_proto,
                                 struct nm_pkt_wrap	*p_pw);

        /* Process a failed outgoing request.
         */
        int (*out_failed)	(struct nm_proto 	* const p_proto,
                                 struct nm_pkt_wrap	*p_pw,
                                 int			_err);

        /* Process a successful incoming request.
         */
        int (*in_success)	(struct nm_proto 	* const p_proto,
                                 struct nm_pkt_wrap	*p_pw);

        /* Process a failed incoming request.
         */
        int (*in_failed)	(struct nm_proto 	* const p_proto,
                                 struct nm_pkt_wrap	*p_pw,
                                 int			_err);
};

#endif /* NM_PROTO_OPS_H */
