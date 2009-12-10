/*
 * This file has been imported from PVFS2 implementation of BMI
 *
 * (C) 2001 Clemson University and The University of Chicago
 *
 * See COPYING in top-level directory.
 *
 * NewMadeleine
 * Copyright (C) 2009 (see AUTHORS file)
 */


/*
 * reference_list.c - functions to handle the creation and modification of 
 * reference structures for the BMI layer
 *
 */

#include <nm_public.h>

/* BMI is only available for 'huge tags' (ie. at least 64 bits) */
#ifdef NM_TAGS_AS_INDIRECT_HASH

#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <assert.h>

#include "nmad_bmi_interface.h"
#include "reference-list.h"

/***************************************************************
 * Visible functions
 */

/*
 * ref_list_new()
 *
 * creates a new reference list.
 *
 * returns pointer to an empty list or NULL on failure.
 */
ref_list_p ref_list_new(void){
    ref_list_p tmp_list = NULL;

    /* There is currently never more than one reference list in BMI.  If we
     * ever have a need for more, then this hash table should be moved from
     * a static global to actually be part of the ref_list_p.
     */

    tmp_list = tbx_slist_nil();
    return (tmp_list);
}

/*
 * ref_list_add()
 *
 * adds a reference to the list.  
 *
 * no return value
 *
 */
void 
ref_list_add(ref_list_p rlp,
	     void* addr){
    struct bnm_peer* peer = (struct bnm_peer*)addr;

    assert (peer->peername);    
    tbx_slist_add_at_head(rlp, peer);
}



/*
 * ref_list_search_addr()
 *
 * looks for a reference structure in the list that matches the given
 * BMI_addr_t.
 *
 * returns a pointer to the structure on success, a NULL on failure.
 */
void*
ref_list_search_addr(ref_list_p rlp,
		     void* addr){

    if (rlp->length){
	p_tbx_slist_element_t list_element ;
	struct bnm_peer *tmp;
	list_element = rlp->head ;
	
	while (list_element != NULL){

	    tmp = list_element->object;
	    if(tmp->p_addr == addr)
		return list_element->object;
	    list_element = list_element->next;
	}
    }

    return NULL;
}

/*
 * ref_list_search_addr()
 *
 * looks for a reference structure in the list that matches the given
 * p_gate.
 *
 * returns a pointer to the structure on success, a NULL on failure.
 */
void*
ref_list_search_gate(ref_list_p rlp,
		     void* p_gate){

    if (rlp->length){
	p_tbx_slist_element_t list_element ;
	struct bnm_peer *tmp;
	list_element = rlp->head;
	
	while (list_element != NULL){

	    tmp = list_element->object;
	    if(tmp->p_gate == p_gate)
		return list_element->object;
	    list_element = list_element->next;
	}
    }

    return NULL;
}

/*
 * ref_list_search_str()
 *
 * looks for a reference structure in the list that matches the given
 * peername.
 *
 * returns a pointer to the structure on success, a NULL on failure.
 */
void*
ref_list_search_str(ref_list_p rlp,
		    const char *idstring){
    if (rlp->length){
	p_tbx_slist_element_t list_element ;
	struct bnm_peer *tmp;
	list_element = rlp->head ;
	
	while (list_element != NULL){

	    tmp = list_element->object;
	    if(!strcmp(tmp->peername, idstring))
		return list_element->object;
	    list_element = list_element->next;
	}
    }
    return NULL;
}

/*
 * ref_list_cleanup()
 *
 * frees up the list and all data associated with it.
 *
 * no return values
 */
void ref_list_cleanup(ref_list_p rlp){
    
    while(! tbx_slist_is_nil(rlp)){
	tbx_slist_remove_from_head(rlp);
    }

    tbx_slist_free(rlp);
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
