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
#if defined(MAMI_ENABLED) && defined(MAMI_FORTRAN)

#include "mami_fortran.h"
#include "mami.h"
#include "mami_private.h"

void mami_init_(mami_manager_t **memory_manager)
{
        mami_init(memory_manager, 0, NULL);
}

void mami_exit_(mami_manager_t **memory_manager)
{
        mami_exit(memory_manager);
}

void mami_unset_alignment_(mami_manager_t **memory_manager)
{
        mami_unset_alignment(*memory_manager);
}

void mami_malloc_(mami_manager_t **memory_manager,
                  size_t *size,
                  int *policy,
                  int *node,
                  void *buffer)
{
        buffer = mami_malloc(*memory_manager, *size, *policy, *node);
}

void mami_free_(mami_manager_t **memory_manager,
                void *buffer)
{
        mami_free(*memory_manager, buffer);
}

void mami_register_(mami_manager_t **memory_manager,
                    void *buffer,
                    size_t *size,
                    int *err)
{
        int _err;
        _err = mami_register(*memory_manager, buffer, *size);
        *err = _err;
}

void mami_unregister_(mami_manager_t **memory_manager,
                      void *buffer,
                      int *err)
{
        int _err;
        _err = mami_unregister(*memory_manager, buffer);
        *err = _err;
}

void mami_locate_(mami_manager_t **memory_manager,
                  void *address,
                  size_t *size,
                  int *node,
                  int *err)
{
        int _node, _err;
        _err = mami_locate(*memory_manager, address, *size, &_node);
        *node = _node;
        *err = _err;
}

void mami_print_(mami_manager_t **memory_manager)
{
        mami_print(*memory_manager);
}

#ifdef MARCEL
void mami_task_attach_(mami_manager_t **memory_manager,
                       void *buffer,
                       size_t *size,
                       marcel_t *owner,
                       int *node,
                       int *err)
{
        int _node, _err;
        _err = mami_task_attach(*memory_manager, buffer, *size, *owner, &_node);
        *node = _node;
        *err = _err;
}

void mami_task_unattach_(mami_manager_t **memory_manager,
                         void *buffer,
                         marcel_t *owner,
                         int *err)
{
        int _err;
        _err = mami_task_unattach(*memory_manager, buffer, *owner);
        *err = _err;
}

void mami_task_migrate_all_(mami_manager_t **memory_manager,
                            marcel_t *owner,
                            int *node,
                            int *err)
{
        int _err;
        _err = mami_task_migrate_all(*memory_manager, *owner, *node);
        *err = _err;
}
#endif /* MARCEL */

void mami_migrate_on_next_touch_(mami_manager_t **memory_manager,
                                 void *buffer,
                                 int *err)
{
        int _err;
        _err = mami_migrate_on_next_touch(*memory_manager, buffer);
        *err = _err;
}

#endif
