
/*
 * CVS Id: $Id: hierarch_protocol.h,v 1.4 2002/10/23 14:07:07 slacour Exp $
 */

/* Sebastien Lacour, Paris Research Group, IRISA, May 2002 */

/* Hierarchical protocol, taking into account the network topology in
 * the consistency operations.  This protocol implements the "partial
 * release". */


#ifndef HIERARCH_PROTOCOL_H
#define HIERARCH_PROTOCOL_H

#include "dsm_const.h"   /* for the types: dsm_node_t, dsm_access_t,
                            dsm_page_index_t */
#include "token_lock.h"   /* for the type "token_lock_id_t" */


/**********************************************************************/
/* protocol management */

extern void hierarch_proto_finalization (void);
extern void hierarch_recv_diff_server (void);
extern void hierarch_diff_ack_server (void);
extern void hierarch_inv_ack_server (void);


/**********************************************************************/
/* memory consistency protocol main functions */

extern void
hierarch_proto_read_fault_handler (const dsm_page_index_t);

extern void
hierarch_proto_write_fault_handler (const dsm_page_index_t);

extern void
hierarch_proto_read_server (const dsm_page_index_t, const dsm_node_t, const int);

extern void
hierarch_proto_write_server (const dsm_page_index_t, const dsm_node_t, const int);

/* This is not the usual mechanism for the invalidate server; bypass
 * the DSM-PM2 RPC server; this function is an RPC server. */
extern void
hierarch_invalidate_server (void);

/* QUALIFIER "const" OF FIRST ARGUMENT YIELDS A WARNING! */
extern void
hierarch_proto_receive_page_server (/*const*/ void * const, const dsm_access_t,
                                   const dsm_node_t, const int);

extern void
hierarch_proto_acquire_func (const token_lock_id_t);

extern void
hierarch_proto_release_func (const token_lock_id_t);

extern void
hierarch_proto_initialization (const int);

extern void
hierarch_proto_page_add_func (const dsm_page_index_t);

#endif   /* HIERARCH_PROTOCOL_H */

