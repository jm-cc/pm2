
/*
 * CVS Id: $Id: hierarch_protocol.c,v 1.10 2002/10/27 17:11:37 slacour Exp $
 */

/* Sebastien Lacour, Paris Research Group, IRISA / INRIA, May 2002 */

/* Hierarchical protocol, taking into account the network topology in
 * the consistency operations.  This protocol implements the "partial
 * release".  This protocol must be used in conjunction with the token
 * locks only. */


#include "pm2.h"
#include "hierarch_topology.h"
#include "hierarch_protocol.h"
#include "token_lock.h"


/* this file should already be included by "pm2.h"... */
#include "dsm_comm.h"


/**********************************************************************/
/* TRACING AND DEBUGGING SYSTEM                                       */
/**********************************************************************/

/* don't print the tracing / debugging messages */
#undef TRCSL
#undef DBGSL
/* remove all assertions "assert" */
#undef NDEBUG

#include "trace_debug.h"


/**********************************************************************/
/* PRIVATE DATA STRUCTURES                                            */
/**********************************************************************/

/* a boolean type */
typedef enum { SL_FALSE, SL_TRUE } bool_t;

/* linked list of pages */
typedef struct pg_elem_struct pg_elem_t;
struct pg_elem_struct
{
   dsm_page_index_t index; /* index of a page */
   pg_elem_t * next;    /* next element in linked list */
};

/* granularity of the diff computation: size of this type */
typedef unsigned int diff_grain_t;
#define GRANULARITY (sizeof(diff_grain_t))

/* when sending diffs, a node may or may not stay in the copyset */
typedef enum { STILL_IN_COPYSET, OUT_OF_COPYSET } cs_stay_t;

/* linked list of mutexes and condition variables associated to a
 * token lock */
typedef struct token_elem_struct token_elem_t;
struct token_elem_struct
{
   token_lock_id_t lck_id;

   /* diff_ack_lock and diff_ack_cond are used to wait for the diff
    * acknowledgements */
   marcel_mutex_t diff_ack_lock;
   marcel_cond_t diff_ack_cond;

   /* *_inval_ack_lock and *_inval_ack_cond are used to wait for the
    * invalidation acknowledgements */
   marcel_mutex_t local_inval_ack_lock;
   marcel_mutex_t remote_inval_ack_lock;
   marcel_cond_t local_inval_ack_cond;
   marcel_cond_t remote_inval_ack_cond;

   /* number of expected invalidations from the local cluster and from
    * remote clusters */
   int expected_local_invalidations;
   int expected_remote_invalidations;

   token_elem_t * next;
};


/**********************************************************************/
/* GLOBAL PRIVATE VARIABLES                                           */
/**********************************************************************/

/* list of the pages modified by this node */
static pg_elem_t * modif_list;
/* a mutex to access the list of modified pages */
static marcel_mutex_t modif_list_lock;

/* list of pending invalidations for this node: the invalidations are
 * stored by the invalidation_server, and they are applied during
 * synchronization operations */
static pg_elem_t * pend_inval_list;
/* a mutex to handle the list of pending invalidations */
static marcel_mutex_t pend_inval_lock;

/* List of the pages I host */
static pg_elem_t * hosted_list;
/* lock to handle the list of hosted pages */
static marcel_mutex_t hosted_lock;

/* linked list of mutexes and condition variables associated to a
 * token lock */
static token_elem_t * token_list;
/* lock to handle the previous linked list of token synchronization
 * objects */
static marcel_mutex_t token_list_lock;


/**********************************************************************/
/* PRIVATE FUNCTIONS                                                  */
/**********************************************************************/

/**********************************************************************/
/* extract any page from a list of pages; the list is assumed to be
 * already protected by a mutex; return its index; return NO_PAGE if
 * the list is empty */
static dsm_page_index_t
extract_page_from_pg_list (pg_elem_t ** const pg_list)
{
   pg_elem_t * elem = *pg_list;
   dsm_page_index_t index;

   IN;
   if ( elem )
   {
      *pg_list = elem->next;
      index = elem->index;
      tfree(elem);
      TRACE("extracted page %d from list", index);
   }
   else
      index = NO_PAGE;

   OUT;
   return index;
}


/**********************************************************************/
/* remove a specific page from a list of pages; the list is assumed to
 * be already protected by a mutex; do nothing if page is absent. */
static void
remove_page_from_pg_list (const dsm_page_index_t index,
                          pg_elem_t ** const pg_list)
{
   pg_elem_t *elem = *pg_list;
   pg_elem_t *previous = NULL;

   IN;
   while ( elem != NULL )
      if ( elem->index == index )
      {
         TRACE("removing page %d from list", index);
         if ( previous == NULL )
            *pg_list = elem->next;
         else
            previous->next = elem->next;
         tfree(elem);
         break;
      }
      else
      {
         previous = elem;
         elem = elem->next;
      }

   OUT;
   return;
}


/**********************************************************************/
/* insert a page into a list of pages; the list is assumed to be
 * already protected by a mutex; the element should not already be
 * present in the list (this is not cheched/enforced here). */
static void
insert_page_into_pg_list (const dsm_page_index_t index,
                          pg_elem_t ** const pg_list)
{
   pg_elem_t * new_elem;

   IN;
   /* allocate memory */
   new_elem = (pg_elem_t *) tmalloc(sizeof(pg_elem_t));
   if ( new_elem == NULL )
   {
      perror(__FUNCTION__);
      OUT;
      abort();
   }

   TRACE("inserting page %d into page list", index);
   /* initialize and insert */
   new_elem->index = index;
   new_elem->next = *pg_list;
   *pg_list = new_elem;

   OUT;
   return;
}


/**********************************************************************/
/* send the invalidations to the nodes in the copyset, except to the
 * diffing node; The page entry is already locked. */
