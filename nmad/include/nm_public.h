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


/* don't include pm2_common.h or tbx.h here. They are not needed and not ISO C compliant */
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <nm_errno.h>
#include <nm_config.h>

/* ** Config sanity checks ********************************* */

#if !defined(NMAD)
#  error "nmad CFLAGS not defined; please compile with CFLAGS returned by 'pkg-config --cflags nmad'"
#endif /* NMAD */

#if defined(NMAD_PIOMAN) && !defined(PIOMAN)
#  error "nmad was configured with pioman support; cannot build without pioman flags."
#endif

#if defined(PIOMAN) && !defined(NMAD_PIOMAN)
#  error "nmad was configured without pioman support; cannot build with pioman flags."
#endif

#if defined(NMAD_MARCEL) && !defined(MARCEL)
#  error "nmad was configured with Marcel threads; cannot build without Marcel flags."
#endif

#if defined(NMAD_PUKABI) && !defined(PUKABI)
#  error "nmad was configured with PuABI support; cannot build without PukABI flags."
#endif

#if defined(PUKABI) && !defined(NMAD_PUKABI)
#  error "nmad was configured without PuABI support; cannot build with PukABI flags."
#endif

#ifdef NMAD_ABT
#define main __abt_app_main
#endif /* PIOMAN_ABT */

/** config options that impact nmad ABI 
 * (content of structures exposed in API or used in inline code)
 */
struct nm_abi_config_s
{
  int enable_pioman;
  int enable_pthread;
  int enable_marcel;
  int enable_pukabi;
  int tags;
};
const static struct nm_abi_config_s nm_abi_config =
  {
    .enable_pioman =
#if defined(PIOMAN)
    1
#else /* PIOMAN */
    0
#endif /* PIOMAN */
    ,
    .enable_pthread =
#if defined(NMAD_PTHREAD)
    1
#else /* PTHREAD */
    0
#endif /* PTHREAD */
    ,
    .enable_marcel =
#if defined(MARCEL)
    1
#else /* MARCEL */
    0
#endif
    ,
    .enable_pukabi =
#if defined(PUKABI)
    1
#else /* PUKABI */
    0
#endif
    ,
    .tags =
#if defined(NM_TAGS_AS_FLAT_ARRAY)
    8
#elif defined(NM_TAGS_AS_HASHTABLE)
    32
#elif defined(NM_TAGS_AS_INDIRECT_HASH)
    64
#else
#error
#endif
  };
static inline void nm_abi_config_check(const struct nm_abi_config_s*p_nm_abi_config)
{
  if( (nm_abi_config.enable_pioman  != p_nm_abi_config->enable_pioman)  ||
      (nm_abi_config.enable_pthread != p_nm_abi_config->enable_pthread) ||
      (nm_abi_config.enable_marcel  != p_nm_abi_config->enable_marcel)  ||
      (nm_abi_config.enable_pukabi  != p_nm_abi_config->enable_pukabi)  ||
      (nm_abi_config.tags           != p_nm_abi_config->tags) )
    {
      fprintf(stderr, "# nmad: FATAL ERROR- ABI inconsistency detected between application and libnmad.\n"
	      "#               libnmad / application\n"
	      "# enable_pioman    %2d   /   %2d\n"
	      "# enable_pthread   %2d   /   %2d\n"
	      "# enable_marcel    %2d   /   %2d\n"
	      "# enable_pukabi    %2d   /   %2d\n"
      	      "# tags width       %2d   /   %2d\n",
	      nm_abi_config.enable_pioman,  p_nm_abi_config->enable_pioman,
	      nm_abi_config.enable_pthread, p_nm_abi_config->enable_pthread,
	      nm_abi_config.enable_marcel,  p_nm_abi_config->enable_marcel,
	      nm_abi_config.enable_pukabi,  p_nm_abi_config->enable_pukabi,
	      nm_abi_config.tags,           p_nm_abi_config->tags);
      abort();
    }
}

/* ** Driver *********************************************** */

/** a nmad driver; opaque type for the user */
typedef struct nm_drv_s*nm_drv_t;

/* ** Gates ************************************************ */

/** a gate; opaque type to designate a peer node */
typedef struct nm_gate_s*nm_gate_t;

/** no gate */
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

/** tag mask that matches all bits */
#define NM_TAG_MASK_FULL ((nm_tag_t)-1)
/** tag mask that matches no bits */
#define NM_TAG_MASK_NONE ((nm_tag_t)0)

/* ** packets length and offsets *************************** */

/** data length used by nmad */
typedef uint64_t nm_len_t;

/** length is undefined */
#define NM_LEN_UNDEFINED  ((nm_len_t)-1)


/* ** data iterators *************************************** */

#include <nm_data.h>

/** @} */

#endif /* NM_PUBLIC_H */


