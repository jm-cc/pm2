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
/** \addtogroup mami */
/* @{ */

#if defined(MAMI_ENABLED) && defined(MAMI_FORTRAN)

#ifndef MAMI_FORTRAN_H
#define MAMI_FORTRAN_H

#ifdef MARCEL
#  include <marcel.h>
#endif
#include "mami.h"

extern
void mami_init_(mami_manager_t **memory_manager);

extern
void mami_exit_(mami_manager_t **memory_manager);

extern
void mami_unset_alignment_(mami_manager_t **memory_manager);

extern
void mami_malloc_(mami_manager_t **memory_manager,
                  size_t *size,
                  int *policy,
                  int *node,
                  void *buffer);

extern
void mami_free_(mami_manager_t **memory_manager,
                void *buffer);

extern
void mami_register_(mami_manager_t **memory_manager,
                    void *buffer,
                    size_t *size,
                    int *err);

extern
void mami_unregister_(mami_manager_t **memory_manager,
                      void *buffer,
                      int *err);

extern
void mami_locate_(mami_manager_t **memory_manager,
                  void *address,
                  size_t *size,
                  int *node,
                  int *err);

extern
void mami_print_(mami_manager_t **memory_manager);

#ifdef MARCEL
extern
void mami_task_attach_(mami_manager_t **memory_manager,
                       void *buffer,
                       size_t *size,
                       marcel_t *owner,
                       int *node,
                       int *err);

extern
void mami_task_unattach_(mami_manager_t **memory_manager,
                         void *buffer,
                         marcel_t *owner,
                         int *err);

extern
void mami_task_migrate_all_(mami_manager_t **memory_manager,
                            marcel_t *owner,
                            int *node,
                            int *err);
#endif /* MARCEL */

extern
void mami_migrate_on_next_touch_(mami_manager_t **memory_manager,
                                 void *buffer,
                                 int *err);

#endif /* MAMI_FORTRAN_H */
#endif /* MAMI_ENABLED */

/* @} */