static void
invalidate_copyset (dsm_page_index_t index, dsm_node_t reply_node,
                    token_lock_id_t lck_id, int * const local_inv_ack,
                    int * const remote_inv_ack, const cs_stay_t cs_stay)
{
   int i;
   int copyset_sz = dsm_get_copyset_size(index);
   dsm_node_t * const copyset = dsm_get_copyset(index);
   const int reply_node_clr = topology_get_cluster_color(reply_node);
   bool_t node_was_in_copyset = SL_FALSE;

   IN;
   assert ( index != NO_PAGE );
   assert ( reply_node < pm2_config_size() );
   assert ( reply_node != NOBODY );
   assert ( dsm_get_prob_owner(index) == pm2_self() );

   TRACE("copyset_sz=%d, pg=%d, diffing_node=%d, lck=%d",
         copyset_sz, index, reply_node, lck_id);
   /* send the invalidations to the nodes in the copyset */
   for (i = 0; i < copyset_sz; i++)
   {
      const dsm_node_t nd = copyset[i];

      /* the owner of a page is never in the copyset of this page */
      assert ( nd != dsm_get_prob_owner(index) );

      if ( nd != reply_node )
      {
         pm2_rawrpc_begin(nd, HIERARCH_INVALIDATE, NULL);
         pm2_pack_byte(SEND_CHEAPER, RECV_CHEAPER, &index,
                       sizeof(dsm_page_index_t));
         pm2_pack_byte(SEND_CHEAPER, RECV_CHEAPER, &reply_node,
                       sizeof(dsm_node_t));
         pm2_pack_byte(SEND_CHEAPER, RECV_CHEAPER, &lck_id,
                       sizeof(token_lock_id_t));
         pm2_rawrpc_end();
         if ( topology_get_cluster_color(nd) == reply_node_clr )
         {
            (*local_inv_ack)++;
            TRACE("sending inval to local node %d", nd);
         }
         else
         {
            (*remote_inv_ack)++;
            TRACE("sending inval to remote node %d", nd);
         }
      }
      else
         node_was_in_copyset = SL_TRUE;
   }

   /* empty the copyset */
   dsm_set_copyset_size(index, 0);
   if ( node_was_in_copyset == SL_TRUE && cs_stay == STILL_IN_COPYSET )
      dsm_add_to_copyset(index, reply_node);


   OUT;
   return;
}


/**********************************************************************/
/* send a chunk of page */
static void
send_chunk (int byte_offset, void * const ptr, int byte_size)
{
   IN;
   TRACE("sending mem chunk at offset=%d, size=%d, value=%d",
         byte_offset, byte_size, *((int*)ptr));
   pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, &byte_offset, sizeof(int));
   pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, &byte_size, sizeof(int));
   pm2_pack_byte(SEND_SAFER, RECV_EXPRESS, ptr, byte_size);
   OUT;
   return;
}


/**********************************************************************/
/* send the chuncks of diffs to the home node; the page entry is
 * already locked. */
static void
send_diff_to_home (const dsm_page_index_t index)
{
   /* the modified page */
   diff_grain_t * const modif = (diff_grain_t *) dsm_get_page_addr(index);
   /* the original twin */
   const diff_grain_t * const twin = (diff_grain_t *) dsm_get_twin(index);

   const int pg_sz = dsm_get_page_size(index); /* in bytes */
   const int word_num = pg_sz / GRANULARITY; /* # of words in 1 page */
   /* current index in the pages: word (grain) offset */
   int word_offset;
   /* WORD offset of the start of the difference */
   int word_start = -1;
   /* size of the contiguous memory chunk to send */
   int byte_size = 0;

   IN;
   assert ( twin != NULL );
   assert ( modif != NULL );

   /* idea: send contiguous chunks of memory that differ between both pages */
   for (word_offset = 0; word_offset < word_num; word_offset++)
   {
      /* are the words the same in the original and the modified pages? */
      if ( modif[word_offset] == twin[word_offset] )
      {
         /* both current words are the same; were the previous ones
          * the same too? */
         if ( word_start != -1 )
         {
            /* the previous words were different */
            /* send the contiguous chunk from word_start (lenght=byte_size) */
            send_chunk(word_start * GRANULARITY, modif + word_start, byte_size);
            word_start = -1;
         }
      }
      else   /* the words differ; were the previous ones different too? */
      {
         if ( word_start == -1 )
         {
            /* the previous words were the same */
            /* first word of a contiguous chunk to be sent */
            word_start = word_offset;
            byte_size = GRANULARITY;
         }
         else   /* one more word to send in the contiguous chunk */
            byte_size += GRANULARITY;
      }
   }

   /* is there anything left to send (in case the last word of both
    * pages differ)? */
   if ( word_start != -1 )
      /* send the contiguous chunk from word_start (lenght=byte_size) */
      send_chunk(word_start*GRANULARITY, modif + word_start, byte_size);

   OUT;
   return;
}


/**********************************************************************/
/* receive and apply diffs (page chuncks); the page entry is already
 * locked. */
static void
apply_diffs (const dsm_page_index_t index)
{
   const dsm_access_t pg_access = dsm_get_access(index);
   int byte_offset;
   char * const page = (char *) dsm_get_page_addr(index); /* handling bytes */

   IN;
   assert ( index != NO_PAGE );
   assert ( page != NULL );
   assert ( pg_access != NO_ACCESS );

   /* setting this page in WRITE access is not a problem: even another
    * thread in this process modifies some part of the page (on the
    * home node), the page will be invalidated anyway after this
    * function apply_diffs() */
   if ( pg_access != WRITE_ACCESS )
      dsm_set_access(index, WRITE_ACCESS);
   pm2_unpack_byte(SEND_SAFER, RECV_EXPRESS, &byte_offset, sizeof(int));
   while ( byte_offset != -1 )
   {
      int byte_size;

      pm2_unpack_byte(SEND_SAFER, RECV_EXPRESS, &byte_size, sizeof(int));
      pm2_unpack_byte(SEND_SAFER, RECV_EXPRESS, page + byte_offset, byte_size);
      TRACE("recv'd mem chunk at offset=%d, size=%d, value=%d", byte_offset,
            byte_size, *((int*)(page + byte_offset)));
      pm2_unpack_byte(SEND_SAFER, RECV_EXPRESS, &byte_offset, sizeof(int));
   }
   if ( pg_access != WRITE_ACCESS )
      dsm_set_access(index, pg_access);

   OUT;
   return;
}


