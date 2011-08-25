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
#ifdef MAMI_ENABLED

#ifndef MAMI_THREAD_H
#define MAMI_THREAD_H

#ifdef MARCEL

#  define th_mami_mutex_t		marcel_mutex_t
#  define th_mami_mutex_init		marcel_mutex_init
#  define th_mami_mutex_lock		marcel_mutex_lock
#  define th_mami_mutex_unlock		marcel_mutex_unlock

#  define th_mami_t			marcel_t
#  define th_mami_create		marcel_create
#  define th_mami_join			marcel_join

#  define th_mami_attr_t		marcel_attr_t
#  define th_mami_attr_init		marcel_attr_init
#  define th_mami_attr_setnode_level(attr, node) marcel_attr_settopo_level_on_node(attr, node)

#  define th_mami_self              	marcel_self
#  define th_mami_current_node()	marcel_current_os_node()


#else
#include <pthread.h>

#  define th_mami_mutex_t             pthread_mutex_t
#  define th_mami_mutex_init          pthread_mutex_init
#  define th_mami_mutex_lock          pthread_mutex_lock
#  define th_mami_mutex_unlock        pthread_mutex_unlock

#  define th_mami_t                   pthread_t
#  define th_mami_create              pthread_create
#  define th_mami_join                pthread_join

#  define th_mami_attr_t              pthread_attr_t
#  define th_mami_attr_init           pthread_attr_init
#  define th_mami_attr_setnode_level(attr, node)   _mami_attr_settopo_level(attr, node)

#  define th_mami_self                pthread_self
#  define th_mami_current_node	      _mami_current_node

typedef void* any_t;


#endif

#endif /* MAMI_THREAD_H */
#endif /* MAMI_ENABLED */
