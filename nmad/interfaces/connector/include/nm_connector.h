/*
 * NewMadeleine
 * Copyright (C) 2013 (see AUTHORS file)
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

/* -*- Mode: C; tab-width: 2; c-basic-offset: 2 -*- */

#ifndef NM_CONNECTOR_H
#define NM_CONNECTOR_H

/** timeout to receive connection ACK after sending connect address to peer (in msec.) */
#define NM_IBVERBS_TIMEOUT_ACK   600

/* ********************************************************* */
/* ** connection management */

struct nm_connector_s*nm_connector_create(int addr_len, const char**url);

int nm_connector_exchange(const char*local_connector_url, const char*remote_connector_url,
			  const void*local_cnx_addr, void*remote_cnx_addr);



#endif /* NM_CONNECTOR_H */

