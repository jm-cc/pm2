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

#include <unistd.h>
#include <sys/syscall.h>
#include <stdio.h>
#ifdef MARCEL
#  include <marcel.h>
#else
#  include <tbx_topology.h>
#endif

#include "mami_helper.h"
#include "mami_debug.h"

int _mami_mbind_call(void *start, unsigned long len, int mode,
                     const unsigned long *nmask, unsigned long maxnode, unsigned flags)
{
        if (_mami_use_synthetic_topology()) {
                return 0;
        }
#if defined (X86_64_ARCH) && defined (X86_ARCH)
        return syscall6(__NR_mbind, (long)start, len, mode, (long)nmask, maxnode, flags);
#else
        return syscall(__NR_mbind, (long)start, len, mode, (long)nmask, maxnode, flags);
#endif
}

int _mami_mbind(void *start, unsigned long len, int mode,
                const unsigned long *nmask, unsigned long maxnode, unsigned flags)
{
        int err = 0;

        MEMORY_ILOG_IN();
        mdebug_memory("binding on mask %lu\n", nmask?*nmask:0);
        err = _mami_mbind_call(start, len, mode, nmask, maxnode, flags);
        if (err < 0) perror("(_mami_mbind) mbind");
        MEMORY_ILOG_OUT();
        return err;
}

int _mami_move_pages(const void **pageaddrs, unsigned long pages, int *nodes, int *status, int flag)
{
        int err=0;

        MEMORY_ILOG_IN();
        if (_mami_use_synthetic_topology()) {
                MEMORY_ILOG_OUT();
                return err;
        }
        if (nodes) mdebug_memory("binding on numa node #%d\n", nodes[0]);

#if defined (X86_64_ARCH) && defined (X86_ARCH)
        err = syscall6(__NR_move_pages, 0, pages, pageaddrs, nodes, status, flag);
#else
        err = syscall(__NR_move_pages, 0, pages, pageaddrs, nodes, status, flag);
#endif
        if (err < 0) perror("(_mami_move_pages) move_pages");
        MEMORY_ILOG_OUT();
        return err;
}

int _mami_use_synthetic_topology(void)
{
#ifdef MARCEL
        return marcel_use_fake_topology;
#else
        return !hwloc_topology_is_thissystem(topology);
#endif
}

#endif /* MAMI_ENABLED */
