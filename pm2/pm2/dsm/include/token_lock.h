
/*
 * CVS Id: $Id: token_lock.h,v 1.1 2002/10/13 12:14:02 slacour Exp $
 */

/* distributed management of the locks (tokens) for DSM-PM2 */

#ifndef TOKEN_LOCK_H
#define TOKEN_LOCK_H


/**********************************************************************/
/* PUBLIC DATA TYPES */
/**********************************************************************/

/* identification of a lock */
typedef unsigned int token_lock_id_t;


/**********************************************************************/
/* GLOBAL PUBLIC VARIABLES */
/**********************************************************************/

/* no lock */
extern const token_lock_id_t TOKEN_LOCK_NONE;


/**********************************************************************/
/* PUBLIC FUNCTIONS */
/**********************************************************************/

/* receive a release notification from a node in my local cluster */
extern void
token_lock_recv_release_notification_server (void);

/* being granted a lock token */
extern void
token_lock_recv_server (void);

/* initialization function of the lock data structures, called by
 * dsm_pm2_init() */
extern void
token_lock_initialization (void);

/* clean up and free memory for the lock data structures */
extern void
token_lock_finalization (void);

/* initialize a new lock; the ID of the lock is returned as an output
 * argument; the return value is 0 in case of success */
extern int
token_lock_init (token_lock_id_t * const);

/* acquire a DSM lock */
extern void
token_lock (const token_lock_id_t);

/* release a DSM lock */
extern void
token_unlock (const token_lock_id_t);

/* partial release of a lock: signal the lock can be acquired in the
 * local cluster, but not by a node in a remote cluster.  This
 * function should be called by the memory consistency release function. */
extern void
token_partial_unlock (const token_lock_id_t);

/* service function called by RPC to request a lock */
extern void
token_lock_request_server (void);

/* the lock server of the manager */
extern void
token_lock_manager_server (void);

/* register the number of diff and inv ack expected for a given lock */
extern void
token_lock_register_acks (const token_lock_id_t, const int, const int,
                          const int);

#endif   /* TOKEN_LOCK_H */

