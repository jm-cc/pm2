/*
 * NewMadeleine
 * Copyright (C) 2006 (see AUTHORS file)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

#ifndef NM_PUBLIC_H
#define NM_PUBLIC_H

/* ** This is the common public header for NewMad. It contains some
 * types and constants used throughout NewMad core and interfaces.
 *
 * The API for end-users are defined in the 'interfaces' directory.
 *
 */

/* don't include pm2_common.h or tbx.h here. They are not needed and not ISO C compliant */
#include <stdlib.h>
#include <stdint.h>
#include <nm_errno.h>


/* ** Gates ************************************************ */

typedef struct nm_gate*nm_gate_t;

#define NM_GATE_NONE ((nm_gate_t)NULL)

#define NM_ANY_GATE NM_GATE_NONE

/** Get the user-registered per-gate data */
void*nm_gate_ref_get(struct nm_gate*p_gate);

/** Set the user-registered per-gate data */
void nm_gate_ref_set(struct nm_gate*p_gate, void*ref);


/* ** Tags ************************************************* */

#if defined(CONFIG_TAG_AS_FLAT_ARRAY)
/* 8 bits, contained in flat array  */
#  define NM_TAGS_AS_FLAT_ARRAY
typedef uint8_t nm_tag_t;
#elif defined(CONFIG_TAG_AS_HASHTABLE)
/* 32 bits, contained in hashtable */
#  define NM_TAGS_AS_HASHTABLE
typedef uint32_t nm_tag_t;
#elif defined(CONFIG_TAG_HUGE)
/* 64 bits, contained in indirect hashtable */
#  define NM_TAGS_AS_INDIRECT_HASH
typedef uint64_t nm_tag_t; 
#else
#  error NewMad: no container type defined for tag-indexed tables
#endif

#define NM_TAG_MASK_FULL ((nm_tag_t)-1)
#define NM_TAG_MASK_NONE ((nm_tag_t)0)


/* ** datatypes ******************************************** */

struct CCSI_Segment;


#endif /* NM_PUBLIC_H */


