
/*
 * CVS Id: $Id: token_lock.c,v 1.27 2002/11/29 18:44:59 slacour Exp $
 */

/* Sebastien Lacour, Paris Research Group, IRISA / INRIA, May 2002 */

/* Distributed management of the locks.  Each lock has a (central)
 * fixed manager and an owner (token owner).  The lock manager can be
 * determined using the ID of the lock.  The manager always knows who
 * currently holds the token (lock). */

/* Currently does not support thread migration */


#include <stdlib.h>
#include "token_lock.h"
#include "hierarch_topology.h"
#include "pm2.h"
#include "dsm_const.h"


/**********************************************************************/
/* TRACING AND DEBUGGING SYSTEM                                       */
/**********************************************************************/

/* don't print tracing / debugging messages */
#undef TRCSL
#undef DBGSL
/* remove all assertions "assert" */
#define NDEBUG

#include "trace_debug.h"


/**********************************************************************/
/* PRIVATE DATA STRUCTURES                                            */
/**********************************************************************/

/* who to grant a token lock to */
typedef enum
{
   NOWHERE,
   LOCAL_THREAD,
   LOCAL_CLUSTER,
   REMOTE_CLUSTER
} lock_destination_t;

/* tag for the lock requests: either no tag, or a tag meaning the lock
 * request was received too late (i.e.: just after the token was
 * granted to a node in ANOTHER cluster) or meaning the token must be
 * requested from a node in another cluster. */
typedef enum { NO_TAG, FORCE_TAG } request_tag_t;

/* a boolean type */
typedef enum { SL_FALSE, SL_TRUE } bool_t;

/* a lock can be fully granted (INTERcluster) or partially granted
 * (INTRAcluster).  Otherwise, the requested lock does not exist */
typedef enum { INTER, INTRA, DOES_NOT_EXIST } grant_state_t;

/* tags used while calling update_expected_release() to modify the
 * number of local / remote expected release notifications */
typedef enum
{
   JUST_ACQUIRED, JUST_RELEASED, JUST_RECVD_NOTIF, JUST_RECVD_TOKEN
} update_t;

/* possible statuses of a lock */
typedef enum
{
   NON_EXISTENT,      /* the lock was requested but was not initialized */
   UNKNOWN,           /* I'm not the owner of the token */
   LOCALLY_PRESENT    /* I own the token */
} lock_status_t;

/* linked list of nodes */
typedef struct node_list_struct node_elem_t;
struct node_list_struct
{
   dsm_node_t node;         /* node number */
   node_elem_t * next; /* next element of the linked list */
};

/* linked list of locks */
typedef struct lock_list_struct lock_list_elem_t;
struct lock_list_struct
{
   token_lock_id_t lock_id; /* global unique ID of the lock */
   marcel_mutex_t mutex;      /* mutex to handle any field of the lock entry */
   lock_status_t status;      /* is the token here? away? non existent? */

   /* has the token been requested? */
   bool_t requested;

   /* identifier of the thread which acquired the lock; NULL <==> lock
    * is free (no partial release expected); non-NULL <==> lock is
    * busy (a partial release is expected). */
   marcel_t owner_thread;

   /* lock requester, in the local cluster */
   dsm_node_t local_next_owner;
   /* lock requester, in a remote cluster */
   dsm_node_t remote_next_owner;

   /* a boolean which becomes true when the lock token is granted to
    * another cluster; becomes false when receiving the lock token, or
    * when forwarding a request that comes too late (with
    * tag==FORCE_TAG) to the lock manager */
   bool_t just_granted;

   /* last requester of the lock for each cluster; relevant only for
    * the lock manager */
   dsm_node_t *last_requester;

   /* the last cluster which requested the lock; relevant for the lock
    * manager only */
   int last_cluster;

   int reservation;  /* number of local threads which reserved the
                        lock (granting a lock to a local thread is
                        preferred to granting it to another node) */

   /* signal the token is available: either just arrived or just freed */
   marcel_cond_t token_available;

   /* number of release signals expected from other nodes (of the
    * local cluster) */
   int expected_remote_release;

   /* number of release signals expected from the threads of this
    * process */
   int expected_local_release;

   /* list of nodes (in the local cluster) which were granted the lock
    * token while releases were expected (lock partially granted) and
    * to which I must send a release notification. */
   node_elem_t * notif_list;
   /* pointer to the last element of the previous linked list */
   node_elem_t * end_of_notif_list;

   /* number of times a lock was granted to a thread in the local
    * process while some other requesting node in a local cluster was
    * registered */
   unsigned long int thread_priority;
   /* number of times a lock was granted to a node in the local
    * cluster while some other requesting node in a remote cluster was
    * registered */
   unsigned long int cluster_priority;

   /* when a thread releases a lock which will be acquired immediately
    * by another thread in the same process, there is no need to call
    * the release and acquire consistency functions */
   bool_t call_acquire_func;

   lock_list_elem_t * next; /* next lock in the linked list */
};


/**********************************************************************/
/* GLOBAL PRIVATE VARIABLES                                           */
/**********************************************************************/

/* hash table for the lock data structures; the pm2_config_size()
 * should not be a multiple of LOCK_HASH_SIZE.  This particular point
 * (the hash table size) could be improved, taking the
 * pm2_config_size() into account. */
#define LOCK_HASH_SIZE 11
static lock_list_elem_t * lock_hash[LOCK_HASH_SIZE];

/* Total number of locks initialized locally (this is the number of
 * locks this node manages). */
static token_lock_id_t local_lock_number;

/* Lock for any operation on the lock data structures */
static marcel_mutex_t lock_mutex;

/* maximum number of lock acquisitions inside a given process to the
 * detriment of other nodes (NO_PRIORITY to disable the local thread
 * priority; INFINITE_PRIORITY to mean "no limit", "infinity") */
static unsigned long int max_local_thread_acquire;

/* maximum number of lock acquisitions inside a cluster to the
 * detriment of remote nodes (NO_PRIORITY to disable the local node
 * priority; INFINITE_PRIORITY to mean "no limit", "infinity") */
static unsigned long int max_local_cluster_acquire;


