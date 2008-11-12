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

#section common
#if defined(MARCEL_MAMI_ENABLED) && defined(MARCEL_FORTRAN)

#section functions

void marcel_memory_init_(int *memory_manager);

void marcel_memory_exit_(int *memory_manager);

void marcel_memory_malloc_(int *memory_manager,
			   int *size,
			   int *buffer);

void marcel_memory_free_(int *memory_manager,
			 int *buffer);

void marcel_memory_register_(int *memory_manager,
			     int *buffer,
			     int *size,
			     int *node,
			     int *err);

void marcel_memory_unregister_(int *memory_manager,
			       int *buffer,
			       int *err);

void marcel_memory_locate_(int *memory_manager,
			   int *address,
			   int *node,
			   int *err);

void marcel_memory_print_(int *memory_manager);

void marcel_memory_task_attach_(int *memory_manager,
				int *buffer,
				marcel_t *owner,
				int *err);

void marcel_memory_task_unattach_(int *memory_manager,
				  int *buffer,
				  marcel_t *owner,
				  int *err);

#section common
#endif /* MARCEL_MAMI_ENABLED */

/* @} */