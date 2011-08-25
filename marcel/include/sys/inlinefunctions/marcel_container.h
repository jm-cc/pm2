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


#ifndef __INLINEFUNCTIONS_SYS_MARCEL_CONTAINER_H__
#define __INLINEFUNCTIONS_SYS_MARCEL_CONTAINER_H__


#ifdef __MARCEL_KERNEL__


static inline
int ma_container_nb_element(ma_container_t * container)
{
	return container->nb_element;
}

static inline
int ma_container_is_full(ma_container_t * container)
{
	return (ma_container_nb_element(container) == container->max_size);
}

static inline
void ma_lock_container(ma_container_t * container)
{
	ma_spin_lock(&(container->lock));
}

static inline
void ma_unlock_container(ma_container_t * container)
{
	ma_spin_unlock(&(container->lock));
}


#endif /** __MARCEL_KERNEL__ **/


#endif /** __INLINEFUNCTIONS_SYS_MARCEL_CONTAINER_H__ **/
