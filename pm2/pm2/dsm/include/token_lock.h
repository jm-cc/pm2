
/*
 * CVS Id: $Id: token_lock.h,v 1.9 2002/10/28 16:48:33 slacour Exp $
 */

/* Sebastien Lacour, Paris Research Group, IRISA / INRIA, May 2002 */

/* Distributed management of the locks.  Each lock has a (central)
 * fixed manager and an owner (token owner).  The lock manager can be
 * determined using the ID of the lock.  The manager always knows who
 * currently holds the token (lock). */


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

/* thread / cluster priority levels, to limit the number of
 * consecutive lock acquisition always within the same process or
 * cluster */
extern const unsigned long int INFINITE_PRIORITY;
extern const unsigned long int NO_PRIORITY;
extern const unsigned long int ANTI_PRIORITY;


/**********************************************************************/
/* PUBLIC FUNCTIONS */
/**********************************************************************/

/**********************************************************************/
/* USER API */

/* initialize a new lock; the ID of the lock is returned as an output
 * argument; return values can be DSM_SUCCESS or DSM_ERR_MEMORY if
* memory is exhausted. */
extern int
token_lock_init (token_lock_id_t * const);

/* acquire a DSM lock; return values can be DSM_SUCCESS (lock has been
 * acquired successfully), DSM_ERR_ILLEGAL (tried to acquire special
 * lock TOKEN_LOCK_NONE), DSM_ERR_NOT_INIT (tried to acquire a lock not
 * initialized), DSM_ERR_MEMORY (memory exhausted). */
extern int
token_lock (const token_lock_id_t);

/* release of a DSM lock; return values can be DSM_SUCCESS
 * (the lock has been released successfully), DSM_ERR_ILLEGAL (tried to
 * release special lock TOKEN_LOCK_NONE, or tried to release a lock
 * which has not been acquired, or tried to release a lock which was
 * not intialized) */
extern int
token_unlock (const token_lock_id_t);
   
/* partial release a DSM lock; this function _can_ be called
 * (optionnally) between token_lock() and token_unlock().  Return
 * values can be DSM_SUCCESS (the lock has been partially released
 * successfully), DSM_ERR_ILLEGAL (special lock TOKEN_LOCK_NONE, lock
 * not acquired, lock not intialized) */
extern int
token_partial_unlock (const token_lock_id_t);

/* set the maximum number of consecutive acquisitions of a lock within
 * a process; this function must be called after pm2_init(); legal
 * values are: INFINITE_PRIORITY, NO_PRIORITY, and positive integers. */
extern int
set_thread_priority_level (const unsigned long int);

/* set the maximum number of consecutive acquisitions of a lock within
 * a cluster; this function must be called after pm2_init(); legal
 * values are: INFINITE_PRIORITY, NO_PRIORITY, and positive integers. */
extern int
set_cluster_priority_level (const unsigned long int);


/**********************************************************************/
/* internal public functions */

/* initialization function of the lock data structures, called by
 * dsm_pm2_init() */
extern int
token_lock_initialization (void);

/* clean up and free memory for the lock data structures; this
* function is called by dsm_pm2_exit(). */
extern int
token_lock_finalization (void);

/* receive a release notification from a node in my local cluster */
extern void
token_lock_recv_release_notification_server (void);

/* being granted a lock token */
extern void
token_lock_recv_server (void);

/* service function called by RPC to request a lock */
extern void
token_lock_request_server (void);

/* the lock server of the manager */
extern void
token_lock_manager_server (void);

#endif   /* TOKEN_LOCK_H */