/**********************************************************************/
/* return a pointer to the structure containing the synchronization
 * objects attached to the given token_lock ID; create that structure
 * and insert it in the linked list if it does not exist yet */
static token_elem_t *
get_token_elem (const token_lock_id_t lck_id)
{
   token_elem_t * token_elem;

   IN;
   marcel_mutex_lock(&token_list_lock);
   token_elem = token_list;

   while ( token_elem != NULL )
      if ( token_elem->lck_id == lck_id )
         break;
      else
         token_elem = token_elem->next;

   if ( token_elem == NULL )   /* not found --> create it */
   {
      token_elem = (token_elem_t *) tmalloc(sizeof(token_elem_t));
      if ( token_elem == NULL )
      {
         perror(__FUNCTION__);
         OUT;
         abort();
      }

      TRACE("creating/initializing a new token_elem for lck=%d", lck_id);
      token_elem->lck_id = lck_id;
      token_elem->expected_local_invalidations = 0;
      token_elem->expected_remote_invalidations = 0;
      marcel_mutex_init(&(token_elem->diff_ack_lock), NULL);
      marcel_mutex_init(&(token_elem->local_inval_ack_lock), NULL);
      marcel_mutex_init(&(token_elem->remote_inval_ack_lock), NULL);
      marcel_cond_init(&(token_elem->diff_ack_cond), NULL);
      marcel_cond_init(&(token_elem->local_inval_ack_cond), NULL);
      marcel_cond_init(&(token_elem->remote_inval_ack_cond), NULL);

      /* insert it in the linked list */
      token_elem->next = token_list;
      token_list = token_elem;
   }

   marcel_mutex_unlock(&token_list_lock);

   assert ( token_elem != NULL );
   OUT;
   return token_elem;
}


/**********************************************************************/
/* tell the diffing node how many invalidations I sent, since the ACKs
 * are going directly to the diffing node */
static void
send_ack_number_to_diffing_node(const dsm_node_t reply_node,
                                token_lock_id_t lck_id, int local_ack_number,
                                int remote_ack_number)
{
   IN;
   assert ( reply_node != NOBODY );
   assert ( reply_node < pm2_config_size() );
   assert ( pm2_self() != reply_node );
   TRACE("sending local_inv_ack=%d, remote_inv_ack=%d to node %d (lck=%d)",
         local_ack_number, remote_ack_number, reply_node, lck_id);

   pm2_rawrpc_begin(reply_node, HIERARCH_DIFF_ACK, NULL);
   pm2_pack_byte(SEND_CHEAPER, RECV_CHEAPER, &lck_id, sizeof(token_lock_id_t));
   pm2_pack_byte(SEND_CHEAPER, RECV_CHEAPER, &local_ack_number, sizeof(int));
   pm2_pack_byte(SEND_CHEAPER, RECV_CHEAPER, &remote_ack_number, sizeof(int));
   pm2_rawrpc_end();

   OUT;
   return;
}


/**********************************************************************/
/* receive the number of expected invalidation ACKs after the home has
 * applied a diff */
static void
recv_diff_ack (void * const unused)
{
   token_elem_t * token_elem;
   token_lock_id_t lck_id;
   int local_ack_num, remote_ack_num;

   IN;
   pm2_unpack_byte(SEND_CHEAPER, RECV_CHEAPER, &lck_id,
                   sizeof(token_lock_id_t));
   pm2_unpack_byte(SEND_CHEAPER, RECV_CHEAPER, &local_ack_num, sizeof(int));
   pm2_unpack_byte(SEND_CHEAPER, RECV_CHEAPER, &remote_ack_num, sizeof(int));
   pm2_rawrpc_waitdata();

   TRACE("recv'd diff ack with local_inv_ack=%d, remote_inv_ack=%d for lck %d",
         local_ack_num, remote_ack_num, lck_id);

   token_elem = get_token_elem(lck_id);
   assert ( token_elem != NULL );

   marcel_mutex_lock(&(token_elem->diff_ack_lock));
   /* the locks local_inval_ack_lock and remote_inval_ack_lock are
    * assumed to be already acquired */
   token_elem->expected_local_invalidations += local_ack_num;
   token_elem->expected_remote_invalidations += remote_ack_num;
   marcel_cond_signal(&(token_elem->diff_ack_cond));
   marcel_mutex_unlock(&(token_elem->diff_ack_lock));

   OUT;
   return;
}


/**********************************************************************/
/* RPC service threaded function to receive a diff */
static void
recv_diff_func (void * const unused)
{
   dsm_page_index_t index;
   int local_ack_num = 0;
   int remote_ack_num = 0;
   dsm_node_t reply_node;
   cs_stay_t cs_stay;
   token_lock_id_t lck_id;

   IN;
   pm2_unpack_byte(SEND_CHEAPER, RECV_CHEAPER, &lck_id,
                   sizeof(token_lock_id_t));
   pm2_unpack_byte(SEND_CHEAPER, RECV_CHEAPER, &cs_stay, sizeof(cs_stay));
   pm2_unpack_byte(SEND_CHEAPER, RECV_EXPRESS, &index, sizeof(dsm_page_index_t));
   pm2_unpack_byte(SEND_CHEAPER, RECV_CHEAPER, &reply_node, sizeof(dsm_node_t));

   assert ( index != NO_PAGE );
   assert ( dsm_get_prob_owner(index) == pm2_self() );

   dsm_lock_page(index);

   /* apply the diffs */
   apply_diffs(index);

   /* terminate */
   pm2_rawrpc_waitdata();
   assert ( cs_stay == OUT_OF_COPYSET || cs_stay == STILL_IN_COPYSET );
   assert ( reply_node != pm2_self() );
   assert ( reply_node != NOBODY );
   assert ( reply_node < pm2_config_size() );
   TRACE("recv'd diff from node %d on page index %d (lock=%d, cs_stay='%s')",
         reply_node, index, lck_id,
         cs_stay == OUT_OF_COPYSET ? "OUT_OF_COPYSET" : "STILL_IN_COPYSET");

   /* send invalidation msgs to the nodes in the copyset */
   /* This code (invalidation of the copyset) could be moved before
    * the application of the diffs (to save a little time) */
   invalidate_copyset(index, reply_node, lck_id, &local_ack_num,
                      &remote_ack_num, cs_stay);
   TRACE("invalidated local copyset with %d nodes", local_ack_num);
   TRACE("invalidated remote copyset with %d nodes", remote_ack_num);

   dsm_unlock_page(index);

   /* tell the diffing node how many invalidations I sent, since the
    * ACKs are going directly to the diffing node */
   send_ack_number_to_diffing_node(reply_node, lck_id, local_ack_num,
                                   remote_ack_num);

   OUT;
   return;
}