/**********************************************************************/
/* GLOBAL PUBLIC VARIABLES                                            */
/**********************************************************************/

/* no lock */
const token_lock_id_t TOKEN_LOCK_NONE = ((token_lock_id_t)(-1));

const unsigned long int INFINITE_PRIORITY = ULONG_MAX;
const unsigned long int ANTI_PRIORITY = 0;
const unsigned long int NO_PRIORITY = 1;


/**********************************************************************/
/* PRIVATE FUNCTIONS                                                  */
/**********************************************************************/

/**********************************************************************/
/* convert a local lock_id into a global (unique) lock_id */
static token_lock_id_t
local_to_global_lock_id (const token_lock_id_t local_lck_id)
{
   return (token_lock_id_t) (local_lck_id * pm2_config_size() + pm2_self());
}


/**********************************************************************/
/* return the manager of a global lock ID */
static dsm_node_t
get_lock_manager (const token_lock_id_t global_lck_id)
{
   assert ( global_lck_id != TOKEN_LOCK_NONE );
   return (dsm_node_t) ( global_lck_id % pm2_config_size() );
}


/**********************************************************************/
/* return the hash bucket for the lock hash table */
static unsigned int
lock_hash_func (const token_lock_id_t lck_id)
{
   assert ( lck_id != TOKEN_LOCK_NONE );
   return (unsigned int) (lck_id % LOCK_HASH_SIZE);
}


/**********************************************************************/
/* return a pointer to the lock structure corresponding to the global
 * lock id given as a parameter in the lock hash table; return NULL if
 * the lock id is not found; the mutex "lock_mutex" is assumed to be
 * acquired. */
static lock_list_elem_t *
seek_lock_in_hash (const token_lock_id_t lck_id)
{
   const unsigned int hash_bucket = lock_hash_func(lck_id);
   lock_list_elem_t * lock_list = lock_hash[hash_bucket];

   while ( lock_list != NULL )
      if ( lck_id == lock_list->lock_id )
         return lock_list;
      else
         lock_list = lock_list->next;

   /* lock_id not found in the list */
   return NULL;
}


/**********************************************************************/
/* insert a node into the list of nodes to be signalled at when the
 * lock gets fully released.  The lock entry is assumed to be
 * locked.  The node must be added at the end of the list. */
static void
insert_into_node_list (node_elem_t ** list, node_elem_t ** end_of_list,
                       const dsm_node_t node)
{
   node_elem_t * elem;

   IN;
   TRACE("add node %d to list", node);

   elem = (node_elem_t *) tmalloc (sizeof(node_elem_t));
   if ( elem == NULL )
   {
      perror(__FUNCTION__);
      OUT;
      abort();
   }
   elem->node = node;
   elem->next = NULL;
   if ( *list == NULL )   /* the list was empty */
   {
      assert ( *end_of_list == NULL );
      *list = elem;
   }
   else
   {
      assert ( *end_of_list != NULL );
      (*end_of_list)->next = elem;
   }
   *end_of_list = elem;

   OUT;
   return;
}


/**********************************************************************/
/* grant a lock token to another node; the lock entry is assumed to be
 * locked. */
static void
grant_lock (const dsm_node_t to_node, lock_list_elem_t * const lck_elem)
{
   grant_state_t state;
   dsm_node_t rmt_nxt;
   unsigned long int cluster_priority;

   IN;
   assert ( to_node != pm2_self() );
   assert ( to_node != NOBODY );
   assert ( to_node < pm2_config_size() );
   assert ( lck_elem != NULL );
   assert ( lck_elem->owner_thread == NULL );
   assert ( lck_elem->status == LOCALLY_PRESENT );

   if ( lck_elem->expected_local_release > 0 ||
        lck_elem->expected_remote_release > 0 )   /* grant partially */
   {
      assert ( topology_get_cluster_color(to_node) ==
               topology_get_cluster_color(pm2_self()) );
      state = INTRA;   /* intra-cluster only */
      /* add the node into the list of nodes to which i'll forward the
       * full release signal */
      insert_into_node_list(&(lck_elem->notif_list),
                            &(lck_elem->end_of_notif_list),
                            to_node);
   }
   else   /* fully grant the lock token */
   {
      assert ( lck_elem->notif_list == NULL );
      state = INTER;
   }

   pm2_rawrpc_begin(to_node, TOKEN_LOCK_RECV, NULL);
   pm2_pack_byte(SEND_CHEAPER, RECV_CHEAPER, &(lck_elem->lock_id),
                 sizeof(token_lock_id_t));
   pm2_pack_byte(SEND_CHEAPER, RECV_CHEAPER, &state, sizeof(grant_state_t));
   if ( to_node != lck_elem->remote_next_owner )
      rmt_nxt = lck_elem->remote_next_owner;
   else
      rmt_nxt = NOBODY;
   pm2_pack_byte(SEND_CHEAPER, RECV_CHEAPER, &rmt_nxt, sizeof(dsm_node_t));
   if ( topology_get_cluster_color(to_node) ==
        topology_get_cluster_color(pm2_self()) )
      cluster_priority = lck_elem->cluster_priority;
   else
      cluster_priority = 0;
   pm2_pack_byte(SEND_CHEAPER, RECV_CHEAPER, &cluster_priority,
                 sizeof(unsigned long int));
   pm2_rawrpc_end();
   TRACE("grant lock %d to node %d with state='%s', rmt_nxt=%d, lcl_rel=%d, " \
         "rmt_rel=%d, cluster_priority=%lu", lck_elem->lock_id, to_node,
         state == INTER ? "inter" : "intra", rmt_nxt,
         lck_elem->expected_local_release, lck_elem->expected_remote_release,
         cluster_priority);

   lck_elem->status = UNKNOWN;
   lck_elem->local_next_owner = lck_elem->remote_next_owner = NOBODY;
   lck_elem->cluster_priority = lck_elem->thread_priority = 0;

   OUT;
   return;
}


/**********************************************************************/
/* send a notification of release to the first node from
 * the node_list; the lock entry is assumed to be locked. */
