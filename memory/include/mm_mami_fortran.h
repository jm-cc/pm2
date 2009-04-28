/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2008 "the PM2 team" (see AUTHORS file)
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

/** \addtogroup mami */
/* @{ */

#if defined(MM_MAMI_ENABLED) && defined(MM_FORTRAN)

#ifndef MM_MAMI_FORTRAN_H
#define MM_MAMI_FORTRAN_H

#include <marcel.h>

extern
void mami_init_(int *memory_manager);

extern
void mami_exit_(int *memory_manager);

extern
void mami_unset_alignment_(int *memory_manager);

extern
void mami_malloc_(int *memory_manager,
                  int *size,
                  int *policy,
                  int *node,
                  int *buffer);

extern
void mami_free_(int *memory_manager,
                int *buffer);

extern
void mami_register_(int *memory_manager,
                    int *buffer,
                    int *size,
                    int *err);

extern
void mami_unregister_(int *memory_manager,
                      int *buffer,
                      int *err);

extern
void mami_locate_(int *memory_manager,
                  int *address,
                  int *size,
                  int *node,
                  int *err);

extern
void mami_print_(int *memory_manager);

extern
void mami_task_attach_(int *memory_manager,
                       int *buffer,
                       int *size,
                       marcel_t *owner,
                       int *node,
                       int *err);

extern
void mami_task_unattach_(int *memory_manager,
                         int *buffer,
                         marcel_t *owner,
                         int *err);

#endif /* MM_MAMI_FORTRAN_H */
#endif /* MM_MAMI_ENABLED */

/* @} */
