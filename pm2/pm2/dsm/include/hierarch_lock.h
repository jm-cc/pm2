
/*
 * CVS Id: $Id: hierarch_lock.h,v 1.1 2002/10/11 18:03:49 slacour Exp $
 */

/* distributed management of the locks (tokens) for DSM-PM2 */

#ifndef HIERARCH_LOCK_H
#define HIERARCH_LOCK_H


/**********************************************************************/
/* PUBLIC DATA TYPES */
/**********************************************************************/

/* identification of a lock */
typedef unsigned int hierarch_lock_id_t;


/**********************************************************************/
/* GLOBAL PUBLIC VARIABLES */
/**********************************************************************/

/* no lock */
extern const hierarch_lock_id_t HIERARCH_NO_LOCK;


/**********************************************************************/
/* PUBLIC FUNCTIONS */
/**********************************************************************/

/* receive a release notification from a node in my local cluster */
extern void
hierarch_recv_release_notification_server (void);

/* being granted a lock token */
extern void
hierarch_recv_lock_token_server (void);

/* initialization function of the lock data structures, called by
 * dsm_pm2_init() */
extern void
hierarch_lock_initialization (void);

/* clean up and free memory for the lock data structures */
extern void
hierarch_lock_exit (void);

/* initialize a new lock; the ID of the lock is returned as an output
 * argument; the return value is 0 in case of success */
extern int
hierarch_lock_init (hierarch_lock_id_t * const);

/* acquire a DSM lock */
extern void
hierarch_lock (const hierarch_lock_id_t);

/* release a DSM lock */
extern void
hierarch_unlock (const hierarch_lock_id_t);

/* partial release of a lock: signal the lock can be acquired in the
 * local cluster, but not by a node in a remote cluster.  This
 * function should be called by the memory consistency release function. */
extern void
hierarch_partial_unlock (const hierarch_lock_id_t);

/* service function called by RPC to request a lock */
extern void
hierarch_lock_request_server (void);

/* the lock server of the manager */
extern void
hierarch_lock_manager_server (void);

/* register the number of diff and inv ack expected for a given lock */
extern void
hierarch_register_acks (const hierarch_lock_id_t, const int, const int,
                        const int);

#endif   /* HIERARCH_LOCK_H */