static void
send_release_notification (lock_list_elem_t * const lck_elem)
{
   node_elem_t * const save = lck_elem->notif_list;

   IN;
   assert ( lck_elem != NULL );
   assert ( save != NULL );
   assert ( save->node != pm2_self() );
   assert ( save->node != NOBODY );
   pm2_rawrpc_begin(save->node, TOKEN_LOCK_RELEASE_NOTIFICATION, NULL);
   pm2_pack_byte(SEND_CHEAPER, RECV_CHEAPER, &(lck_elem->lock_id),
                 sizeof(token_lock_id_t));
   pm2_rawrpc_end();
   TRACE("sending release notification to node %d for lock %d",
         save->node, lck_elem->lock_id);
   lck_elem->notif_list = save->next;
   if ( lck_elem->notif_list == NULL )   /* empty list */
        lck_elem->end_of_notif_list = NULL;
   tfree(save);

   OUT;
   return;
}


/**********************************************************************/
/* decide whether a release notification should be sent or not; the
 * lock entry is assumed to be locked. */
static void
update_expected_release (lock_list_elem_t * const lck_elem, const update_t type)
{
   IN;

   assert ( lck_elem != NULL );

   switch ( type )
   {
      case JUST_RECVD_TOKEN:
         lck_elem->expected_remote_release++;
         break;
      case JUST_RELEASED:
         lck_elem->expected_local_release--;
         if ( lck_elem->owner_thread == marcel_self() )
            lck_elem->owner_thread = NULL;
         break;
      case JUST_RECVD_NOTIF:
         lck_elem->expected_remote_release--;
         break;
      case JUST_ACQUIRED:
         lck_elem->expected_local_release++;
         assert ( lck_elem->owner_thread == NULL );
         lck_elem->owner_thread = marcel_self();
         assert ( lck_elem->owner_thread != NULL );
         break;
      default: assert ( ! "Should never reach this point" );
   }

   TRACE("lcl_rel=%d, rmt_rel=%d for lock %d", lck_elem->expected_local_release,
         lck_elem->expected_remote_release, lck_elem->lock_id);

   if ( lck_elem->notif_list == NULL )
   {
      assert ( lck_elem->end_of_notif_list == NULL );
      OUT;
      return;
   }

   /* keep at most one element in the notification list */
   while ( lck_elem->notif_list != lck_elem->end_of_notif_list )
   {
      assert ( lck_elem->notif_list != NULL );
      assert ( lck_elem->end_of_notif_list != NULL );
      send_release_notification(lck_elem);
   }
   assert ( lck_elem->notif_list == lck_elem->end_of_notif_list );
   assert ( lck_elem->notif_list != NULL );
   assert ( lck_elem->end_of_notif_list != NULL );

   if ( type == JUST_RECVD_TOKEN || type == JUST_ACQUIRED )
   {
      assert ( lck_elem->status == LOCALLY_PRESENT );
      /* empty the notification list */
      send_release_notification(lck_elem);
      assert ( lck_elem->notif_list == NULL );
      assert ( lck_elem->end_of_notif_list == NULL );
   }
   else
   {
      assert ( type == JUST_RELEASED || type == JUST_RECVD_NOTIF );
      if ( lck_elem->status == LOCALLY_PRESENT ||
           ( lck_elem->expected_local_release == 0 &&
             lck_elem->expected_remote_release == 0 ) )
      {
         send_release_notification(lck_elem);
         assert ( lck_elem->notif_list == NULL );
         assert ( lck_elem->end_of_notif_list == NULL );
      }
   }

   OUT;
   return;
}


/**********************************************************************/
/* send a request for a lock with a given tag to the manager; the lock
 * entry is assumed to be locked */
static void
request_lock_from_manager (dsm_node_t requester, token_lock_id_t lck_id,
                           request_tag_t tag)
{
   IN;
   assert ( requester != NOBODY );
   assert ( requester < pm2_config_size() );
   assert ( lck_id != TOKEN_LOCK_NONE );
   TRACE("requesting lock %d from manager %d for requester %d with tag '%s'",
         lck_id, get_lock_manager(lck_id), requester,
         tag == NO_TAG ? "NO_TAG" : "FORCE_TAG");

   pm2_rawrpc_begin(get_lock_manager(lck_id), TOKEN_LOCK_MANAGER_SERVER, NULL);
   pm2_pack_byte(SEND_CHEAPER, RECV_CHEAPER, &requester, sizeof(dsm_node_t));
   pm2_pack_byte(SEND_CHEAPER, RECV_CHEAPER, &lck_id,
                 sizeof(token_lock_id_t));
   pm2_pack_byte(SEND_CHEAPER, RECV_CHEAPER, &tag, sizeof(request_tag_t));
   pm2_rawrpc_end();

   OUT;
   return;
}


/**********************************************************************/
/* decide whether the lock token should be granted to the local
 * thread, to the local cluster or the a remote cluster.  The token
 * entry is assumed to be locked. */
static void
decide_who_to_grant_to (lock_list_elem_t * const lck_elem,
                        lock_destination_t * const initial_dest,
                        lock_destination_t * const destination)
{
   const int can_go_to_remote_cluster =
      ( (lck_elem->remote_next_owner != NOBODY) &&
        (lck_elem->expected_local_release == 0) &&
        (lck_elem->expected_remote_release == 0) );

   IN;
   assert ( lck_elem != NULL );

   *destination = *initial_dest = NOWHERE;
   if ( lck_elem->status != LOCALLY_PRESENT || lck_elem->owner_thread != NULL )
   {
      OUT;
      return;
   }

   TRACE("lck %d: reserv=%d, lcl_nxt=%d, rmt_nxt=%d, lcl_rel=%d, rmt_rel=%d",
         lck_elem->lock_id, lck_elem->reservation, lck_elem->local_next_owner,
         lck_elem->remote_next_owner, lck_elem->expected_local_release,
         lck_elem->expected_remote_release);
   TRACE("lck %d: thrd_prio=%d, max_thrd=%d, cluster_prio=%d, max_cluster=%d",
         lck_elem->lock_id, lck_elem->thread_priority,
         max_local_thread_acquire, lck_elem->cluster_priority,
         max_local_cluster_acquire);

   /* granting the lock to a local thread is prefered to granting it
    * to another node; granting the lock to a node in the local
    * cluster is prefered to granting it to a node in a remote cluster */
   if ( lck_elem->reservation > 0 )
      *initial_dest = LOCAL_THREAD;
   else if ( lck_elem->local_next_owner != NOBODY )
      *initial_dest = LOCAL_CLUSTER;
   else if ( can_go_to_remote_cluster )
      *initial_dest = REMOTE_CLUSTER;

   if ( *initial_dest == NOWHERE )
   {
      OUT;
      return;
   }
   else
      *destination = *initial_dest;

   if ( lck_elem->thread_priority >= max_local_thread_acquire &&
        *destination == LOCAL_THREAD )
   {
      if ( lck_elem->local_next_owner != NOBODY )
         *destination = LOCAL_CLUSTER;
      else if ( can_go_to_remote_cluster )
         *destination = REMOTE_CLUSTER;
   }

   if ( lck_elem->cluster_priority >= max_local_cluster_acquire &&
        *destination == LOCAL_CLUSTER && can_go_to_remote_cluster )
      *destination = REMOTE_CLUSTER;

   OUT;
   return;
}


