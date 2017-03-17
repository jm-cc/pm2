/*
 * NewMadeleine
 * Copyright (C) 2006-2017 (see AUTHORS file)
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

/** @ingroup core_interface
 * @{
 */

#ifndef NMAD
#error NMAD flags not defined. Please compile with flags returned by pkg-config --cflags nmad
#endif /* NMAD */

#if ((!defined(__pic__)) && (!defined(__PIC__)))
#  error "nmad needs Position Independant Code; please build with -fPIC compilation flag."
#endif

/* don't include pm2_common.h or tbx.h here. They are not needed and not ISO C compliant */
#include <stdlib.h>
#include <stdint.h>
#include <nm_errno.h>
#include <nm_config.h>

#ifdef NMAD_ABT
#define main __abt_app_main
#endif /* PIOMAN_ABT */

/* ** Driver *********************************************** */

typedef struct nm_drv_s*nm_drv_t;

/* ** Gates ************************************************ */

typedef struct nm_gate_s*nm_gate_t;

#define NM_GATE_NONE ((nm_gate_t)NULL)

#define NM_ANY_GATE NM_GATE_NONE

/** Get the user-registered per-gate data */
void*nm_gate_ref_get(nm_gate_t p_gate);

/** Set the user-registered per-gate data */
void nm_gate_ref_set(nm_gate_t p_gate, void*ref);


/* ** Tags ************************************************* */

/* 8 bits, contained in flat array  */
#if defined(NM_TAGS_AS_FLAT_ARRAY)
typedef uint8_t nm_tag_t;
#elif defined(NM_TAGS_AS_HASHTABLE)
/* 32 bits, contained in hashtable */
typedef uint32_t nm_tag_t;
#elif defined(NM_TAGS_AS_INDIRECT_HASH)
/* 64 bits, contained in indirect hashtable */
typedef uint64_t nm_tag_t; 
#else
#  error NewMad: no container type defined for tag-indexed tables
#endif

#define NM_TAG_MASK_FULL ((nm_tag_t)-1)
#define NM_TAG_MASK_NONE ((nm_tag_t)0)

/* ** packets length and offsets *************************** */

typedef uint64_t nm_len_t;

/** length is undefined */
#define NM_LEN_UNDEFINED  ((nm_len_t)-1)


/* ** data iterators *************************************** */

#include <nm_data.h>

/** @} */

#endif /* NM_PUBLIC_H */


