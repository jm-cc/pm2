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
 * Header file for reference list management functions.
 */

#ifndef __REFERENCE_LIST_H
#define __REFERENCE_LIST_H

#include <nm_public.h>

/* BMI is only available for 'huge tags' (ie. at least 64 bits) */
#ifdef NM_TAGS_AS_INDIRECT_HASH

#include "bmi-types.h"
#include "bmi-method-support.h"
#include <nm_private.h>

typedef tbx_slist_t* ref_list_p;

/********************************************************************
 * reference list management prototypes
 */

ref_list_p ref_list_new(void);
void ref_list_add(ref_list_p rlp,
		  void* addr);
void* ref_list_search_addr(ref_list_p rlp,
				void* addr);

void* ref_list_search_gate(ref_list_p rlp,
			   void* p_gate);

void* ref_list_search_str(ref_list_p rlp,
			  const char *idstring);
void ref_list_cleanup(ref_list_p rlp);

#endif /* NM_TAGS_AS_INDIRECT_HASH */

#endif /* __REFERENCE_LIST_H */

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 *
 * vim: ts=8 sts=4 sw=4 expandtab
 */