/**********************************************************************/
/* try to grant the lock to a thread in the local process, or to a
 * process in the local cluster or to a node in a remote cluster (in
 * that order of preference).  The token entry is assumed to be locked */
static void
grant_lock_if_possible (lock_list_elem_t * const lck_elem,
                        const lock_destination_t initial_destination,
                        const lock_destination_t destination)
{
   dsm_node_t requester = NOBODY;

   IN;
   assert ( lck_elem != NULL );
   if ( destination == NOWHERE )
   {
      OUT;
      return;
   }
   assert ( lck_elem->status == LOCALLY_PRESENT );
   assert ( lck_elem->owner_thread == NULL );

   TRACE("try to grant lock %d; rmt_nxt=%d, lcl_nxt=%d, rmt_rel=%d, lcl_rel=%d",
         lck_elem->lock_id, lck_elem->remote_next_owner,
         lck_elem->local_next_owner, lck_elem->expected_remote_release,
         lck_elem->expected_local_release);

   if ( initial_destination == LOCAL_THREAD )
      requester = pm2_self();
   else if ( initial_destination == LOCAL_CLUSTER )
      requester = lck_elem->local_next_owner;

   switch ( destination )
   {
      case LOCAL_THREAD:
         assert ( initial_destination == LOCAL_THREAD );
         if ( max_local_thread_acquire != INFINITE_PRIORITY &&
              ( lck_elem->local_next_owner != NOBODY ||
                lck_elem->remote_next_owner != NOBODY ) )
            lck_elem->thread_priority++;
         marcel_cond_signal(&(lck_elem->token_available));
         break;
      case LOCAL_CLUSTER:
         if ( max_local_cluster_acquire != INFINITE_PRIORITY &&
              lck_elem->remote_next_owner != NOBODY )
            lck_elem->cluster_priority++;
         grant_lock(lck_elem->local_next_owner, lck_elem);
         break;
      case REMOTE_CLUSTER:
         assert ( lck_elem->just_granted == SL_FALSE );
         grant_lock(lck_elem->remote_next_owner, lck_elem);
         lck_elem->just_granted = SL_TRUE;
         break;
      case NOWHERE:
      default: assert ( ! "Should never reach this point!" );
   }

   if ( initial_destination != destination )
   {
      request_tag_t tag;

      assert ( requester != NOBODY );
      assert ( initial_destination == LOCAL_THREAD ||
               initial_destination == LOCAL_CLUSTER );
      assert ( destination == LOCAL_CLUSTER || destination == REMOTE_CLUSTER );
      assert ( lck_elem->local_next_owner == NOBODY );
      assert ( lck_elem->remote_next_owner == NOBODY );
      assert ( lck_elem->status == UNKNOWN );

      if ( lck_elem->just_granted == SL_TRUE )
      {
         lck_elem->just_granted = SL_FALSE;
         tag = FORCE_TAG;
      }
      else
         tag = NO_TAG;

      request_lock_from_manager(requester, lck_elem->lock_id, tag);

      if ( initial_destination == LOCAL_THREAD )
         lck_elem->requested = SL_TRUE;
   }

   OUT;
   return;
}


/**********************************************************************/
/* receive a release notification from a node of the local cluster */
static void
recv_release_notification_func (void * const unused)
{
   lock_destination_t initial_dest, destination;
   token_lock_id_t lck_id;
   lock_list_elem_t * lck_elem;

   IN;

   pm2_unpack_byte(SEND_CHEAPER, RECV_CHEAPER, &lck_id,
                   sizeof(token_lock_id_t));
   pm2_rawrpc_waitdata();

   marcel_mutex_lock(&lock_mutex);
   lck_elem = seek_lock_in_hash(lck_id);
   marcel_mutex_unlock(&lock_mutex);
   assert ( lck_elem != NULL );

   TRACE("recv'd release notification for lock %d", lck_id);

   marcel_mutex_lock(&(lck_elem->mutex));
   update_expected_release(lck_elem, JUST_RECVD_NOTIF);
   decide_who_to_grant_to(lck_elem, &initial_dest, &destination);
   grant_lock_if_possible(lck_elem, initial_dest, destination);
   marcel_mutex_unlock(&(lck_elem->mutex));

   OUT;
   return;
}


/**********************************************************************/
/* create and initialize a lock entry in the hash table; the mutex
 * "lock_mutex" is assumed to be acquired.  The calling function must
 * set the following fields by itself: status. */
