
/*
 * CVS Id: $Id: hierarch_lock.c,v 1.1 2002/10/11 18:03:51 slacour Exp $
 */

/* Distributed management of the locks.  Each lock has a (central)
 * fixed manager and an owner (token owner).  The lock manager can be
 * determined using the ID of the lock.  The manager always knows who
 * currently holds the token (lock). */

#include <stdlib.h>
#include "hierarch_lock.h"
#include "hierarch_topology.h"
#include "pm2.h"


/* The program might be slightly slower to run if the correctness of
 * the arguments of the functions called by the user application were
 * checked.  Those checks are replaced with assertions "assert". */


/**********************************************************************/
/* TRACING AND DEBUGGING SYSTEM                                       */
/**********************************************************************/

#undef TRCSL
#undef DBGSL

/* remove all assertions "assert" */
#define NDEBUG
#include <assert.h>

#undef PROFIN
#undef PROFOUT
#define PROFIN(x) ((void)0)
#define PROFOUT(x) ((void)0)

#if defined(TRCSL) || defined(DBGSL)
static int depth = 0;
static void spaces (const int i) { int j; for (j=0; j<i; j++) tfprintf(stderr, "   "); return; }
#endif

#undef IN
#undef OUT
#ifdef DBGSL
#define IN { spaces(depth++); tfprintf(stderr, "--> [%d:%x:%s]\n", pm2_self(), marcel_self(), __FUNCTION__); }
#define OUT { spaces(--depth); tfprintf(stderr, "[%d:%x:%s] -->\n", pm2_self(), marcel_self(), __FUNCTION__); }
#else
#define IN ((void)0)
#define OUT ((void)0)
#endif

#undef TRACE
#ifdef TRCSL
#define TRACE(strng, args...) { spaces(depth); tfprintf(stderr, "[%d:%x:%s] " strng "\n", pm2_self(), marcel_self(), __FUNCTION__, ##args); }
#else
#define TRACE(...) ((void)0)
#endif

#define STRACE(strng, args...) { tfprintf(stderr, "[%d:%x:%s] *** " strng "\n", pm2_self(), marcel_self(), __FUNCTION__, ##args); }


/**********************************************************************/
/* PRIVATE DATA STRUCTURES                                            */
/**********************************************************************/

/* tag for the lock requests: either no tag, or a tag meaning the lock
 * request was received too late, just after granting the lock to a
 * node in another cluster */
typedef enum { NO_TAG, TOO_LATE, FORCE_TAG } request_tag_t;

/* a boolean type */
typedef enum { SL_TRUE, SL_FALSE } bool_t;

/* a lock can be fully granted (INTERcluster) or partially granted
 * (INTRAcluster) */
typedef enum { INTER, INTRA } grant_state_t;

/* possible statuses of a lock */
typedef enum
{
   UNKNOWN,  /* I'm not the owner of the lock */
   FREE,     /* I own the lock, it's free or partially released */
   BUSY,     /* I own the lock, it is busy */
   REQUESTED /* the lock has already been requested but has not arrived yet */
} lock_status_t;

/* linked list of nodes (to which I must send a release notification) */
typedef struct node_list_struct node_list_elem_t;
typedef node_list_elem_t * node_list_t;
struct node_list_struct
{
   dsm_node_t node;         /* node number */
   node_list_elem_t * next; /* next element of the linked list */
};

/* linked list of locks */
typedef struct lock_list_struct lock_list_elem;
typedef lock_list_elem * lock_list_t;
struct lock_list_struct
{
   hierarch_lock_id_t lock_id; /* global unique ID of the lock */
   marcel_mutex_t mutex;      /* mutex to handle any field of the lock entry */
   lock_status_t status;      /* free? busy? requested? unknown? */

   /* lock requester, in the local cluster */
   dsm_node_t local_next_owner;
   /* lock requester, in a remote cluster */
   dsm_node_t remote_next_owner;

   /* a boolean which becomes true when the lock token is granted to
    * another cluster; becomes false when receiving the lock token, or
    * when forwarding a request that comes too late (with
    * tag==TOO_LATE) to the lock manager */
   bool_t just_granted;

   /* last requester of the lock for each cluster; relevant only for
    * the lock manager */
   dsm_node_t *last_requester;

   /* if true: the last requester of the cluster has granted the lock
    * because it has just sent a lock request to the manager; relevant
    * only on the lock manager */
   bool_t *force_next_request;

   /* the last cluster which requested the lock; relevant for the lock
    * manager only */
   unsigned int last_cluster;

   int reservation;  /* number of local threads which reserved the
                        lock (granting a lock to a local thread is
                        preferred to granting it to another node) */