/**********************************************************************/
/* send the diff of the given page to the home node and free the
 * memory for the twin; the page entry is already locked. */
static void
diffing (dsm_page_index_t index, token_lock_id_t lck_id, cs_stay_t cs_stay)
{
   const dsm_node_t home = dsm_get_prob_owner(index);
   dsm_node_t self = pm2_self();
   int byte_offset = -1;

   IN;
   assert ( index != NO_PAGE );
   assert ( home != self );
   TRACE("sending diffs from %d to %d on page %d (lock=%d)",
         self, home, index, lck_id);
   pm2_rawrpc_begin(home, HIERARCH_DIFF, NULL);
   pm2_pack_byte(SEND_CHEAPER, RECV_CHEAPER, &lck_id, sizeof(token_lock_id_t));
   pm2_pack_byte(SEND_CHEAPER, RECV_CHEAPER, &cs_stay, sizeof(cs_stay_t));
   pm2_pack_byte(SEND_CHEAPER, RECV_EXPRESS, &index, sizeof(dsm_page_index_t));
   pm2_pack_byte(SEND_CHEAPER, RECV_CHEAPER, &self, sizeof(dsm_node_t));

   /* send the diffs */
   send_diff_to_home(index);

   /* terminate */
   pm2_pack_byte(SEND_CHEAPER, RECV_EXPRESS, &byte_offset, sizeof(int));
   pm2_rawrpc_end();
   tfree(dsm_get_twin(index));
   dsm_set_twin_ptr(index, NULL);

   OUT;
   return;
}


/**********************************************************************/
/* make a twin (the memory for the twin has already been allocated);
 * the page entry has already been locked. */
static void
make_twin (const dsm_page_index_t index)
{
   IN;
   assert ( index != NO_PAGE );
   assert ( dsm_get_page_addr(index) != NULL );
   assert ( dsm_get_twin(index) != NULL );

   TRACE("twinning page index %d", index);
   memcpy(dsm_get_twin(index), dsm_get_page_addr(index),
          dsm_get_page_size(index));
   OUT;
   return;
}


/**********************************************************************/
/* insert the node into the copyset if it is not already present; the
 * page entry is already locked */
static void
insert_node_into_copyset (const dsm_page_index_t index, const dsm_node_t node)
{
   int i;
   const int copyset_sz = dsm_get_copyset_size(index);
   const dsm_node_t * const copyset = dsm_get_copyset(index);

   IN;
   assert ( index != NO_PAGE );
   assert ( node != NOBODY );
   assert ( node < pm2_config_size() );
   /* the home of a page is never in the copyset of the page */
   assert ( node != pm2_self() );
   assert ( dsm_get_prob_owner(index) == pm2_self() );

   /* if the node is already in the copyset, do nothing */
   for (i = 0; i < copyset_sz; i++)
      if ( copyset[i] == node )
      {
         OUT;
         return;
      }

   /* the node was not found in the copyset: insert it */
   TRACE("add node %d in copyset of pg %d", node, index);
   dsm_add_to_copyset(index, node);

   OUT;
   return;
}


/**********************************************************************/
/* the page entry is supposed to be locked, and there should be no
 * twin already allocated. */
static void
allocate_twin (const dsm_page_index_t index)
{
   void * ptr;

   IN;
   assert ( dsm_get_twin(index) == NULL );

   ptr = (void *) tmalloc(dsm_get_page_size(index));
   if ( ptr == NULL )
   {
      perror(__FUNCTION__);
      OUT;
      abort();
   }
   dsm_set_twin_ptr(index, ptr);
   TRACE("allocated memory for twin of page %d", index);

   OUT;
   return;
}
 
 
/**********************************************************************/
/* invalidate the pages found in the pending invalidation list; send
 * the diffs if the invalidated page was in write access mode; both
 * lists of pending invalidations and locally modified pages are
 * assumed to be locked. */
static int
flush_pending_invalidations (const token_lock_id_t lck_id)
{
   int diff_ack_number = 0;
   dsm_page_index_t index;

   IN;
   while ( (index = extract_page_from_pg_list(&pend_inval_list)) != NO_PAGE )
   {
      dsm_access_t pg_access;

      /* the invalidate server should not insert pages it owns in the
       * list of pending invalidations */
      assert ( dsm_get_prob_owner(index) != pm2_self() );

      TRACE("invalidating page %d (lock=%d)", index, lck_id);
      dsm_lock_page(index);
      pg_access = dsm_get_access(index);
      /* a given page can occur at most once in the "pend_inval_list" */
      assert ( pg_access != NO_ACCESS );
      if ( pg_access == WRITE_ACCESS )
      {
         remove_page_from_pg_list(index, &modif_list);
         diff_ack_number++;
         diffing(index, lck_id, OUT_OF_COPYSET);
      }
      dsm_set_access(index, NO_ACCESS);
      dsm_unlock_page(index);
   }

   OUT;
   return diff_ack_number;
}


/**********************************************************************/
/* send the diffs of the pages which have been modified locally; the
 * list of locally modified pages is assumed to be locked. */
