/*
 * NewMadeleine
 * Copyright (C) 2013-2016 (see AUTHORS file)
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

/** @defgroup connector_interface Connector interface
 * This is the connector interface, an interface designed for drivers to exchange urls.
 * End-users are not expected to use this interface designed only for nmad internal use.
 * @{
 */


/** timeout to receive connection ACK after sending connect address to peer (in msec.) */
#define NM_CONNECTOR_TIMEOUT_ACK   600

/** create a connector object */
struct nm_connector_s*nm_connector_create(int addr_len, const char**url);

/** dynamically exchange addresses with peer node */
int nm_connector_exchange(const char*local_connector_url, const char*remote_connector_url,
			  const void*local_cnx_addr, void*remote_cnx_addr);

/** destroy a connector object */
void nm_connector_destroy(struct nm_connector_s*p_connector);

/** @} */

#endif /* NM_CONNECTOR_H */