   /* signal the arrival of a lock token */
   marcel_cond_t token_arrival;
   /* signal a local release: the lock can be granted to a thread of
    * the local node */
   marcel_cond_t local_release;

   /* number of release signals expected from other nodes (of the
    * local cluster) */
   int expected_remote_release;

   /* number of diff acks expected */
   int expected_diff_acks;
   /* number of invalidation acks expected from local nodes */
   int expected_local_inv_acks;
   /* number of invalidation acks expected from remote nodes */
   int expected_remote_inv_acks;

   /* this boolean is set to true when I am the originator of a
    * partial grant.  False otherwise.  Set to true when I grant a
    * partial lock to a remote cluster with no remote release
    * notifications expected (only local release are expected).  Set
    * to false when I send a release notification. */
   bool_t partial_release_originator;

   /* list of nodes (in the local cluster) which were granted the lock
    * token while releases were expected (lock partially granted). */
   node_list_t notif_list;
   /* pointer to the last element of the previous linked list */
   node_list_t end_of_notif_list;

   lock_list_elem * next; /* next lock in the linked list */
};


/**********************************************************************/
/* GLOBAL PRIVATE VARIABLES */
/**********************************************************************/

/* hash table for the lock data structures; the pm2_config_size()
 * should not be a multiple of LOCK_HASH_SIZE.  This particular point
 * (the hash table size) could be improved, taking the
 * pm2_config_size() into account. */
#define LOCK_HASH_SIZE 10
static lock_list_t lock_hash[LOCK_HASH_SIZE];

/* Total number of locks initialized locally (this is the number of
 * locks this node manages). */
static hierarch_lock_id_t local_lock_number = 0;

/* Lock for any operation on the lock data structures */
static marcel_mutex_t lock_mutex;

/* no node */
static const dsm_node_t NOBODY = (dsm_node_t)-1;

/* maximum number of lock acquisitions inside a given process to the
 * detriment of other nodes (0 to disable the local thread priority;
 * -1 to mean "no limit", "infinity") */
const int max_local_thread_acquire = -1;

/* maximum number of lock acquisitions inside a cluster to the
 * detriment of remote nodes (0 to disable the local node priority; -1
 * to mean "no limit", "infinity") */
const int max_local_node_acquire = -1;


/**********************************************************************/
/* GLOBAL PUBLIC VARIABLES */
/**********************************************************************/

/* no lock */
const hierarch_lock_id_t HIERARCH_NO_LOCK = (hierarch_lock_id_t)(-1);


/**********************************************************************/
/* PRIVATE FUNCTIONS */
/**********************************************************************/

/**********************************************************************/
/* convert a local lock_id into a global (unique) lock_id */
static hierarch_lock_id_t
local_to_global_lock_id (const hierarch_lock_id_t local_lck_id)
{
   return (hierarch_lock_id_t) (local_lck_id * pm2_config_size() + pm2_self());
}


/**********************************************************************/
/* return the manager of a global lock ID */
static dsm_node_t
get_lock_manager (const hierarch_lock_id_t global_lck_id)
{
   return (dsm_node_t) ( global_lck_id % pm2_config_size() );
}


/**********************************************************************/
/* return the hash bucket for the lock hash table */
static unsigned int
lock_hash_func (const hierarch_lock_id_t lck_id)
{
   return (unsigned int) (lck_id % LOCK_HASH_SIZE);
}


/**********************************************************************/
/* return a pointer to the lock structure corresponding to the global
 * lock id given as a parameter in the lock hash table; return NULL if
 * the lock id is not found; the mutex "lock_mutex" is assumed to be
 * acquired. */
