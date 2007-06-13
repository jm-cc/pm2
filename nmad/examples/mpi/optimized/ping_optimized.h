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

#ifndef PING_OPTIMIZED_H
#define PING_OPTIMIZED_H

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#include <tbx.h>

#include <nm_public.h>
#include <nm_so_public.h>

#include <nm_so_pack_interface.h>

#define MIN_SIZE   4
#define MAX_SIZE   1024 * 1024 * 4
#define MIN_BLOCKS 2
#define MAX_BLOCKS 10
#define LOOPS      2000

#define RWAIT
//#define NO_RWAIT

#if defined(CONFIG_MULTI_RAIL)
#  define STRATEGY "multi_rail"
#elif defined(CONFIG_STRAT_DEFAULT)
#  define STRATEGY "strat_default"
#elif defined(CONFIG_STRAT_AGGREG)
#  define STRATEGY "strat_aggreg"
#else
#  define STRATEGY "strat_default"
#endif

//#define PRINTF(...) { printf(__VA_ARGS__) ; }
#define PRINTF(...) { }

//#define DEBUG

#if defined(DEBUG)
#define DEBUG_PRINTF(...) { fprintf(stderr,__VA_ARGS__) ; }
#undef LOOPS
#define LOOPS 1
#undef MAX_SIZE 
#define MAX_SIZE 16
#else /* ! DEBUG */
#define DEBUG_PRINTF(...) { }
#endif /* DEBUG */

/*
 * EMULATION OF THE MPI DATATYPE
 */
typedef enum {
    MPIR_INT, MPIR_FLOAT, MPIR_DOUBLE, MPIR_COMPLEX, MPIR_LONG, MPIR_SHORT,
    MPIR_CHAR, MPIR_BYTE, MPIR_UCHAR, MPIR_USHORT, MPIR_ULONG, MPIR_UINT,
    MPIR_CONTIG, MPIR_VECTOR, MPIR_HVECTOR,
    MPIR_INDEXED, MPIR_HINDEXED, MPIR_STRUCT, MPIR_DOUBLE_COMPLEX, MPIR_PACKED,
    MPIR_UB, MPIR_LB, MPIR_LONGDOUBLE, MPIR_LONGLONGINT,
    MPIR_LOGICAL, MPIR_FORT_INT
} MPIR_NODETYPE;

struct MPIR_DATATYPE {
  MPIR_NODETYPE dte_type; /* type of datatype element this is */
  int          committed; /* whether committed or not */
  int          is_contig; /* whether entirely contiguous */
  int              basic; /* Is this a basic type */
  int          permanent; /* Is this a permanent type */
  int               size; /* size of type */
  int           elements; /* number of basic elements */
  int          ref_count; /* nodes depending on this node */
  int              align; /* alignment needed for start of datatype */
  int              count; /* replication count */
  int        stride; /* stride, for VECTOR and HVECTOR types */
  int      *indices; /* array of indices, for (H)INDEXED, STRUCT */
  int           blocklen; /* blocklen, for VECTOR and HVECTOR types */
  int         *blocklens; /* array of blocklens for (H)INDEXED, STRUCT */
  struct MPIR_DATATYPE *old_type,
    **old_types,
    *flattened;
  int        nb_elements;   /* number of elements for STRUCT */
};

#endif /* PING_OPTIMIZED_H */

