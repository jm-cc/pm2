/*
 * This file has been imported from PVFS2 implementation of BMI
 *
 * (C) 2001 Clemson University and The University of Chicago
 *
 * See COPYING in top-level directory.
 * NewMadeleine
 * Copyright (C) 2009 (see AUTHORS file)
 */

/*
 * functions to handle storage of  
 * operation info structures for the BMI
 *
 * This is built on top of the quicklist.[ch] files
 */

/*
 * NOTE: I am not locking any data structures in here!  It is assumed
 * that the calling process is locking the appropriate operation
 * structures before calling any of the functions provided here.
 */

/* BMI is only available for 'huge tags' (ie. at least 64 bits) */
#ifdef NM_TAGS_AS_INDIRECT_HASH

#include <stdlib.h>
#include <errno.h>

#include "bmi-method-support.h"
#include "op-list.h"
//#include "gossip.h"


/***************************************************************
 * Function prototypes
 */

static void gossip_print_op(method_op_p print_op);
static int op_list_cmp_key(struct op_list_search_key *my_key,
			   method_op_p my_op);

/***************************************************************
 * Visible functions
 */

/*
 * op_list_dump()
 *
 * dumps the contents of the op list to stderr
 *
 * returns 0 on success, -errno on failure
 */
void op_list_dump(op_list_p olp)
{
    op_list_p tmp_entry = NULL;
    
    TBX_ERRORF("op_list_dump():\n");

    tbx_fast_list_for_each(tmp_entry, olp)
    {
	gossip_print_op(tbx_fast_list_entry(tmp_entry, struct method_op,
					    op_list_entry));
    }
}


/*
 * op_list_count()
 *
 * counts the number of entries in the op_list
 *
 * returns integer number of items on success, -errno on failure
 */
int op_list_count(op_list_p olp)
{
    int count = 0;
    op_list_p tmp_entry = NULL;
    tbx_fast_list_for_each(tmp_entry, olp)
    {
	count++;
    }
    return (count);
}


/*
 * op_list_new()
 *
 * creates a new operation list.
 *
 * returns pointer to an empty list or NULL on failure.
 */
op_list_p op_list_new(void)
{
    struct tbx_fast_list_head *tmp_op_list = NULL;

    tmp_op_list = (struct tbx_fast_list_head *) TBX_MALLOC(sizeof(struct tbx_fast_list_head));
    if (tmp_op_list)
    {
	TBX_INIT_FAST_LIST_HEAD(tmp_op_list);
    }

    return (tmp_op_list);
}

/*
 * op_list_add()
 *
 * adds an operation to the list.  
 *
 * returns 0 on success, -1 on failure
 */
void op_list_add(op_list_p olp,
		 method_op_p oip)
{
    /* note we are adding to tail:
     * most modules will want to preserve FIFO ordering when searching
     * through op_lists for work to do.
     */
    tbx_fast_list_add_tail(&(oip->op_list_entry), olp);
}

/*
 * op_list_cleanup()
 *
 * frees up the list and all data associated with it.
 *
 * no return values
 */
void op_list_cleanup(op_list_p olp)
{
    op_list_p iterator = NULL;
    op_list_p scratch = NULL;
    method_op_p tmp_method_op = NULL;

    tbx_fast_list_for_each_safe(iterator, scratch, olp)
    {
	tmp_method_op = tbx_fast_list_entry(iterator, struct method_op,
					    op_list_entry);
	bmi_dealloc_method_op(tmp_method_op);
    }
    TBX_FREE(olp);
    olp = NULL;
}

/* op_list_empty()
 *
 * checks to see if the operation list is empty or not.
 *
 * returns 1 if empty, 0 if items are present.
 */
int op_list_empty(op_list_p olp)
{
    return (tbx_fast_list_empty(olp));
}


/*
 * op_list_remove()
 *
 * Removes the network operation from the given list.
 * DOES NOT destroy the operation.
 *
 * returns 0 on success, -errno on failure.
 */
void op_list_remove(method_op_p oip)
{
    tbx_fast_list_del(&(oip->op_list_entry));
}


/* op_list_search()
 *
 * Searches the operation list based on parameters in the
 * op_list_search_key structure.  Returns first match.
 *
 * returns pointer to operation on success, NULL on failure.
 */
method_op_p op_list_search(op_list_p olp,
			   struct op_list_search_key *key)
{
    op_list_p tmp_entry = NULL;
    tbx_fast_list_for_each(tmp_entry, olp)
    {
	if (!(op_list_cmp_key(key, tbx_fast_list_entry(tmp_entry, struct method_op,
						       op_list_entry))))
	{
	    return (tbx_fast_list_entry(tmp_entry, struct method_op, op_list_entry));
	}
    }
    return (NULL);
}


/* op_list_shownext()
 *
 * shows the next entry in an op list; does not remove the entry
 *
 * returns pointer to method op on success, NULL on failure
 */
method_op_p op_list_shownext(op_list_p olp)
{
    if (olp->next == olp)
    {
	return (NULL);
    }
    return (tbx_fast_list_entry(olp->next, struct method_op, op_list_entry));
}

/****************************************************************
 * Internal utility functions
 */


/* 
 * op_list_cmp_key()
 *
 * compares a key structure against an operation to see if they match.  
 *
 * returns 0 if match is found, 1 otherwise
 */
static int op_list_cmp_key(struct op_list_search_key *my_key,
			   method_op_p my_op)
{

    if (my_key->method_addr_yes && (my_key->method_addr != my_op->addr))
    {
	return (1);
    }
    if (my_key->msg_tag_yes && (my_key->msg_tag != my_op->msg_tag))
    {
	return (1);
    }
    if (my_key->op_id_yes && (my_key->op_id != my_op->op_id))
    {
	return (1);
    }
    return (0);
}


static void gossip_print_op(method_op_p print_op)
{

    TBX_ERRORF("Operation:\n------------\n");
    TBX_ERRORF("  op_id: %ld\n", (long) print_op->op_id);
    TBX_ERRORF("  send_recv: %d\n", (int) print_op->send_recv);
    TBX_ERRORF("  msg_tag: %d\n", (int) print_op->msg_tag);
    TBX_ERRORF("  error_code: %d\n", (int) print_op->error_code);
    TBX_ERRORF("  amt_complete: %ld\n", (long) print_op->amt_complete);
    TBX_ERRORF("  buffer: %p\n", print_op->buffer);
    TBX_ERRORF("  actual size: %ld\n", (long) print_op->actual_size);
    TBX_ERRORF("  expected size: %ld\n", (long) print_op->expected_size);
    TBX_ERRORF("  addr: %p\n", print_op->addr);
    TBX_ERRORF("  mode: %d\n", (int) print_op->mode);

    return;
}

#endif /* NM_TAGS_AS_INDIRECT_HASH */

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 *
 * vim: ts=8 sts=4 sw=4 expandtab
 */