static lock_list_elem_t *
create_lock_entry (const token_lock_id_t lck_id)
{
   lock_list_elem_t * lck_elem;
   unsigned int hash_indx;
   dsm_node_t myself = pm2_self();

   IN;
   TRACE("creating entry for lock %d", lck_id);

   /* create a new lock data structure */
   lck_elem = (lock_list_elem_t *) tmalloc(sizeof(lock_list_elem_t));
   if ( lck_elem == NULL ) return NULL;

   /* common initialization */
   lck_elem->lock_id = lck_id;
   marcel_mutex_init(&(lck_elem->mutex), NULL);
   lck_elem->local_next_owner = lck_elem->remote_next_owner = NOBODY;
   lck_elem->requested = lck_elem->just_granted = SL_FALSE;
   marcel_cond_init(&(lck_elem->token_available), NULL);
   lck_elem->expected_remote_release = lck_elem->expected_local_release = 0;
   lck_elem->reservation = 0;
   lck_elem->notif_list = lck_elem->end_of_notif_list = NULL;
   lck_elem->owner_thread = NULL;
   lck_elem->thread_priority = lck_elem->cluster_priority = 0;
   lck_elem->call_acquire_func = SL_TRUE;

   /* initialization depending on whether I'm the manager or not */
   if ( myself == get_lock_manager(lck_id) )
   {
      const int cluster_number = topology_get_cluster_number();
      int i;

      lck_elem->last_cluster = topology_get_cluster_color(myself);
      lck_elem->last_requester = (dsm_node_t *) tmalloc(cluster_number *
                                                           sizeof(dsm_node_t));
      if ( lck_elem->last_requester == NULL )
      {
         tfree(lck_elem);
         return NULL;
      }
      for (i = 0; i < cluster_number; i++)
         lck_elem->last_requester[i] = NOBODY;
      lck_elem->last_requester[lck_elem->last_cluster] = myself;
   }
   else
   {
      lck_elem->last_requester = NULL;
      lck_elem->last_cluster = NO_COLOR;
   }

   /* insert this lock data structure into the hash table */
   hash_indx = lock_hash_func(lck_id);
   lck_elem->next = lock_hash[hash_indx];
   lock_hash[hash_indx] = lck_elem;

   OUT;
   return lck_elem;
}


/**********************************************************************/
/* simply send a request for a lock; the lock entry is assumed to be
 * locked */
static void
send_lock_request (const dsm_node_t to_node, dsm_node_t requester,
                   token_lock_id_t lck_id)
{
   IN;
   assert ( to_node < pm2_config_size() );
   assert ( to_node != NOBODY );
   assert ( requester < pm2_config_size() );
   assert ( requester != NOBODY );
   assert ( get_lock_manager(lck_id) == pm2_self() );

   TRACE("requesting lock %d from node %d for requester %d",
         lck_id, to_node, requester);

   pm2_rawrpc_begin(to_node, TOKEN_LOCK_REQUEST, NULL);
   pm2_pack_byte(SEND_CHEAPER, RECV_CHEAPER, &requester, sizeof(dsm_node_t));
   pm2_pack_byte(SEND_CHEAPER, RECV_CHEAPER, &lck_id, sizeof(token_lock_id_t));
   pm2_rawrpc_end();

   OUT;
}


/**********************************************************************/
/* register the given node as the (remote or local) next owner of the
 * lock token; the lock entry is assumed to be locked */
static void
register_next_owner (const dsm_node_t node, lock_list_elem_t * lck_elem)
{
   lock_destination_t initial_dest, destination;

   IN;

   assert ( node != NOBODY );
   assert ( node < pm2_config_size() );
   assert ( lck_elem != NULL );
   if ( topology_get_cluster_color(pm2_self()) ==
        topology_get_cluster_color(node) )
   {
      assert ( lck_elem->local_next_owner == NOBODY );
      TRACE("registering node %d as local next owner of lock %d", node,
            lck_elem->lock_id);
      lck_elem->local_next_owner = node;
   }
   else
   {
      assert ( lck_elem->remote_next_owner == NOBODY );
      TRACE("registering node %d as remote next owner of lock %d", node,
            lck_elem->lock_id);
      lck_elem->remote_next_owner = node;
   }
   decide_who_to_grant_to(lck_elem, &initial_dest, &destination);
   grant_lock_if_possible(lck_elem, initial_dest, destination);

   OUT;
   return;
}


/**********************************************************************/
/* server function called to obtain a lock token */
static void
lock_request_server_func (void * const unused)
{
   dsm_node_t requester;
   token_lock_id_t lck_id;
   lock_list_elem_t * lck_elem;

   IN;
   pm2_unpack_byte(SEND_CHEAPER, RECV_CHEAPER, &requester, sizeof(dsm_node_t));
   pm2_unpack_byte(SEND_CHEAPER, RECV_CHEAPER, &lck_id,
                   sizeof(token_lock_id_t));
   pm2_rawrpc_waitdata();

   TRACE("recv'd request for lock %d for requester %d", lck_id, requester);

   marcel_mutex_lock(&lock_mutex);
   lck_elem = seek_lock_in_hash(lck_id);
   marcel_mutex_unlock(&lock_mutex);
   assert ( lck_elem != NULL );

   marcel_mutex_lock(&(lck_elem->mutex));

   if ( lck_elem->just_granted == SL_TRUE )
   {
      assert ( lck_elem->status = UNKNOWN );
      request_lock_from_manager(requester, lck_id, FORCE_TAG);
      lck_elem->just_granted = SL_FALSE;
   }
   else
      register_next_owner(requester, lck_elem);

   marcel_mutex_unlock(&(lck_elem->mutex));

   OUT;
   return;
}


/**********************************************************************/
/* being granted the lock token after a full or partial release of the
 * lock. */
