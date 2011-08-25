/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 the PM2 team (see AUTHORS file)
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


/**
 * Temporary header file to facilitate the support for 
 * hwloc-1.0 and hwloc-1.1
 *
 **/


#ifndef __MARCEL_HWLOCAPI_H__
#define __MARCEL_HWLOCAPI_H__


#include <hwloc.h>


#ifdef __MARCEL_KERNEL__


#if HWLOC_API_VERSION >= 0x00010100 // hwloc-1.1 and above
#define ma_hwlocapi_get_board_info(tlevel, dmi_board_name, dmi_board_vendor) \
	{								\
		dmi_board_name = hwloc_obj_get_info_by_name(tlevel, "DMIBoardName"); \
		dmi_board_vendor = hwloc_obj_get_info_by_name(tlevel, "DMIBoardVendor"); \
	}
#else
#define ma_hwlocapi_get_board_info(tlevel, dmi_board_name, dmi_board_vendor) \
	{								\
		if (tlevel->attr) {					\
			dmi_board_name = tlevel->attr->machine.dmi_board_name; \
			dmi_board_vendor = tlevel->attr->machine.dmi_board_vendor; \
		}							\
	}
#endif


/* hwloc <1.1 does not offer the bitmap API yet */
#ifndef HWLOC_BITMAP_H
#  define hwloc_bitmap_t hwloc_cpuset_t
#  define hwloc_const_bitmap_t hwloc_const_cpuset_t
#  define hwloc_bitmap_alloc hwloc_cpuset_alloc
#  define hwloc_bitmap_free hwloc_cpuset_free
#  define hwloc_bitmap_first hwloc_cpuset_first
#  define hwloc_bitmap_to_ith_ulong hwloc_cpuset_to_ith_ulong
#  define hwloc_bitmap_set hwloc_cpuset_set
#  define hwloc_bitmap_from_ith_ulong hwloc_cpuset_from_ith_ulong
#  define hwloc_bitmap_isfull hwloc_cpuset_isfull
#  define hwloc_bitmap_iszero hwloc_cpuset_iszero
#endif


#endif /** __MARCEL_KERNEL__ **/


#endif /** __MARCEL_HWLOCAPI_H__ **/
