
/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 "the PM2 team" (see AUTHORS file)
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

#ifndef DSM_CONST_IS_DEF
#define DSM_CONST_IS_DEF

typedef int dsm_proto_t;

typedef unsigned long int dsm_page_index_t;

typedef enum
{ NO_ACCESS, READ_ACCESS, WRITE_ACCESS, UNKNOWN_ACCESS } dsm_access_t;

typedef int dsm_node_t;

extern const dsm_node_t NOBODY;



/* error codes returned by some functions */
enum
{
   /* this dummy value is to make sure the compiler will choose type
   * "int" for this enumeration (instead of "unsigned int") */
   DSM_ERR_DUMMY = -1,
   DSM_SUCCESS = 0,
   DSM_ERR_MEMORY,      /* memory exhausted */
   DSM_ERR_ILLEGAL,     /* illegal call / operation */
   DSM_ERR_NOT_INIT,    /* utilization of an object not initialized */
   DSM_ERR_MISC,        /* miscellaneous errors */
};

#endif