static int
flush_modified_pages (const token_lock_id_t lck_id)
{
   int diff_number = 0;
   dsm_page_index_t index;

   IN;
   while ( (index = extract_page_from_pg_list(&modif_list)) != NO_PAGE )
   {
      diff_number++;
      assert ( dsm_get_prob_owner(index) != pm2_self() );
      dsm_lock_page(index);
      assert ( dsm_get_access(index) == WRITE_ACCESS );
      dsm_set_access(index, READ_ACCESS);
      diffing(index, lck_id, STILL_IN_COPYSET);
      dsm_unlock_page(index);
   }

   OUT;
   return diff_number;
}


/**********************************************************************/
/* send an ACK to acknowledge the receipt of the invalidation msg;
 * send the ACK to the diffing node directly */
static void
send_inv_ack (const dsm_node_t node, token_lock_id_t lck_id)
{
   dsm_node_t self = pm2_self();

   IN;
   assert ( node != NOBODY );
   assert ( node < pm2_config_size() );
   assert ( node != self );

   TRACE("sending an inv ack for lock %d to node %d", lck_id, node);
   pm2_rawrpc_begin(node, HIERARCH_INV_ACK, NULL);
   pm2_pack_byte(SEND_CHEAPER, RECV_CHEAPER, &lck_id, sizeof(token_lock_id_t));
   pm2_pack_byte(SEND_CHEAPER, RECV_CHEAPER, &self, sizeof(dsm_node_t));
   pm2_rawrpc_end();

   OUT;
   return;
}


/**********************************************************************/
/* Invalidate Server: this is the RPC service threaded function;
 * receive the index of the page to invalidate. */
static void
invalidate_server_func (void * const unused)
{
   dsm_page_index_t index;
   token_lock_id_t lck_id;
   dsm_node_t reply_node;

   IN;
   pm2_unpack_byte(SEND_CHEAPER, RECV_CHEAPER, &index, sizeof(dsm_page_index_t));
   pm2_unpack_byte(SEND_CHEAPER, RECV_CHEAPER, &reply_node, sizeof(dsm_node_t));
   pm2_unpack_byte(SEND_CHEAPER, RECV_CHEAPER, &lck_id,
                   sizeof(token_lock_id_t));
   pm2_rawrpc_waitdata();

   assert ( index != NO_PAGE );
   assert ( reply_node != NOBODY );
   assert ( reply_node < pm2_config_size() );
   assert ( reply_node != pm2_self() );
   /* I can't receive an invalidation msg for a page I own */
   assert ( dsm_get_prob_owner(index) != pm2_self() );
   TRACE("recv'd invalidation for page %d (lock=%d)", index, lck_id);

   /* The pages are invalidated during synchronization operations
    * (acquire & release) */
   marcel_mutex_lock(&pend_inval_lock);
   assert ( dsm_get_access(index) != NO_ACCESS );
   insert_page_into_pg_list(index, &pend_inval_list);
   marcel_mutex_unlock(&pend_inval_lock);

   /* send an ACK for the invalidation msg */
   send_inv_ack(reply_node, lck_id);
   OUT;
   return;
}


/**********************************************************************/
/* receive an ack for an invalidation message */
static void
recv_inv_ack (void * const unused)
{
   token_elem_t * token_elem;
   dsm_node_t from_node;
   token_lock_id_t lck_id;

   IN;
   pm2_unpack_byte(SEND_CHEAPER, RECV_CHEAPER, &lck_id,
                   sizeof(token_lock_id_t));
   pm2_unpack_byte(SEND_CHEAPER, RECV_CHEAPER, &from_node, sizeof(dsm_node_t));
   pm2_rawrpc_waitdata();

   assert ( from_node != pm2_self() );
   assert ( from_node != NOBODY );
   assert ( from_node < pm2_config_size() );

   token_elem = get_token_elem(lck_id);
   assert ( token_elem != NULL );

   if ( topology_get_cluster_color(pm2_self()) ==
        topology_get_cluster_color(from_node) )
   {
      TRACE("recv'd inv ack from node=%d in local cluster", from_node);
      /* local inv ack recv'd */
      marcel_mutex_lock(&(token_elem->local_inval_ack_lock));
      token_elem->expected_local_invalidations--;
      /* the acquire and release consistency functions canNOT be
       * waiting for this condition at the same time: just signal */
      marcel_cond_signal(&(token_elem->local_inval_ack_cond));
      marcel_mutex_unlock(&(token_elem->local_inval_ack_lock));
   }
   else
   {
      TRACE("recv'd inv ack from node=%d in remote cluster", from_node);
      /* remote inv ack recv'd */
      marcel_mutex_lock(&(token_elem->remote_inval_ack_lock));
      token_elem->expected_remote_invalidations--;
      /* both the acquired and release consistency functions may be
       * waiting for this condition due to partial unlock: broadcast */
      marcel_cond_broadcast(&(token_elem->remote_inval_ack_cond));
      marcel_mutex_unlock(&(token_elem->remote_inval_ack_lock));
   }

   OUT;
   return;
}


/**********************************************************************/
/* send invalidation messages to all the nodes included in the
 * copysets of the pages I host and which are accessible in WRITE mode */
static void
invalidate_writable_hosted_pages (token_elem_t * const token_elem)
{
   pg_elem_t * list;
   int local_inv_ack = 0;
   int remote_inv_ack = 0;

   IN;

   marcel_mutex_lock(&hosted_lock);
   list = hosted_list;
   while ( list != NULL )
   {
      const dsm_page_index_t index = list->index;

      /* I'm supposed to be the host of this page... */
      assert ( dsm_get_prob_owner(index) == pm2_self() );
      list = list->next;

      dsm_lock_page(index);
      if ( dsm_get_access(index) == WRITE_ACCESS )
      {
         TRACE("home invalidating copyset of page %d, lck=%d",
               index, token_elem->lck_id);
         dsm_set_access(index, READ_ACCESS);
         invalidate_copyset(index, pm2_self(), token_elem->lck_id,
                            &local_inv_ack, &remote_inv_ack, OUT_OF_COPYSET);
      }
      assert ( dsm_get_access(index) == READ_ACCESS );
      dsm_unlock_page(index);
   }
   marcel_mutex_unlock(&hosted_lock);

   token_elem->expected_local_invalidations += local_inv_ack;
   token_elem->expected_remote_invalidations += remote_inv_ack;

   OUT;
   return;
}