static lock_list_elem *
seek_lock_in_hash (const hierarch_lock_id_t lck_id)
{
   unsigned int hash_bucket;
   lock_list_t lock_list;

   hash_bucket = lock_hash_func(lck_id);
   lock_list = lock_hash[hash_bucket];

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
insert_into_node_list (node_list_t * list, node_list_t * end_of_list,
                       const dsm_node_t node)
{
   node_list_elem_t * elem;

   IN;
   TRACE("add node %d to list", node);

   elem = (node_list_t) tmalloc (sizeof(node_list_elem_t));
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
grant_lock (const dsm_node_t to_node, lock_list_elem * const lck_elem)
{
   grant_state_t state;
   dsm_node_t rmt_nxt;

   IN;
   assert ( lck_elem->just_granted == SL_FALSE );
   assert ( to_node != pm2_self() );
   assert ( to_node != NOBODY );
   assert ( lck_elem->reservation == 0 );
   assert ( lck_elem->status == FREE );
   assert ( lck_elem->expected_diff_acks == 0 );
   assert ( lck_elem->expected_local_inv_acks == 0 );

   if ( lck_elem->expected_remote_inv_acks != 0  ||
        lck_elem->expected_remote_release > 0 )   /* grant partially */
   {
      assert ( lck_elem->expected_remote_inv_acks > 0 );
      assert ( cluster_colors[to_node] == cluster_colors[pm2_self()] );
      state = INTRA;   /* intra-cluster only */
      /* add the node into the list of nodes to which i'll forward the
       * full release signal */
      insert_into_node_list(&(lck_elem->notif_list),
                            &(lck_elem->end_of_notif_list),
                            to_node);
      if ( lck_elem->expected_remote_release == 0 )
         lck_elem->partial_release_originator = SL_TRUE;
   }
   else   /* fully grant the lock token */
   {
      assert ( lck_elem->notif_list == NULL );
      state = INTER;
      if ( cluster_colors[to_node] != cluster_colors[pm2_self()] )
         lck_elem->just_granted = SL_TRUE;
   }

   pm2_rawrpc_begin(to_node, HIERARCH_RECV_LOCK_TOKEN, NULL);
   pm2_pack_byte(SEND_CHEAPER, RECV_CHEAPER, &(lck_elem->lock_id),
                 sizeof(hierarch_lock_id_t));
   pm2_pack_byte(SEND_CHEAPER, RECV_CHEAPER, &state, sizeof(grant_state_t));
   if ( to_node != lck_elem->remote_next_owner )
      rmt_nxt = lck_elem->remote_next_owner;
   else
      rmt_nxt = NOBODY;
   pm2_pack_byte(SEND_CHEAPER, RECV_CHEAPER, &rmt_nxt, sizeof(dsm_node_t));
   pm2_rawrpc_end();
   TRACE("grant lock %d to node %d with state '%s' and remote_next %d",
         lck_elem->lock_id, to_node, state == INTER ? "inter" : "intra",
         rmt_nxt);

   lck_elem->status = UNKNOWN;
   lck_elem->local_next_owner = lck_elem->remote_next_owner = NOBODY;

   OUT;
   return;
}


/**********************************************************************/
/* send a notification of release to the first node from
 * the node_list; the lock entry is assumed to be locked. */
static void
send_release_notifications (lock_list_elem * const lck_elem)
{
   IN;

   if ( lck_elem->notif_list != NULL )
   {
      node_list_t save = lck_elem->notif_list;

      assert ( save->node != pm2_self() );
      pm2_rawrpc_begin(save->node, HIERARCH_RELEASE_NOTIFICATION, NULL);
      pm2_pack_byte(SEND_CHEAPER, RECV_CHEAPER, &(lck_elem->lock_id),
            sizeof(hierarch_lock_id_t));
      pm2_rawrpc_end();
      TRACE("sending release notification to node %d for lock %d",
            save->node, lck_elem->lock_id);
      lck_elem->notif_list = save->next;
      if ( lck_elem->notif_list == NULL )   /* empty list */
         lck_elem->end_of_notif_list = NULL;
      tfree(save);
   }

   OUT;
   return;
}


/**********************************************************************/
/* if the requester is in the same cluster as I, then I must wait
 * until the lock is FREE and there are no local reservations.  If the
 * requester is in another cluster than mine, then wait for the lock
 * to be FREE, with no local reservations and no
 * expected_remote_release and no expected_local_release; the lock
 * entry is assumed to be locked. */
static void
grant_lock_if_possible (lock_list_elem * const lck_elem)
{
   IN;
   assert ( lck_elem != NULL );
   if ( lck_elem->status != FREE  ||  lck_elem->reservation != 0  ||
        lck_elem->expected_diff_acks != 0  ||
        lck_elem->expected_local_inv_acks != 0 )
   {
      OUT;
      return;
   }

   TRACE("try to grant lock %d; rmt_nxt=%d, lcl_nxt=%d, rmt_rel=%d",
         lck_elem->lock_id, lck_elem->remote_next_owner,
         lck_elem->local_next_owner, lck_elem->expected_remote_release);

   /* granting the lock to a node in the local cluster is prefered to
    * granting it to a node in a remote cluster */
   if ( lck_elem->local_next_owner != NOBODY )
      grant_lock(lck_elem->local_next_owner, lck_elem);
   else if ( lck_elem->remote_next_owner != NOBODY  &&
             lck_elem->expected_remote_inv_acks == 0 &&
             lck_elem->expected_remote_release == 0 )
      grant_lock(lck_elem->remote_next_owner, lck_elem);

   assert ( lck_elem->local_next_owner == NOBODY );

   OUT;
   return;
}


/**********************************************************************/
/* receive a release notification from a node of the local cluster */
static void
recv_release_notification_func (void * unused)
{
   hierarch_lock_id_t lck_id;
   lock_list_elem * lck_elem;

   IN;

   pm2_unpack_byte(SEND_CHEAPER, RECV_CHEAPER, &lck_id,
                   sizeof(hierarch_lock_id_t));
   pm2_rawrpc_waitdata();

   marcel_mutex_lock(&lock_mutex);
   lck_elem = seek_lock_in_hash(lck_id);
   marcel_mutex_unlock(&lock_mutex);
   assert ( lck_elem != NULL );

   marcel_mutex_lock(&(lck_elem->mutex));
   assert ( lck_elem->partial_release_originator == SL_FALSE );
   lck_elem->expected_remote_release--;
   TRACE("recv'd release notification for lock %d, rmt_rel=%d",
         lck_id, lck_elem->expected_remote_release);
   if ( lck_elem->expected_remote_release > 0  ||
        ( lck_elem->expected_remote_release == 0 &&
          lck_elem->expected_diff_acks == 0 &&
          lck_elem->expected_local_inv_acks == 0 ) )
      send_release_notifications(lck_elem);
   grant_lock_if_possible(lck_elem);
   marcel_mutex_unlock(&(lck_elem->mutex));

   OUT;
   return;
}


/**********************************************************************/
/* create and initialize a lock entry in the hash table; the mutex
 * "lock_mutex" is assumed to be acquired. */
static lock_list_elem *
create_lock_entry (const hierarch_lock_id_t lck_id)
{
   lock_list_elem * lck_elem;
   unsigned int hash_indx;

   IN;
   TRACE("creating entry for lock %d", lck_id);
   /* create a new lock data structure */
   lck_elem = (lock_list_elem *) tmalloc(sizeof(lock_list_elem));
   if ( lck_elem == NULL )
   {
      perror(__FUNCTION__);
      abort();
   }

   lck_elem->lock_id = lck_id;
   marcel_mutex_init(&(lck_elem->mutex), NULL);
   lck_elem->status = UNKNOWN;
   lck_elem->local_next_owner = lck_elem->remote_next_owner = NOBODY;
   lck_elem->partial_release_originator = lck_elem->just_granted = SL_FALSE;
   lck_elem->force_next_request = lck_elem->last_requester = NULL;
   lck_elem->expected_diff_acks = lck_elem->reservation = 0;
   lck_elem->expected_local_inv_acks = lck_elem->expected_remote_inv_acks = 0;
   marcel_cond_init(&(lck_elem->token_arrival), NULL);
   marcel_cond_init(&(lck_elem->local_release), NULL);
   lck_elem->expected_remote_release = 0;
   lck_elem->notif_list = lck_elem->end_of_notif_list = NULL;

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
                   lock_list_elem * lck_elem)
{
   request_tag_t tag;
   const unsigned int color = cluster_colors[to_node];

   IN;
   assert ( get_lock_manager(lck_elem->lock_id) == pm2_self() );
   assert ( color < cluster_number );

   if ( lck_elem->force_next_request[color] == SL_TRUE )
   {
      tag = FORCE_TAG;
      lck_elem->force_next_request[color] = SL_FALSE;
   }
   else
      tag = NO_TAG;

   TRACE("requesting lock %d from node %d for requester %d with tag '%s'",
         lck_elem->lock_id, to_node, requester,
         tag == NO_TAG ? "NO_TAG" : "FORCE_TAG");

   pm2_rawrpc_begin(to_node, HIERARCH_REQUEST_LOCK, NULL);
   pm2_pack_byte(SEND_CHEAPER, RECV_CHEAPER, &requester, sizeof(dsm_node_t));
   pm2_pack_byte(SEND_CHEAPER, RECV_CHEAPER, &(lck_elem->lock_id),
                 sizeof(hierarch_lock_id_t));
   pm2_pack_byte(SEND_CHEAPER, RECV_CHEAPER, &tag, sizeof(request_tag_t));
   pm2_rawrpc_end();

   OUT;
}


/**********************************************************************/
/* send a request for a lock with a given tag to the manager; the lock
 * entry is assumed to be locked */
static void
request_lock_from_manager (dsm_node_t requester, hierarch_lock_id_t lck_id,
                           request_tag_t tag)
{
   IN;

   TRACE("requesting lock %d from manager for requester %d with tag '%s'",
         lck_id, requester, tag == NO_TAG ? "NO_TAG" : "TOO_LATE");

   pm2_rawrpc_begin(get_lock_manager(lck_id),
                    HIERARCH_LOCK_MANAGER_SERVER, NULL);
   pm2_pack_byte(SEND_CHEAPER, RECV_CHEAPER, &requester, sizeof(dsm_node_t));
   pm2_pack_byte(SEND_CHEAPER, RECV_CHEAPER, &lck_id,
                 sizeof(hierarch_lock_id_t));
   pm2_pack_byte(SEND_CHEAPER, RECV_CHEAPER, &tag, sizeof(request_tag_t));
   pm2_rawrpc_end();

   OUT;
}


/**********************************************************************/
/* register the given node as the (remote or local) next owner of the
 * lock token; the lock entry is assumed to be locked */
static void
register_next_owner (const dsm_node_t node, lock_list_elem * lck_elem)
{
   IN;

   assert ( lck_elem->status != UNKNOWN );

   if ( cluster_colors[pm2_self()] == cluster_colors[node] )
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
   grant_lock_if_possible(lck_elem);

   OUT;
   return;
}


/**********************************************************************/
/* send a lock token request to the last cluster which requested the
 * lock token; the lock entry is assumed to be locked */
static void
send_request_to_current_owner_cluster (dsm_node_t requester,
                                       lock_list_elem * lck_elem)
{
   const unsigned int requester_color = cluster_colors[requester];

   IN;
   assert ( get_lock_manager(lck_elem->lock_id) == pm2_self() );
   assert ( requester_color < cluster_number );
   assert ( lck_elem->last_cluster < cluster_number );
   assert ( lck_elem->last_cluster != requester_color );
   assert ( lck_elem->last_requester[lck_elem->last_cluster] != NOBODY );
   assert ( lck_elem->last_requester[lck_elem->last_cluster] != requester );

   send_lock_request(lck_elem->last_requester[lck_elem->last_cluster],
                     requester, lck_elem);
   TRACE("last cluster = %d", requester_color);
   lck_elem->last_cluster = requester_color;

   OUT;
   return;
}


/**********************************************************************/
/* server function called to obtain a lock token */
static void
lock_request_server_func (void *unused)
{
   dsm_node_t requester;
   hierarch_lock_id_t lck_id;
   lock_list_elem * lck_elem;
   request_tag_t tag;

   IN;
   pm2_unpack_byte(SEND_CHEAPER, RECV_CHEAPER, &requester, sizeof(dsm_node_t));
   pm2_unpack_byte(SEND_CHEAPER, RECV_CHEAPER, &lck_id,
                   sizeof(hierarch_lock_id_t));
   pm2_unpack_byte(SEND_CHEAPER, RECV_CHEAPER, &tag, sizeof(request_tag_t));
   pm2_rawrpc_waitdata();

   assert ( tag == NO_TAG  ||  tag == FORCE_TAG );
   TRACE("recv'd request for lock %d for requester %d with tag '%s'",
         lck_id, requester, tag == NO_TAG ? "NO_TAG" : "FORCE_TAG");

   marcel_mutex_lock(&lock_mutex);
   lck_elem = seek_lock_in_hash(lck_id);
   marcel_mutex_unlock(&lock_mutex);
   assert ( lck_elem != NULL );

   marcel_mutex_lock(&(lck_elem->mutex));

   if ( lck_elem->just_granted == SL_TRUE  &&  tag == NO_TAG )
   {
      assert ( lck_elem->status == UNKNOWN || lck_elem->status == REQUESTED );
      assert ( lck_elem->local_next_owner == NOBODY );
      assert ( lck_elem->remote_next_owner == NOBODY );
      request_lock_from_manager(requester, lck_id, TOO_LATE);
   }
   else
      register_next_owner(requester, lck_elem);
   lck_elem->just_granted = SL_FALSE;

   marcel_mutex_unlock(&(lck_elem->mutex));

   OUT;
   return;
}


/**********************************************************************/
/* send a lock request and wait for its arrival; the
 * lock entry is assumed to exist in the lock hash table, and the
 * mutex on the lock entry is supposed to be acquired. */
static void
request_lock (lock_list_elem * const lck_elem)
{
   IN;
   assert ( lck_elem->reservation > 0 );
   assert ( lck_elem->status == UNKNOWN  ||  lck_elem->status == REQUESTED );

   TRACE("request lock %d", lck_elem->lock_id);

   /* if the lock has already been requested, just wait for it to arrive */
   if ( lck_elem->status == UNKNOWN )
   {
      lck_elem->status = REQUESTED;
      request_lock_from_manager(pm2_self(), lck_elem->lock_id, NO_TAG);
   }

   /* wait for the lock token to arrive */
   while ( lck_elem->status == REQUESTED  ||  lck_elem->status == UNKNOWN )
      marcel_cond_wait(&(lck_elem->token_arrival), &(lck_elem->mutex));

   OUT;
   return;
}


/**********************************************************************/
/* being granted the lock token after a full or partial release of the
 * lock. */
static void
recv_lock_token_func (void * unused)
{
   hierarch_lock_id_t lck_id;
   grant_state_t state;
   lock_list_elem * lck_elem;
   dsm_node_t remote_nxt;

   IN;
   pm2_unpack_byte(SEND_CHEAPER, RECV_CHEAPER, &lck_id,
                   sizeof(hierarch_lock_id_t));
   pm2_unpack_byte(SEND_CHEAPER, RECV_CHEAPER, &state, sizeof(grant_state_t));
   pm2_unpack_byte(SEND_CHEAPER, RECV_CHEAPER, &remote_nxt, sizeof(dsm_node_t));
   pm2_rawrpc_waitdata();
   assert ( state == INTRA  ||  state == INTER );

   marcel_mutex_lock(&lock_mutex);
   lck_elem = seek_lock_in_hash(lck_id);
   marcel_mutex_unlock(&lock_mutex);
   assert ( lck_elem != NULL );

   marcel_mutex_lock(&(lck_elem->mutex));
   assert ( lck_elem->reservation > 0 );
   assert ( lck_elem->status == REQUESTED );
   lck_elem->status = FREE;
   lck_elem->just_granted = SL_FALSE;

   if ( remote_nxt != NOBODY )
   {
      assert ( lck_elem->remote_next_owner == NOBODY );
      assert ( cluster_colors[pm2_self()] != cluster_colors[remote_nxt] );
      lck_elem->remote_next_owner = remote_nxt;
   }

   if ( state == INTRA )   /* the lock was partially granted */
      lck_elem->expected_remote_release++;

   TRACE("recv'd lock %d with state=%s, rmt_rel=%d, rmt_nxt=%d",
         lck_id, state == INTER ? "inter" : "intra",
         lck_elem->expected_remote_release, lck_elem->remote_next_owner);

   TRACE("signal lock %d arrival", lck_id);
   marcel_cond_broadcast(&(lck_elem->token_arrival));
   marcel_mutex_unlock(&(lck_elem->mutex));

   OUT;
   return;
}


/**********************************************************************/
/* the lock server running on the manager */
static void
lock_manager_server_func (void * unused)
{
   dsm_node_t requester;
   hierarch_lock_id_t lck_id;
   lock_list_elem * lck_elem;
   request_tag_t req_tag;
   unsigned int requester_color;

   IN;
   pm2_unpack_byte(SEND_CHEAPER, RECV_CHEAPER, &requester, sizeof(dsm_node_t));
   pm2_unpack_byte(SEND_CHEAPER, RECV_CHEAPER, &lck_id,
                   sizeof(hierarch_lock_id_t));
   pm2_unpack_byte(SEND_CHEAPER, RECV_CHEAPER, &req_tag, sizeof(request_tag_t));
   pm2_rawrpc_waitdata();

   requester_color = cluster_colors[requester];

   TRACE("recv'd request for lock %d for requester %d with tag '%s'",
         lck_id, requester, req_tag == NO_TAG ? "NO_TAG" : "TOO_LATE");
   assert ( pm2_self() == get_lock_manager(lck_id) );
   assert ( requester_color < cluster_number );

   marcel_mutex_lock(&lock_mutex);
   lck_elem = seek_lock_in_hash(lck_id);
   marcel_mutex_unlock(&lock_mutex);
   assert ( lck_elem != NULL );

   marcel_mutex_lock(&(lck_elem->mutex));

   if ( req_tag == TOO_LATE )
      /* the lock request was bounced, because it arrived just
       * after the lock token was granted to another cluster */
      send_request_to_current_owner_cluster(requester, lck_elem);
   else
   {
      if ( lck_elem->last_requester[requester_color] != NOBODY  &&
           lck_elem->last_requester[requester_color] != requester )
      {
         send_lock_request(lck_elem->last_requester[requester_color],
                           requester, lck_elem);
         lck_elem->force_next_request[requester_color] = SL_FALSE;
      }
      else
      {
         if ( lck_elem->last_requester[requester_color] == requester )
            lck_elem->force_next_request[requester_color] = SL_TRUE;
         else
            lck_elem->force_next_request[requester_color] = SL_FALSE;
         send_request_to_current_owner_cluster(requester, lck_elem);
      }
      lck_elem->last_requester[requester_color] = requester;
      TRACE("last requester of cluster %d becoming node %d", requester_color,
            requester);
   }

   marcel_mutex_unlock(&(lck_elem->mutex));

   OUT;
   return;
}


/**********************************************************************/
/* PUBLIC FUNCTIONS */
/**********************************************************************/

/**********************************************************************/
/* this function initializes the data structures for the locks; this
 * should be called by hierarch_dsm_init(); there's only one thread per
 * process executing this function. */
void
hierarch_lock_initialization (void)
{
   int i;

   IN;
   marcel_mutex_init(&lock_mutex, NULL);

   /* the hash table is initially empty */
   for (i = 0; i < LOCK_HASH_SIZE; i++)
      lock_hash[i] = NULL;

   OUT;
   return;
}


/**********************************************************************/
/* this function cleans up the data structures for the locks; there's
 * only one thread per process executing this function. */
void
hierarch_lock_exit (void)
{
   int i;
   const dsm_node_t myself = pm2_self();

   IN;
   marcel_mutex_destroy(&lock_mutex);

   /* free memory from the lock hash table */
   for (i = 0; i < LOCK_HASH_SIZE; i++)
   {
      lock_list_t lck_list = lock_hash[i];

      while ( lck_list != NULL )
      {
         lock_list_t save = lck_list;
         node_list_t lst;

         lck_list = save->next;
         marcel_cond_destroy(&(save->local_release));
         marcel_cond_destroy(&(save->token_arrival));
         marcel_mutex_destroy(&(save->mutex));
         if ( get_lock_manager(save->lock_id) == myself )
         {
            tfree(save->last_requester);
            tfree(save->force_next_request);
         }
         lst = save->notif_list;
         while ( lst != NULL )
         {
            node_list_t l = lst->next;

            lst = lst->next;
            tfree(l);
         }
         tfree(save);
      }
   }

   local_lock_number = 0;

   OUT;
   return;
}


/**********************************************************************/
/* initialize a new lock; return 0 in case of success, return -1 in
 * case of failure (wrong attribute values in future extensions).  The
 * default manager of the lock is myself. */
int
hierarch_lock_init (hierarch_lock_id_t * const lck_id)
{
   IN;
   /* in case the attributes (future extensions) specify another manager
    * than pm2_self(), do an RPC onto the other node (wanted as the
    * owner) to get a lock_id */
   {
      hierarch_lock_id_t local_id;
      lock_list_elem * lck_elem;
      int i;
      const dsm_node_t myself = pm2_self();

      /* initialize this new lock data structure */
      /* the local node can only deliver lock_id's which are congruent
       * to "pm2_self()" modulo "pm2_config_size()"... */
      marcel_mutex_lock(&lock_mutex);
      local_id = local_lock_number++;
      *lck_id = local_to_global_lock_id(local_id);
      lck_elem = create_lock_entry(*lck_id);
      /* I'm the manager of this node */
      lck_elem->status = FREE;
      lck_elem->last_cluster = cluster_colors[myself];
      lck_elem->last_requester = (dsm_node_t *) tmalloc(cluster_number *
                                                           sizeof(dsm_node_t));
      lck_elem->force_next_request = (dsm_node_t *) tmalloc(cluster_number *
                                                           sizeof(dsm_node_t));
      if ( lck_elem->last_requester==NULL || lck_elem->force_next_request==NULL )
      {
         perror(__FUNCTION__);
         abort();
      }
      for (i = 0; i < cluster_number; i++)
      {
         lck_elem->last_requester[i] = NOBODY;
         lck_elem->force_next_request[i] = SL_FALSE;
      }
      lck_elem->last_requester[lck_elem->last_cluster] = myself;
      marcel_mutex_unlock(&lock_mutex);
   }

   OUT;
   return 0;
}


/**********************************************************************/
/* acquire a DSM lock */
void
hierarch_lock (const hierarch_lock_id_t lck_id)
{
   dsm_acq_action_t acquire_func;
   lock_list_elem * lck_elem;
   
   IN;
   TRACE("want to acquire lock %d", lck_id);
   assert ( lck_id != HIERARCH_NO_LOCK );
   acquire_func = dsm_get_acquire_func(dsm_get_default_protocol());

   /* Is this lock registered in my lock hash table? */
   marcel_mutex_lock(&lock_mutex);
   lck_elem = seek_lock_in_hash(lck_id);

   /* If the lock is not registered locally or if I'm not the owner of
    * this lock, then request it from the manager. */
   if ( lck_elem == NULL )   /* the lock is not registered locally */
   {
      lck_elem = create_lock_entry(lck_id);
      assert ( lck_elem->status == UNKNOWN );
   }
   marcel_mutex_unlock(&lock_mutex);
   assert ( lck_elem != NULL );

   marcel_mutex_lock(&(lck_elem->mutex));
   /* reserve the lock locally so that the token could not be passed
    * to another process */
   lck_elem->reservation++;
   if ( lck_elem->status == UNKNOWN  ||  lck_elem->status == REQUESTED )
      request_lock(lck_elem);

   /* at this point, the lock entry exists in my local hash table, and
    * I own the lock */
   assert ( lck_elem->status == FREE  ||  lck_elem->status == BUSY );
   /* wait for the lock to be free */
   while ( lck_elem->status != FREE )
      marcel_cond_wait(&(lck_elem->local_release), &(lck_elem->mutex));

   /* The lock is free: acquire it! */
   assert ( lck_elem->reservation > 0 );
   assert ( lck_elem->status == FREE );
   lck_elem->status = BUSY;
   lck_elem->reservation--;

   marcel_mutex_unlock(&(lck_elem->mutex));

   TRACE("lock %d acquired, calling acquire consistency function", lck_id);
   /* protocol specific action at acquire time */
   if ( acquire_func != NULL ) (*acquire_func)(lck_id);

   OUT;
   return;
}


/**********************************************************************/
/* release a DSM lock. */
void
hierarch_unlock (const hierarch_lock_id_t lck_id)
{
   lock_list_elem * lck_elem;
   int local_reserv;

   IN;
   assert ( lck_id != HIERARCH_NO_LOCK );

   marcel_mutex_lock(&lock_mutex);
   lck_elem = seek_lock_in_hash(lck_id);
   marcel_mutex_unlock(&lock_mutex);
   assert ( lck_elem != NULL );

   marcel_mutex_lock(&(lck_elem->mutex));
   local_reserv = lck_elem->reservation;
   marcel_mutex_unlock(&(lck_elem->mutex));

   if ( local_reserv == 0 )
   {
      const dsm_rel_action_t release_func =
                             dsm_get_release_func(dsm_get_default_protocol());

      TRACE("calling release consistency function for lock %d", lck_id);
      if ( release_func != NULL ) (*release_func)(lck_id);
   }

   marcel_mutex_lock(&(lck_elem->mutex));

   /* this function should be called excatly once by the release function */
   assert ( lck_elem->status == BUSY );
   lck_elem->status = FREE;
   if ( local_reserv == 0 )
      grant_lock_if_possible(lck_elem);
   else
   {
      TRACE("signal lock %d locally released", lck_elem->lock_id);
      marcel_cond_signal(&(lck_elem->local_release));
   }

   marcel_mutex_unlock(&(lck_elem->mutex));

   OUT;
   return;
}


/**********************************************************************/
/* service function called by RPC to obtain a lock token */
void
hierarch_lock_request_server (void)
{
   pm2_service_thread_create(lock_request_server_func, NULL);
   return;
}


/**********************************************************************/
/* being notified of the full release of a lock which was partially
 * granted */
void
hierarch_recv_lock_token_server (void)
{
   pm2_service_thread_create(recv_lock_token_func, NULL);
   return;
}


/**********************************************************************/
/* RPC server to receive a release notification */
void
hierarch_recv_release_notification_server (void)
{
   pm2_service_thread_create(recv_release_notification_func, NULL);
   return;
}


/**********************************************************************/
/* lock manager server */
void
hierarch_lock_manager_server (void)
{
   pm2_service_thread_create(lock_manager_server_func, NULL);
   return;
}


/**********************************************************************/
/* register the number of diff/inv acks expected (positive values) or
 * received (negative values) */
void
hierarch_register_acks (const hierarch_lock_id_t lck_id, const int diff_acks,
                       const int local_inv_ack, const int remote_inv_ack)
{
   lock_list_elem * lck_elem;

   IN;

   if ( diff_acks == 0 && local_inv_ack == 0 && remote_inv_ack == 0 )
   {
      OUT;
      return;
   }

   assert ( lck_id != HIERARCH_NO_LOCK );
   marcel_mutex_lock(&lock_mutex);
   lck_elem = seek_lock_in_hash(lck_id);
   marcel_mutex_unlock(&lock_mutex);
   assert ( lck_elem != NULL );

   marcel_mutex_lock(&(lck_elem->mutex));
   lck_elem->expected_diff_acks += diff_acks;
   lck_elem->expected_local_inv_acks += local_inv_ack;
   lck_elem->expected_remote_inv_acks += remote_inv_ack;

   if ( lck_elem->expected_diff_acks == 0  &&
        lck_elem->expected_local_inv_acks == 0  &&
        ( lck_elem->expected_remote_inv_acks == 0  ||
          lck_elem->partial_release_originator == SL_TRUE ) )
   {
      send_release_notifications(lck_elem);
      lck_elem->partial_release_originator = SL_FALSE;
   }

   grant_lock_if_possible(lck_elem);

   marcel_mutex_unlock(&(lck_elem->mutex));

   OUT;
   return;
}

