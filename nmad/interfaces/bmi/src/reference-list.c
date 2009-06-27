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

#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <assert.h>

#include "reference-list.h"
#include "tbx.h"
#include "Padico/Puk.h"
#include "Padico/Puk-hashtable.h"
//#include "id-generator.h"
//#include "quickhash.h"

static puk_hashtable_t str_table = NULL;
#define STR_TABLE_SIZE 137

/***************************************************************
 * Visible functions
 */

static int ref_list_compare_key_entry(void* key, struct puk_hash_entry_s* link);

/*
 * ref_list_new()
 *
 * creates a new reference list.
 *
 * returns pointer to an empty list or NULL on failure.
 */
ref_list_p ref_list_new(void)
{

    ref_list_p tmp_list = NULL;

    /* There is currently never more than one reference list in BMI.  If we
     * ever have a need for more, then this hash table should be moved from
     * a static global to actually be part of the ref_list_p.
     */
    assert(str_table == NULL);

    str_table = puk_hashtable_new_string();

    if(!str_table)
    {
        return(NULL);
    }

    tmp_list = (ref_list_p) TBX_MALLOC(sizeof(struct list_head));
    if(!tmp_list)
    {
	puk_hashtable_delete(str_table);
        str_table = NULL;
        return(NULL);
    }

    INIT_LIST_HEAD(tmp_list);
    return (tmp_list);
}

/*
 * ref_list_add()
 *
 * adds a reference to the list.  
 *
 * no return value
 */
void ref_list_add(ref_list_p rlp,
		  ref_st_p rsp)
{
    if(rsp->id_string)
    {
	puk_hashtable_insert(str_table, rsp->id_string, &rsp->hash_link);
    }

    list_add(&(rsp->list_link), rlp);
}

/*
 * ref_list_search_addr()
 *
 * looks for a reference structure in the list that matches the given
 * BMI_addr_t.
 *
 * returns a pointer to the structure on success, a NULL on failure.
 */
ref_st_p ref_list_search_addr(ref_list_p rlp,
			      BMI_addr_t my_addr)
{
    /* TODO: Implement this ! */
    //return(id_gen_safe_lookup(my_addr));
    return NULL;
}


/*
 * ref_list_search_method_addr()
 *
 * looks for a reference structure in the list that matches the given
 * method_addr_p.
 *
 * returns a pointer to the structure on success, NULL on failure.
 */
ref_st_p ref_list_search_method_addr(ref_list_p rlp,
				     bmi_method_addr_p map)
{
    return(map->parent);
}

/*
 * ref_list_search_str()
 *
 * looks for a reference structure in the list that matches the given
 * id string.
 *
 * returns a pointer to the structure on success, a NULL on failure.
 */
ref_st_p ref_list_search_str(ref_list_p rlp,
			     const char *idstring)
{

    struct puk_hash_entry_s* tmp_link;

    tmp_link = puk_hashtable_lookup(str_table, (char*)idstring);
    if(!tmp_link)
    {
        return(NULL);
    }

    return(list_entry(tmp_link, ref_st, hash_link));
}

/*
 * ref_list_rem()
 *
 * removes the first match from the list - does not destroy it 
 *
 * returns a pointer to the structure on success, a NULL on failure.
 */
ref_st_p ref_list_rem(ref_list_p rlp,
		      BMI_addr_t my_addr)
{
    ref_st_p tmp_entry;
    
    /* TODO: get rid of this */
    tmp_entry = NULL;//id_gen_safe_lookup(my_addr);

    if(tmp_entry)
    {
        list_del(&tmp_entry->list_link);

        if(tmp_entry->id_string)
        {
            puk_hashtable_remove(str_table, tmp_entry->id_string);
        }
    }
    return (tmp_entry);
}



/*
 * ref_list_cleanup()
 *
 * frees up the list and all data associated with it.
 *
 * no return values
 */
void ref_list_cleanup(ref_list_p rlp)
{
    ref_list_p tmp_link = NULL;
    ref_list_p scratch = NULL;
    ref_st_p tmp_entry = NULL;

    list_for_each_safe(tmp_link, scratch, rlp)
    {
	tmp_entry = list_entry(tmp_link, struct ref_st,
			       list_link);
        dealloc_ref_st(tmp_entry);
    }

    puk_hashtable_delete(str_table);
    str_table = NULL;

    TBX_FREE(rlp);
    return;
}

/*
 * alloc_ref_st()
 *
 * allocates storage for a reference struct.
 *
 * returns a pointer to the new structure on success, NULL on failure.
 */
ref_st_p alloc_ref_st(void)
{

    int ssize = sizeof(struct ref_st);
    ref_st_p new_ref = NULL;

    new_ref = (ref_st_p) TBX_MALLOC(ssize);
    if (!new_ref)
    {
	return (NULL);
    }

    memset(new_ref, 0, ssize);

    /* we can go ahead and set the bmi_addr here */
    /* TODO: get rid of this! */
    //id_gen_safe_register(&(new_ref->bmi_addr), new_ref);

    return (new_ref);
}

/*
 * dealloc_ref_st()
 *
 * frees all memory associated with a reference structure
 * NOTE: it *does not*, however, destroy the associated method address.
 *
 * returns 0 on success, -1 on failure
 */
void dealloc_ref_st(ref_st_p deadref)
{

    if (!deadref)
    {
	return;
    }

    if (deadref->id_string)
    {
	TBX_FREE(deadref->id_string);
    }

    if (deadref->method_addr)
    {
	deadref->interface->set_info(BMI_DROP_ADDR, deadref->method_addr);
    }

    /* TODO: get rid of this */
    //id_gen_safe_unregister(deadref->bmi_addr);

    TBX_FREE(deadref);
}

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 *
 * vim: ts=8 sts=4 sw=4 expandtab
 */
