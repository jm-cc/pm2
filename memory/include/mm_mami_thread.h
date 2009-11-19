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

#ifdef MM_MAMI_ENABLED

#ifndef MM_MAMI_THREAD_H
#define MM_MAMI_THREAD_H

#ifdef MARCEL

#  define th_mami_malloc(psize) tmalloc(psize)
#  define th_mami_free(ptr) tfree(ptr)

#  define th_mami_sprintf(str, format, ...) marcel_sprintf(str, format, ##__VA_ARGS__)
#  define th_mami_snprintf(str, size, format, ...) marcel_snprintf(str, size, format, ##__VA_ARGS__)
#  define th_mami_fprintf(output, ...) marcel_fprintf(output, ##__VA_ARGS__)
#  define th_mami_fgets(s, size, stream) marcel_fgets(s, size, stream)
#  define th_mami_feof(stream) marcel_feof(stream)
#  define th_mami_fclose(stream) marcel_fclose(stream)
#  define th_mami_fopen(path, mode) marcel_fopen(path, mode)
#  define th_mami_fscanf(stream, format, ...) marcel_fscanf(stream, format, ##__VA_ARGS__)
#  define th_mami_fflush(stream) marcel_fflush(stream)

#  define th_mami_mutex_t		marcel_mutex_t
#  define th_mami_mutex_init		marcel_mutex_init
#  define th_mami_mutex_lock		marcel_mutex_lock
#  define th_mami_mutex_unlock		marcel_mutex_unlock

#  define th_mami_t			marcel_t
#  define th_mami_create		marcel_create
#  define th_mami_join			marcel_join

#  define th_mami_attr_t		marcel_attr_t
#  define th_mami_attr_init		marcel_attr_init
#  define th_mami_attr_setnode_level(attr, node)   marcel_attr_settopo_level(attr, &marcel_topo_node_level[node])

#  define th_mami_self              	marcel_self
#  define th_mami_current_node()	marcel_current_node_level()->os_node


#else

#  define th_mami_malloc(psize) malloc(psize)
#  define th_mami_free(ptr) free(ptr)

#  define th_mami_sprintf(str, format, ...) sprintf(str, format, ##__VA_ARGS__)
#  define th_mami_snprintf(str, size, format, ...) snprintf(str, size, format, ##__VA_ARGS__)
#  define th_mami_fprintf(output, ...) fprintf(output, ##__VA_ARGS__)
#  define th_mami_fgets(s, size, stream) fgets(s, size, stream)
#  define th_mami_feof(stream) feof(stream)
#  define th_mami_fclose(stream) fclose(stream)
#  define th_mami_fopen(path, mode) fopen(path, mode)
#  define th_mami_fscanf(stream, format, ...) fscanf(stream, format, ##__VA_ARGS__)
#  define th_mami_fflush(stream) fflush(stream)

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

#endif /* MM_MAMI_THREAD_H */
#endif /* MM_MAMI_ENABLED */
