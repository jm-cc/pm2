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
#if defined(MAMI_ENABLED)

#ifndef MAMI_DEBUG_H
#define MAMI_DEBUG_H

#ifdef MEMORY_DEBUG

extern unsigned long _mami_debug_mask;

#  define MDEBUG_MEMORY_DEBUG_SET 2
#  define MDEBUG_MEMORY_LOG_SET   4
#  define MDEBUG_MEMORY_ILOG_SET  8

#  define mdebug_memory_isset(flag)   ((_mami_debug_mask & flag) == flag)
#  define mdebug_memory(fmt, args...) do { if (mdebug_memory_isset(MDEBUG_MEMORY_DEBUG_SET)) fprintf(stderr,"[%s] " fmt , __func__, ##args); } while(0)
#  define MEMORY_LOG_IN()             do { if (mdebug_memory_isset(MDEBUG_MEMORY_LOG_SET)) fprintf(stderr,"%s: -->\n", __func__); } while(0)
#  define MEMORY_LOG_OUT()            do { if (mdebug_memory_isset(MDEBUG_MEMORY_LOG_SET)) fprintf(stderr,"%s: <--\n", __func__); } while(0)
#  define MEMORY_ILOG_IN()            do { if (mdebug_memory_isset(MDEBUG_MEMORY_ILOG_SET)) fprintf(stderr,"%s: -->\n", __func__); } while(0)
#  define MEMORY_ILOG_OUT()           do { if (mdebug_memory_isset(MDEBUG_MEMORY_ILOG_SET)) fprintf(stderr,"%s: <--\n", __func__); } while(0)

#else /* ! MEMORY_DEBUG */

#  define mdebug_memory(fmt, args...)
#  define MEMORY_LOG_IN()
#  define MEMORY_LOG_OUT()
#  define MEMORY_ILOG_IN()
#  define MEMORY_ILOG_OUT()

#endif /* MEMORY_DEBUG */

void mami_debug_init(int argc, char **argv);


#endif /* MAMI_DEBUG_H */
#endif /* MAMI_ENABLED */
