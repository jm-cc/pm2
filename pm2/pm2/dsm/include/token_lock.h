
/*
 * CVS Id: $Id: token_lock.h,v 1.2 2002/10/14 15:42:06 slacour Exp $
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
extern int
token_lock_initialization (void);

/* clean up and free memory for the lock data structures */
extern int
token_lock_finalization (void);

/* initialize a new lock; the ID of the lock is returned as an output
 * argument */
extern int
token_lock_init (token_lock_id_t * const);

/* acquire a DSM lock */
extern int
token_lock (const token_lock_id_t);

/* release a DSM lock */
extern int
token_unlock (const token_lock_id_t);

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

