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

#ifndef NM_PROTO_H
#define NM_PROTO_H

#include "nm_proto_ops.h"
#include "nm_proto_cap.h"

struct nm_core;

/** Protocol.
 */
struct nm_proto {

        /** Protocol id. */
        uint8_t	id;

        /** nm_core object. */
        struct nm_core		*p_core;

        /** Implementation dependent data.
         */
        void			*priv;

        /** Protocol operations. */
        struct nm_proto_ops	 ops;

        /** Protocol capabilities. */
        struct nm_proto_cap	 cap;
};

#endif /* NM_PROTO_H */