/**********************************************************************/
/* send a page to a requesting node */
static void
provide_page (const dsm_page_index_t index, const dsm_node_t req_node,
              const dsm_access_t access, const int tag)
{
   IN;
   assert ( index != NO_PAGE );
   assert ( req_node != NOBODY );
   assert ( req_node < pm2_config_size() );
   assert ( req_node != pm2_self() );

   dsm_lock_page(index);

   /* the page must be locked before sending it because the
    * receive_diff_server may be applying a diff on this page; here is
    * the bad scenario which must not happen: thread 0 sending page p
    * to the requesting node, thread 1 applying the diffs on page p
    * and sending invalidations (except to the requesting node of
    * thread 0), thread 0 adding the requesting node into the copyset
    * ==> page p on the requesting node may not be invalidated early
    * enough. */

   /* I should be the home of the requested page */
   assert ( dsm_get_prob_owner(index) == pm2_self() );

   TRACE("sending page %d to node %d", index, req_node);
   dsm_send_page(req_node, index, access, tag);

   /* insert the requesting node into the copyset if it is not
    * already present */
   insert_node_into_copyset(index, req_node);
   dsm_unlock_page(index);

   OUT;
   return;
}


/**********************************************************************/
/* PUBLIC FUNCTIONS                                                   */
/**********************************************************************/

/**********************************************************************/
/* Read Fault Handler */
void
hierarch_proto_read_fault_handler (const dsm_page_index_t index)
{
   IN;
   assert ( index != NO_PAGE );
   /* There can't be any READ page fault on the home node of a page */
   assert ( dsm_get_prob_owner(index) != pm2_self() );

   TRACE("caught read fault for page %d", index);
   dsm_lock_page(index);
   /* In case another thread has already requested the page and the
    * page had enough time to arrive */
   if ( dsm_get_access(index) >= READ_ACCESS )
   {
      dsm_unlock_page(index);
      OUT;
      return;
   }

   /* In case another thread has already sent a page request, just
    * wait for the page to arrive */
   if ( dsm_get_pending_access(index) == NO_ACCESS )
   {
      TRACE("sending read page req to %d for page %d",
            dsm_get_prob_owner(index), index);
      dsm_send_page_req(dsm_get_prob_owner(index), index,
                        pm2_self() /* reply to me */, READ_ACCESS, 0 /* tag */);
      dsm_set_pending_access(index, READ_ACCESS);
   }

   /* Wait for the page to arrive */
   dsm_wait_for_page(index);
   dsm_unlock_page(index);
   TRACE("has read access on page %d", index);
   OUT;
   return;
}


/**********************************************************************/
/* Write Fault Handler */
void
hierarch_proto_write_fault_handler (const dsm_page_index_t index)
{
   const dsm_node_t myself = pm2_self();
   const dsm_node_t home = dsm_get_prob_owner(index);
   dsm_access_t pg_access;

   IN;
   assert ( index != NO_PAGE );
   TRACE("caught write fault for page %d", index);

   if ( myself == home )
   {
      TRACE("home setting page %d in WRITE access", index);
      dsm_lock_page(index);
      assert ( dsm_get_access(index) >= READ_ACCESS );
      if ( dsm_get_access(index) == READ_ACCESS )
         dsm_set_access(index, WRITE_ACCESS);
      dsm_unlock_page(index);
      OUT;
      return;
   }

   marcel_mutex_lock(&modif_list_lock);
   dsm_lock_page(index);

   pg_access = dsm_get_access(index);

   /* In case another thread has already requested the page and the
    * page had enough time to arrive */
   if ( pg_access == WRITE_ACCESS )
   {
      dsm_unlock_page(index);
      marcel_mutex_unlock(&modif_list_lock);
      OUT;
      return;
   }

   /* if i already has read access on the page, just make a twin and
    * set the page in WRITE access mode */
   if ( pg_access == READ_ACCESS )
   {
      assert ( dsm_get_pending_access(index) == NO_ACCESS );
      allocate_twin(index);
      make_twin(index);
      dsm_set_access(index, WRITE_ACCESS);
   }
   else   /* I have no access to the page */
   {
      const dsm_access_t pending_access = dsm_get_pending_access(index);

      assert ( pg_access == NO_ACCESS );
      if ( pending_access == NO_ACCESS ) 
         /* the page has not been requested yet */
         dsm_send_page_req(home, index, pm2_self() /* reply to me */,
                           WRITE_ACCESS, 0 /* tag */);
      if ( pending_access != WRITE_ACCESS )
      {
         allocate_twin(index);
         insert_page_into_pg_list(index, &modif_list);
      }
      dsm_set_pending_access(index, WRITE_ACCESS);
      dsm_wait_for_page(index);
   }

   dsm_unlock_page(index);
   marcel_mutex_unlock(&modif_list_lock);
   TRACE("has write access on page %d", index);
   OUT;
   return;
}


/**********************************************************************/
/* Read Server */
void
hierarch_proto_read_server (const dsm_page_index_t index,
                            const dsm_node_t req_node, const int tag)
{
   IN;
   provide_page(index, req_node, READ_ACCESS, tag);
   OUT;
   return;
}


/**********************************************************************/
/* Write Server */
void
hierarch_proto_write_server (const dsm_page_index_t index,
                             const dsm_node_t req_node, const int tag)
{
   IN;
   provide_page(index, req_node, WRITE_ACCESS, tag);
   OUT;
   return;
}


/**********************************************************************/
/* Invalidate Server: this is the RPC service function */
void
hierarch_invalidate_server (void)
{
   IN;
   pm2_service_thread_create(invalidate_server_func, NULL);
   OUT;
   return;
}