static void
recv_lock_token_func (void * const unused)
{
   token_lock_id_t lck_id;
   grant_state_t state;
   lock_list_elem_t * lck_elem;
   dsm_node_t remote_nxt;
   unsigned long int cluster_priority;

   IN;
   pm2_unpack_byte(SEND_CHEAPER, RECV_CHEAPER, &lck_id,
                   sizeof(token_lock_id_t));
   pm2_unpack_byte(SEND_CHEAPER, RECV_CHEAPER, &state, sizeof(grant_state_t));
   pm2_unpack_byte(SEND_CHEAPER, RECV_CHEAPER, &remote_nxt, sizeof(dsm_node_t));
   pm2_unpack_byte(SEND_CHEAPER, RECV_CHEAPER, &cluster_priority,
                   sizeof(unsigned long int));
   pm2_rawrpc_waitdata();
   assert ( state == INTRA  ||  state == INTER  ||  state == DOES_NOT_EXIST );

   marcel_mutex_lock(&lock_mutex);
   lck_elem = seek_lock_in_hash(lck_id);
   marcel_mutex_unlock(&lock_mutex);
   assert ( lck_elem != NULL );

   marcel_mutex_lock(&(lck_elem->mutex));
   assert ( lck_elem->status != LOCALLY_PRESENT );
   assert ( lck_elem->owner_thread == NULL );
   assert ( lck_elem->reservation > 0 );
   assert ( lck_elem->requested == SL_TRUE );
   lck_elem->requested = SL_FALSE;
   if ( state != DOES_NOT_EXIST )
   {
      lock_destination_t initial_dest, destination;

      lck_elem->status = LOCALLY_PRESENT;
      lck_elem->just_granted = SL_FALSE;
      lck_elem->thread_priority = 0;
      lck_elem->cluster_priority = cluster_priority;

      if ( remote_nxt != NOBODY )
      {
         assert ( lck_elem->remote_next_owner == NOBODY );
         assert ( topology_get_cluster_color(pm2_self()) !=
                  topology_get_cluster_color(remote_nxt) );
         lck_elem->remote_next_owner = remote_nxt;
      }

      if ( state == INTRA )   /* the lock was partially granted */
         update_expected_release(lck_elem, JUST_RECVD_TOKEN);
      else
      {
         assert ( lck_elem->expected_local_release == 0 );
         assert ( lck_elem->expected_remote_release == 0 );
      }

      decide_who_to_grant_to(lck_elem, &initial_dest, &destination);
      grant_lock_if_possible(lck_elem, initial_dest, destination);
   }
   else
   {
      assert ( lck_elem->status == NON_EXISTENT );
      TRACE("Bcast token available with status='NON_EXISTENT'", 0);
      marcel_cond_broadcast(&(lck_elem->token_available));
   }

   marcel_mutex_unlock(&(lck_elem->mutex));

   OUT;
   return;
}


/**********************************************************************/
/* send a msg to the requester notifying the requested lock token does
 * not exist */
static void
send_no_such_token (const dsm_node_t to_node, token_lock_id_t lck_id)
{
   dsm_node_t dummy_node = NOBODY;
   grant_state_t state = DOES_NOT_EXIST;
   unsigned long int dummy_cluster_priority = NO_PRIORITY;

   IN;
   assert ( to_node < pm2_config_size() );
   assert ( to_node != NOBODY );
   assert ( lck_id != TOKEN_LOCK_NONE );

   TRACE("sending DOES_NOT_EXIST msg to node %d for token %d", to_node, lck_id);
   pm2_rawrpc_begin(to_node, TOKEN_LOCK_RECV, NULL);
   pm2_pack_byte(SEND_CHEAPER, RECV_CHEAPER, &lck_id, sizeof(token_lock_id_t));
   pm2_pack_byte(SEND_CHEAPER, RECV_CHEAPER, &state, sizeof(grant_state_t));
   pm2_pack_byte(SEND_CHEAPER, RECV_CHEAPER, &dummy_node, sizeof(dsm_node_t));
   pm2_pack_byte(SEND_CHEAPER, RECV_CHEAPER, &dummy_cluster_priority,
                 sizeof(unsigned long int));
   pm2_rawrpc_end();

   OUT;
   return;
}


/**********************************************************************/
/* the lock server running on the manager */
static void
lock_manager_server_func (void * const unused)
{
   dsm_node_t requester;
   token_lock_id_t lck_id;
   lock_list_elem_t * lck_elem;
   request_tag_t req_tag;

   IN;
   pm2_unpack_byte(SEND_CHEAPER, RECV_CHEAPER, &requester, sizeof(dsm_node_t));
   pm2_unpack_byte(SEND_CHEAPER, RECV_CHEAPER, &lck_id,
                   sizeof(token_lock_id_t));
   pm2_unpack_byte(SEND_CHEAPER, RECV_CHEAPER, &req_tag, sizeof(request_tag_t));
   pm2_rawrpc_waitdata();

   assert ( requester != NOBODY );
   assert ( requester < pm2_config_size() );
   assert ( lck_id != TOKEN_LOCK_NONE );
   assert ( req_tag == NO_TAG  ||  req_tag == FORCE_TAG );
   assert ( pm2_self() == get_lock_manager(lck_id) );
   TRACE("recv'd request for lock %d for requester %d with tag '%s'",
         lck_id, requester, req_tag == NO_TAG ? "NO_TAG" : "FORCE_TAG");

   marcel_mutex_lock(&lock_mutex);
   lck_elem = seek_lock_in_hash(lck_id);
   marcel_mutex_unlock(&lock_mutex);

   if ( lck_elem == NULL )
   {
      send_no_such_token(requester, lck_id);
      OUT;
      return;
   }

   marcel_mutex_lock(&(lck_elem->mutex));

   if ( lck_elem->status == NON_EXISTENT )
      send_no_such_token(requester, lck_id);
   else
   {
      const int requester_clr = topology_get_cluster_color(requester);

      assert ( requester_clr != NO_COLOR );
      assert ( requester_clr < topology_get_cluster_number() );

      if ( req_tag == FORCE_TAG ||
           lck_elem->last_requester[requester_clr] == NOBODY )
      {
         assert ( lck_elem->last_cluster < topology_get_cluster_number() );
         assert ( lck_elem->last_cluster != NO_COLOR );
         assert ( lck_elem->last_cluster != requester_clr );
         assert ( lck_elem->last_requester[lck_elem->last_cluster] != NOBODY );

         send_lock_request(lck_elem->last_requester[lck_elem->last_cluster],
                           requester, lck_id);
         TRACE("last cluster = %d for lock %d", requester_clr,
               lck_elem->lock_id);
         lck_elem->last_cluster = requester_clr;
         if ( lck_elem->last_requester[requester_clr] == NOBODY )
            lck_elem->last_requester[requester_clr] = requester;
      }
      else
      {
         assert ( requester != lck_elem->last_requester[requester_clr] );
         send_lock_request(lck_elem->last_requester[requester_clr], requester,
                           lck_id);
         lck_elem->last_requester[requester_clr] = requester;
         TRACE("last requester of cluster %d becoming node %d",
               requester_clr, requester);
      }
   }

   marcel_mutex_unlock(&(lck_elem->mutex));

   OUT;
   return;
}


