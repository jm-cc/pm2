/* MaMI --- NUMA Memory Interface
 *
 * Copyright (C) 2008, 2009, 2010, 2011  Centre National de la Recherche Scientifique
 *
 * MaMI is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or (at
 * your option) any later version.
 *
 * MaMI is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU Lesser General Public License in COPYING.LGPL for more details.
 */
#include <errno.h>

#define MAMI_TEST_SKIPPED 77

void *ALL_IS_OK = (void *)123456789L;
void *ALL_IS_NOT_OK = (void *)987654321L;

#define MAMI_CHECK_MALLOC(ptr) {if (!ptr) { fprintf(stderr, "mami_malloc failed\n"); return 1; }}
#define MAMI_CHECK_MALLOC_HAS_FAILED(ptr) {if (ptr) { fprintf(stderr, "mami_malloc should have failed\n"); return 1; }}
#define MAMI_CHECK_RETURN_VALUE(err, message) {if (err < 0) { perror(message); return 1; }}
#define MAMI_CHECK_RETURN_VALUE_IS(err, value, message) {if (err >= 0 || errno != value) { perror(message); return 1; }}

#define MAMI_CHECK_MALLOC_THREAD(ptr) {if (!ptr) { fprintf(stderr, "mami_malloc failed\n"); return ALL_IS_NOT_OK; }}
#define MAMI_CHECK_MALLOC_HAS_FAILED_THREAD(ptr) {if (ptr) { fprintf(stderr, "mami_malloc should have failed\n"); return ALL_IS_NOT_OK; }}
#define MAMI_CHECK_RETURN_VALUE_THREAD(err, message) {if (err < 0) { perror(message); return ALL_IS_NOT_OK; }}
#define MAMI_CHECK_RETURN_VALUE_IS_THREAD(err, value, message) {if (err >= 0 || errno != value) { perror(message); return ALL_IS_NOT_OK; }}

//#define MAMI_TEST_OUTPUT
#ifdef MAMI_TEST_OUTPUT
#  define mprintf(ofile, fmt, args...) do { fprintf(ofile,"[%s] " fmt , __func__, ##args); } while(0)
#else
#  define mprintf(ofile, fmt, args...)
#endif
