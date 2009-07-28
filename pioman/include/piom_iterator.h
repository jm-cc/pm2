/* -*- Mode: C; c-basic-offset:4 ; -*- */
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

#ifndef PIOM_ITERATOR_H
#define PIOM_ITERATOR_H

/****************************************************************
 * Iterators over the submitted queries of a server
 */

/* Base iterator
 *   piom_req_t req : iterator
 *   piom_server_t server : server
 */
#define FOREACH_REQ_REGISTERED_BASE(req, server)			\
    tbx_fast_list_for_each_entry((req), &(server)->list_req_registered, chain_req_registered)

/* Idem but safe (intern use) */
#define FOREACH_REQ_REGISTERED_BASE_SAFE(req, tmp, server)		\
    tbx_fast_list_for_each_entry_safe((req), (tmp), &(server)->list_req_registered, chain_req_registered)

/* Iterates using a custom type 
 *
 * [User Type] req : pointer to a structure contening a struct piom_req
 *                   (iterator)
 *   piom_server_t server : server
 *   member : name of struct piom in the structure pointed by req
 */
#define FOREACH_REQ_REGISTERED(req, server, member)			\
    tbx_fast_list_for_each_entry((req), &(server)->list_req_registered, member.chain_req_registered)

/****************************************************************
 * Iterator over the grouped polling queries
 */

/* Base iterator
 *   piom_req_t req : iterator
 *   piom_server_t server : server
 */
#define FOREACH_REQ_POLL_BASE(req, server)				\
    tbx_fast_list_for_each_entry((req), &(server)->list_req_poll_grouped, chain_req_grouped)

/* Idem but safe (intern use) */
#define FOREACH_REQ_POLL_BASE_SAFE(req, tmp, server)			\
    tbx_fast_list_for_each_entry_safe((req), (tmp), &(server)->list_req_poll_grouped, chain_req_grouped)

/* Iterates using a custom type 
 *
 * [User Type] req : pointer to a structure contening a struct piom_req
 *                   (iterator)
 *   piom_server_t server : server
 *   member : name of struct piom in the structure pointed by req
 */
#define FOREACH_REQ_POLL(req, server, member)				\
    tbx_fast_list_for_each_entry((req), &(server)->list_req_poll_grouped, member.chain_req_grouped)

/****************************************************************
 * Iterator over the grouped polling queries
 */

/* Base iterator
 *   piom_req_t req : iterator
 *   piom_server_t server : server
 */
#define FOREACH_REQ_BLOCKING_BASE(req, server)				\
    tbx_fast_list_for_each_entry((req), &(server)->list_req_block_grouped,  chain_req_block_grouped)

/* Idem but safe (intern use) */
#define FOREACH_REQ_BLOCKING_BASE_SAFE(req, tmp, server)		\
    tbx_fast_list_for_each_entry_safe((req), (tmp), &(server)->list_req_block_grouped, chain_req_block_grouped)

/* Iterates using a custom type 
 *
 * [User Type] req : pointer to a structure contening a struct piom_req
 *                   (iterator)
 *   piom_server_t server : server
 *   member : name of struct piom in the structure pointed by req
 */
#define FOREACH_REQ_BLOCKING(req, server, member)			\
    tbx_fast_list_for_each_entry((req), &(server)->list_req_block_grouped, member.chain_req_block_grouped)

#define FOREACH_REQ_BLOCKING_SAFE(req, tmp, server, member)		\
    tbx_fast_list_for_each_entry_safe((req),(tmp), &(server)->list_req_block_grouped, member.chain_req_block_grouped)

/****************************************************************
 * Iterator over the queries to be exported
 */

/* Base iterator
 *   piom_req_t req : iterator
 *   piom_server_t server : server
 */
#define FOREACH_REQ_TO_EXPORT_BASE(req, server)				\
    tbx_fast_list_for_each_entry((req), &(server)->list_req_to_export, chain_req_to_export)

/* Idem but safe (intern use) */
#define FOREACH_REQ_TO_EXPORT_BASE_SAFE(req, tmp, server)		\
    tbx_fast_list_for_each_entry_safe((req), (tmp), &(server)->list_req_to_export, chain_req_to_export)

/* Iterates using a custom type 
 *
 * [User Type] req : pointer to a structure contening a struct piom_req
 *                   (iterator)
 *   piom_server_t server : server
 *   member : name of struct piom in the structure pointed by req
 */
#define FOREACH_REQ_TO_EXPORT(req, server, member)			\
    tbx_fast_list_for_each_entry((req), &(server)->list_req_to_export, member.chain_req_to_export)

/****************************************************************
 * Iterator over the exported queries
 */

/* Base iterator
 *   piom_req_t req : iterator
 *   piom_server_t server : server
 */
#define FOREACH_REQ_EXPORTED_BASE(req, server)				\
    tbx_fast_list_for_each_entry((req), &(server)->list_req_exported, chain_req)

/* Idem but safe (intern use) */
#define FOREACH_REQ_EXPORTED_BASE_SAFE(req, tmp, server)		\
    tbx_fast_list_for_each_entry_safe((req), (tmp), &(server)->list_req_exported, chain_req)

/* Iterates using a custom type 
 *
 * [User Type] req : pointer to a structure contening a struct piom_req
 *                   (iterator)
 *   piom_server_t server : server
 *   member : name of struct piom in the structure pointed by req
 */
#define FOREACH_REQ_EXPORTED(req, server, member)			\
    tbx_fast_list_for_each_entry((req), &(server)->list_req_exported, member.chain_req)


/****************************************************************
 * Iterator over the waiters attached to a request
 */

/* Base iterator
 *   piom_wait_t wait : iterator
 *   piom_req_t req : request
 */
#define FOREACH_WAIT_BASE(wait, req)				\
    tbx_fast_list_for_each_entry((wait), &(req)->list_wait, chain_wait)

/* Idem but safe (intern use) */
#define FOREACH_WAIT_BASE_SAFE(wait, tmp, req)				\
    tbx_fast_list_for_each_entry_safe((wait), (tmp), &(req)->list_wait, chain_wait)

/* Iterates using a custom type
 *
 *   [User Type] req : pointer to a structure contening a struct piom_req
 *                   (iterator)
 *   piom_server_t server : server
 *   member : name of struct piom in the structure pointed by req
 */
#define FOREACH_WAIT(wait, req, member)					\
    tbx_fast_list_for_each_entry((wait), &(req)->list_wait, member.chain_wait)

#endif	/* PIOM_ITERATOR_H */