/**********************************************************************/
/* PUBLIC FUNCTIONS                                                   */
/**********************************************************************/

/**********************************************************************/
/* this function initializes the data structures for the locks; this
 * should be called by dsm_pm2_init(); there's only one thread per
 * process executing this function. */
int
token_lock_initialization (void)
{
   int i;

   IN;
   local_lock_number = 0;
   max_local_thread_acquire = max_local_cluster_acquire = INFINITE_PRIORITY;
   marcel_mutex_init(&lock_mutex, NULL);

   /* the hash table is initially empty */
   for (i = 0; i < LOCK_HASH_SIZE; i++)
      lock_hash[i] = NULL;

   OUT;
   return DSM_SUCCESS;
}


/**********************************************************************/
/* this function cleans up the data structures for the locks; there's
 * only one thread per process executing this function; this function
 * is called by dsm_pm2_exit(). */
int
token_lock_finalization (void)
{
   int i;
   const dsm_node_t myself = pm2_self();

   IN;
   marcel_mutex_destroy(&lock_mutex);

   /* free memory from the lock hash table */
   for (i = 0; i < LOCK_HASH_SIZE; i++)
   {
      lock_list_elem_t * lck_list = lock_hash[i];

      while ( lck_list != NULL )
      {
         lock_list_elem_t * const save = lck_list;
         node_elem_t * lst;

         lck_list = save->next;
         marcel_cond_destroy(&(save->token_available));
         marcel_mutex_destroy(&(save->mutex));
         if ( get_lock_manager(save->lock_id) == myself )
            tfree(save->last_requester);
         lst = save->notif_list;
         while ( lst != NULL )
         {
            node_elem_t * const l = lst->next;

            lst = lst->next;
            tfree(l);
         }
         tfree(save);
      }
      lock_hash[i] = NULL;
   }

   local_lock_number = 0;
   max_local_thread_acquire = max_local_cluster_acquire = INFINITE_PRIORITY;

   OUT;
   return DSM_SUCCESS;
}


/**********************************************************************/
/* initialize a new lock; The default manager of the lock is myself. */
int
token_lock_init (token_lock_id_t * const lck_id)
{
   int ret_val = DSM_SUCCESS;

   IN;
   /* in case the attributes (future extensions) specify another manager
    * than pm2_self(), do an RPC onto the other node (wanted as the
    * owner) to get a lock_id */
   {
      lock_list_elem_t * lck_elem;

      /* initialize this new lock data structure */
      /* the local node can only deliver lock_id's which are congruent
       * to "pm2_self()" modulo "pm2_config_size()"... */
      marcel_mutex_lock(&lock_mutex);
      *lck_id = local_to_global_lock_id(local_lock_number);
      lck_elem = seek_lock_in_hash(*lck_id);
      if ( lck_elem == NULL )
         lck_elem = create_lock_entry(*lck_id);
      else
         assert ( lck_elem->status == NON_EXISTENT );
      if ( lck_elem == NULL )
      {
         TRACE("not enough memory to create lock token %d", *lck_id);
         ret_val = DSM_ERR_MEMORY;
         *lck_id = TOKEN_LOCK_NONE;
      }
      else
      {
         local_lock_number++;
         lck_elem->status = LOCALLY_PRESENT;
      }
      marcel_mutex_unlock(&lock_mutex);
   }

   OUT;
   return ret_val;
}


/**********************************************************************/
/* acquire a DSM lock */
int
token_lock (const token_lock_id_t lck_id)
{
   int ret_val = DSM_SUCCESS;
   lock_list_elem_t * lck_elem;
   bool_t call_acquire_func;

   IN;
   TRACE("want to acquire lock %d", lck_id);
   if ( lck_id == TOKEN_LOCK_NONE )
   {
      OUT;
      return DSM_ERR_ILLEGAL;
   }

   marcel_mutex_lock(&lock_mutex);
   lck_elem = seek_lock_in_hash(lck_id);

   /* If the lock is not registered locally or if I'm not the owner of
    * this lock, then request it from the manager. */
   if ( lck_elem == NULL )   /* the lock is not registered locally */
   {
      if ( pm2_self() == get_lock_manager(lck_id) )
      {
         TRACE("lock %d not initialized", lck_id);
         ret_val = DSM_ERR_NOT_INIT;
      }
      else
      {
         lck_elem = create_lock_entry(lck_id);
         if ( lck_elem == NULL )
         {
            TRACE("not enough memory to create entry for lock %d", lck_id);
            ret_val = DSM_ERR_MEMORY;
         }
         else
            lck_elem->status = NON_EXISTENT;
      }
   }
   marcel_mutex_unlock(&lock_mutex);

   if ( ret_val != DSM_SUCCESS )
   {
      OUT;
      return ret_val;
   }
   assert ( lck_elem != NULL );

   marcel_mutex_lock(&(lck_elem->mutex));
   /* reserve the lock locally so that the token could not be passed
    * to another process */
   lck_elem->reservation++;
   if ( lck_elem->requested == SL_FALSE && lck_elem->status != LOCALLY_PRESENT )
   {
      request_tag_t tag;

      if ( lck_elem->just_granted == SL_TRUE )
      {
         tag = FORCE_TAG;
         lck_elem->just_granted = SL_FALSE;
      }
      else
         tag = NO_TAG;

      request_lock_from_manager(pm2_self(), lck_elem->lock_id, tag);
      lck_elem->requested = SL_TRUE;
   }

   while ( lck_elem->requested == SL_TRUE || lck_elem->owner_thread != NULL )
   {
      TRACE("waiting for token %d", lck_id);
      marcel_cond_wait(&(lck_elem->token_available), &(lck_elem->mutex));
   }

   assert ( lck_elem->status != UNKNOWN );
   assert ( lck_elem->owner_thread == NULL );

   assert ( lck_elem->reservation > 0 );
   if ( lck_elem->status != NON_EXISTENT )
      /* The lock is free: acquire it! */
      update_expected_release(lck_elem, JUST_ACQUIRED);
   else
      ret_val = DSM_ERR_NOT_INIT;

   call_acquire_func = lck_elem->call_acquire_func;
   lck_elem->call_acquire_func = SL_TRUE;
   lck_elem->reservation--;

   marcel_mutex_unlock(&(lck_elem->mutex));

   if ( ret_val == DSM_SUCCESS && call_acquire_func == SL_TRUE )
   {
      const dsm_acq_action_t acquire_func =
                              dsm_get_acquire_func(dsm_get_default_protocol());

      TRACE("lock %d acquired, calling acquire consistency function", lck_id);
      /* protocol specific action at acquire time */
      if ( acquire_func != NULL ) (*acquire_func)(lck_id);
   }

   OUT;
   return ret_val;
}