/**********************************************************************/
/* Receive Page Server */
void
hierarch_proto_receive_page_server (void * const addr,
                                    const dsm_access_t access,
                                    const dsm_node_t reply_node, const int tag)
{
   /* no need to acquire the lock on the page entry: this value is constant */
   const dsm_page_index_t index = dsm_page_index(addr);
   dsm_access_t pending_access;

   IN;
   assert ( index != NO_PAGE );
   assert ( addr != NULL );
   assert ( reply_node != pm2_self() );
   assert ( reply_node != NOBODY );
   assert ( reply_node < pm2_config_size() );

   dsm_lock_page(index);
   pending_access = dsm_get_pending_access(index);

   TRACE("recv'd page %d from %d", index, reply_node);
   assert ( dsm_get_access(index) == NO_ACCESS );
   assert ( pending_access > NO_ACCESS );
   /* the home node is the only one that should reply! */
   assert ( reply_node == dsm_get_prob_owner(index) );

   if ( pending_access == READ_ACCESS )
      dsm_set_access(index, READ_ACCESS);
   else   /* pending_access == WRITE_ACCESS */
   {
      dsm_set_access(index, READ_ACCESS);
      make_twin(index);
      dsm_set_access(index, WRITE_ACCESS);
   }
   dsm_set_pending_access(index, NO_ACCESS);
   dsm_signal_page_ready(index);  /* this is really a Broadcast! */
   dsm_unlock_page(index);

   OUT;
   return;
}


/**********************************************************************/
/* Lock Acquire; if lck_id == TOKEN_LOCK_NONE, then it is the 2nd part
 * of a barrier (barrier == release + acquire). */
void
hierarch_proto_acquire_func (const token_lock_id_t lck_id)
{
   token_elem_t * token_elem;
   /* number of expected ack's for the diffs msgs */
   int diff_number;

   IN;

   token_elem = get_token_elem(lck_id);
   assert ( token_elem != NULL );

   /* to avoid deadlocks, "remote_inval_ack_lock" is always acquired
    * before "local_inval_ack_lock"; "local_inval_ack_lock" is always
    * acquired before "diff_ack_lock"; "diff_ack_lock" is always
    * acquired before "modif_list_lock"; "modif_list_lock" is always
    * acquired before "pend_inval_lock". */
   marcel_mutex_lock(&(token_elem->remote_inval_ack_lock));
   marcel_mutex_lock(&(token_elem->local_inval_ack_lock));
   marcel_mutex_lock(&(token_elem->diff_ack_lock));
   marcel_mutex_lock(&modif_list_lock);
   marcel_mutex_lock(&pend_inval_lock);
   diff_number = flush_pending_invalidations(lck_id);
   marcel_mutex_unlock(&pend_inval_lock);
   marcel_mutex_unlock(&modif_list_lock);

   /* the token lock can't be acquired when local invalidations are
    * expected; but it can be acquired while remote invalidations are
    * expected due to partial unlock. */
   assert ( token_elem->expected_local_invalidations == 0 );

   TRACE("waiting for %d diff acks, lck=%d", diff_number, lck_id);

   /* the acquire function does not need to wait for the diff ACKs to
    * arrive.  Warning: the recv_diff_ack function assumes that the
    * locks local_inval_ack_lock & remote_inval_ack_lock are acquired
    * when recving a diff ack... so the following wait code can't be
    * simply removed */
   while ( diff_number > 0 )
   {
      marcel_cond_wait(&(token_elem->diff_ack_cond),
                       &(token_elem->diff_ack_lock));
      diff_number--;
   }
   assert ( diff_number == 0 );
   marcel_mutex_unlock(&(token_elem->diff_ack_lock));

   TRACE("have %d local inv acks, %d remote inv acks, lck=%d",
         token_elem->expected_local_invalidations,
         token_elem->expected_remote_invalidations, lck_id);

   marcel_mutex_unlock(&(token_elem->local_inval_ack_lock));
   marcel_mutex_unlock(&(token_elem->remote_inval_ack_lock));

   OUT;
   return;
}


/**********************************************************************/
/* Lock Release; if lck_id == TOKEN_LOCK_NONE, then it is the 2nd part
 * of a barrier (barrier == release + acquire). */
void
hierarch_proto_release_func (const token_lock_id_t lck_id)
{
   token_elem_t * token_elem;
   /* number of diffs sent to the home nodes */
   int diff_number;

   IN;

   token_elem = get_token_elem(lck_id);
   assert ( token_elem != NULL );

   /* to avoid deadlocks, "remote_inval_ack_lock" is always acquired
    * before "local_inval_ack_lock"; "local_inval_ack_lock" is always
    * acquired before "diff_ack_lock"; "diff_ack_lock" is always
    * acquired before "modif_list_lock"; "modif_list_lock" is always
    * acquired before "pend_inval_lock". */
   marcel_mutex_lock(&(token_elem->remote_inval_ack_lock));
   marcel_mutex_lock(&(token_elem->local_inval_ack_lock));
   marcel_mutex_lock(&(token_elem->diff_ack_lock));

   /* the acquire consistency operation does not wait the for local /
    * remote invalidation ACKs to arrive, so
    * expected_local_invalidations or expected_remote_invalidations
    * may not be null */

   /* send invalidations for the copysets of ALL the pages I host */
   invalidate_writable_hosted_pages(token_elem);

   /* The flushing of the pending invalidations could be postponed
    * until the next acquire operation, but the latter is Synchronous
    * while the release operation could be Asynchronous, so it might
    * be better to flush the pending invalidations at release time;
    * that also yields better reactivity at acquire time */
   marcel_mutex_lock(&modif_list_lock);
   marcel_mutex_lock(&pend_inval_lock);
   diff_number = flush_pending_invalidations(lck_id);
   marcel_mutex_unlock(&pend_inval_lock);

   /* send diffs of modified pages */
   diff_number += flush_modified_pages(lck_id);
   marcel_mutex_unlock(&modif_list_lock);

   TRACE("waiting for %d diff acks, lck=%d", diff_number, lck_id);

   while ( diff_number > 0 )
   {
      marcel_cond_wait(&(token_elem->diff_ack_cond),
                       &(token_elem->diff_ack_lock));
      diff_number--;
   }
   assert ( diff_number == 0 );
   marcel_mutex_unlock(&(token_elem->diff_ack_lock));

   TRACE("wait for %d local inv acks, %d remote inv acks, lck=%d",
         token_elem->expected_local_invalidations,
         token_elem->expected_remote_invalidations, lck_id);

   while ( token_elem->expected_local_invalidations > 0 )
      marcel_cond_wait(&(token_elem->local_inval_ack_cond),
                       &(token_elem->local_inval_ack_lock));
   assert ( token_elem->expected_local_invalidations == 0 );
   marcel_mutex_unlock(&(token_elem->local_inval_ack_lock));

   /* partially release the lock if this is not a barrier */
   if ( lck_id != TOKEN_LOCK_NONE )
      token_partial_unlock(lck_id);

   while ( token_elem->expected_remote_invalidations > 0 )
      marcel_cond_wait(&(token_elem->remote_inval_ack_cond),
                       &(token_elem->remote_inval_ack_lock));
   assert ( token_elem->expected_remote_invalidations == 0 );
   marcel_mutex_unlock(&(token_elem->remote_inval_ack_lock));

   OUT;
   return;
}


/**********************************************************************/
/* Protocol Initialization; only one thread per process should call
 * this function (which is called by dsm_pm2_init()). */
void
hierarch_proto_initialization (const int prot_id)
{
   IN;
   /* the list of modified pages is initially empty */
   modif_list = NULL;
   marcel_mutex_init(&modif_list_lock, NULL);

   /* the list of pending invalidations is initially empty */
   pend_inval_list = NULL;
   marcel_mutex_init(&pend_inval_lock, NULL);

   /* the list of hosted pages is initially empty */
   hosted_list = NULL;
   marcel_mutex_init(&hosted_lock, NULL);

   /* the list of synchronization objects attached to each token lock
    * is initially empty */
   token_list = NULL;
   marcel_mutex_init(&token_list_lock, NULL);

   OUT;
   return;
}


/**********************************************************************/
/* Addition of a Page into the PageTable */
void
hierarch_proto_page_add_func (const dsm_page_index_t index)
{
   IN;
   assert ( index != NO_PAGE );

   /* the twin is initialized to NULL beforehand */
   /* the copyset is allocated, and its size is set to 0 beforehand */
   /* the write_page and read_page condition variables are initialized
    * beforehand */

   /* ATTENTION:
    * the prob_owner of this page should already be set properly.  If
    * this node is the creator of the page, then it is the owner;  if
    * this page was received from another node, then the owner is set
    * to this other node */

   /* the home node has always at least read access to its own pages */
   /* "hosted_lock" is always acquired before the page mutex to avoid
    * deadlocks */
   marcel_mutex_lock(&hosted_lock);
   dsm_lock_page(index);

   TRACE("home node of page %d is %d", index, dsm_get_prob_owner(index));
   if ( dsm_get_prob_owner(index) == pm2_self() )   /* I'm the home node */
   {
      dsm_set_access(index, READ_ACCESS);
      /* insert this page into the list of pages I host */
      insert_page_into_pg_list(index, &hosted_list);
   }
   else
      dsm_set_access(index, NO_ACCESS);

   dsm_unlock_page(index);
   marcel_mutex_unlock(&hosted_lock);

   OUT;
   return;
}


/**********************************************************************/
/* protocol termination function; a parameter should be added to the
 * function dsm_create_protocol() to register this function, and the
 * finalization functions of each protocol should be called from
 * dsm_pm2_exit(). Only one thread per process should call this
 * function. */
void
hierarch_proto_finalization (void)
{
   IN;
   /* empty the list of modified pages to free memory */
   /* it is useless to acquire the lock since there's only one thread
    * per process executing this function. */
   while ( extract_page_from_pg_list(&modif_list) != NO_PAGE )
      /* empty the list */ ;
   marcel_mutex_destroy(&modif_list_lock);

   /* empty the list of pending invalidations to free memory */
   /* it is useless to acquire the lock since there's only one thread
    * per process executing this function. */
   while ( extract_page_from_pg_list(&pend_inval_list) != NO_PAGE )
      /* empty the list */ ;
   marcel_mutex_destroy(&pend_inval_lock);

   /* empty the list of hosted pages.  no need to acquire the lock
    * because there's only one thread per process executing this
    * function. */
   while ( extract_page_from_pg_list(&hosted_list) != NO_PAGE )
      /* empty the list */ ;
   marcel_mutex_destroy(&hosted_lock);

   /* empty the list of synchronization objects attached to each token
    * lock */
   while ( token_list != NULL )
   {
      token_elem_t * const next = token_list->next;

      marcel_cond_destroy(&(token_list->diff_ack_cond));
      marcel_cond_destroy(&(token_list->local_inval_ack_cond));
      marcel_cond_destroy(&(token_list->remote_inval_ack_cond));
      marcel_mutex_destroy(&(token_list->diff_ack_lock));
      marcel_mutex_destroy(&(token_list->local_inval_ack_lock));
      marcel_mutex_destroy(&(token_list->remote_inval_ack_lock));
      tfree(token_list);

      token_list = next;
   }
   marcel_mutex_destroy(&token_list_lock);

   OUT;
   return;
}


/**********************************************************************/
/* RPC service function to receive the number of expected invalidation
 * ACKs after applying a diff */
void
hierarch_diff_ack_server (void)
{
   IN;
   pm2_service_thread_create(recv_diff_ack, NULL);
   OUT;
   return;
}


/**********************************************************************/
/* RPC service function to apply a diff */
void
hierarch_recv_diff_server (void)
{
   IN;
   pm2_service_thread_create(recv_diff_func, NULL);
   OUT;
   return;
}


/**********************************************************************/
/* RPC service function to receive an invalidation ack */
void
hierarch_inv_ack_server (void)
{
   IN;
   pm2_service_thread_create(recv_inv_ack, NULL);
   OUT;
   return;
}