/**********************************************************************/
/* partial release of a DSM lock */
int
token_partial_unlock (const token_lock_id_t lck_id)
{
   int ret_val = DSM_SUCCESS;
   lock_list_elem_t * lck_elem;

   IN;

   TRACE("want to partially release lock %d", lck_id);
   if ( lck_id == TOKEN_LOCK_NONE )
   {
      OUT;
      return DSM_ERR_ILLEGAL;
   }

   marcel_mutex_lock(&lock_mutex);
   lck_elem = seek_lock_in_hash(lck_id);
   marcel_mutex_unlock(&lock_mutex);

   if ( lck_elem == NULL )   /* lock not acquired */
   {
      TRACE("trying to release partially lock %d while no entry", lck_id);
      OUT;
      return DSM_ERR_ILLEGAL;
   }

   marcel_mutex_lock(&(lck_elem->mutex));

   if ( lck_elem->owner_thread != marcel_self() )
   {
      TRACE("trying to release partially lock %d while not acquired", lck_id);
      ret_val = DSM_ERR_ILLEGAL;
   }
   else
   {
      lock_destination_t initial_dest, destination;

      assert ( lck_elem->expected_local_release > 0 );
      assert ( lck_elem->requested == SL_FALSE );
      assert ( lck_elem->status == LOCALLY_PRESENT );
      lck_elem->owner_thread = NULL;
      decide_who_to_grant_to(lck_elem, &initial_dest, &destination);
      grant_lock_if_possible(lck_elem, initial_dest, destination);
   }

   marcel_mutex_unlock(&(lck_elem->mutex));

   OUT;
   return ret_val;
}


/**********************************************************************/
/* release a DSM lock. */
int
token_unlock (const token_lock_id_t lck_id)
{
   lock_list_elem_t * lck_elem;
   lock_destination_t initial_dest, destination;

   IN;
   TRACE("want to (fully) release lock %d", lck_id);
   if ( lck_id == TOKEN_LOCK_NONE )
   {
      OUT;
      return DSM_ERR_ILLEGAL;
   }

   marcel_mutex_lock(&lock_mutex);
   lck_elem = seek_lock_in_hash(lck_id);
   marcel_mutex_unlock(&lock_mutex);
   if ( lck_elem == NULL )   /* lock not acquired */
   {
      TRACE("trying to (fully) release lock %d while no entry", lck_id);
      OUT;
      return DSM_ERR_ILLEGAL;
   }

   marcel_mutex_lock(&(lck_elem->mutex));

   if ( lck_elem->expected_local_release == 0 ||
        lck_elem->status == NON_EXISTENT )
   {
      /* lock already released or not initialized */
      TRACE("lock %d already released or not initialized", lck_id);
      marcel_mutex_unlock(&(lck_elem->mutex));
      OUT;
      return DSM_ERR_ILLEGAL;
   }

   {
      const marcel_t save_owner_thread = lck_elem->owner_thread;

      lck_elem->expected_local_release--;
      if ( lck_elem->owner_thread == marcel_self() )
         lck_elem->owner_thread = NULL;
      decide_who_to_grant_to(lck_elem, &initial_dest, &destination);
      lck_elem->expected_local_release++;
      lck_elem->owner_thread = save_owner_thread;
   }

   if ( destination == LOCAL_THREAD )
   {
      lck_elem->call_acquire_func = SL_FALSE;
      update_expected_release(lck_elem, JUST_RELEASED);
   }
   else
   {
      const dsm_rel_action_t release_func =
                           dsm_get_release_func(dsm_get_default_protocol());

      marcel_mutex_unlock(&(lck_elem->mutex));
      TRACE("calling release consistency function for lock %d", lck_id);
      if ( release_func != NULL ) (*release_func)(lck_id);
      marcel_mutex_lock(&(lck_elem->mutex));
      lck_elem->call_acquire_func = SL_TRUE;
      update_expected_release(lck_elem, JUST_RELEASED);
      decide_who_to_grant_to(lck_elem, &initial_dest, &destination);
   }

   grant_lock_if_possible(lck_elem, initial_dest, destination);

   marcel_mutex_unlock(&(lck_elem->mutex));

   OUT;
   return DSM_SUCCESS;
}


/**********************************************************************/
/* service function called by RPC to obtain a lock token */
void
token_lock_request_server (void)
{
   pm2_service_thread_create(lock_request_server_func, NULL);
   return;
}


/**********************************************************************/
/* being notified of the full release of a lock which was partially
 * granted */
void
token_lock_recv_server (void)
{
   pm2_service_thread_create(recv_lock_token_func, NULL);
   return;
}


/**********************************************************************/
/* RPC server to receive a release notification */
void
token_lock_recv_release_notification_server (void)
{
   pm2_service_thread_create(recv_release_notification_func, NULL);
   return;
}


/**********************************************************************/
/* lock manager server */
void
token_lock_manager_server (void)
{
   pm2_service_thread_create(lock_manager_server_func, NULL);
   return;
}


/**********************************************************************/
/* set the maximum number of consecutive acquisitions of a lock within
 * a process; this function must be called after pm2_init(). */
int
set_thread_priority_level (const unsigned long int lvl)
{
   max_local_thread_acquire = lvl;
   return DSM_SUCCESS;
}


/**********************************************************************/
/* set the maximum number of consecutive acquisitions of a lock within
 * a cluster; this function must be called after pm2_init(). */
int
set_cluster_priority_level (const unsigned long int lvl)
{
   max_local_cluster_acquire = lvl;
   return DSM_SUCCESS;
}

