
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

#ifndef DSM_PAGE_MAN_IS_DEF
#define DSM_PAGE_MAN_IS_DEF

#include "dsm_const.h"
#include "marcel.h"

dsm_node_t dsm_self();

boolean dsm_static_addr(void *addr);

void *dsm_page_base(void *addr);

void dsm_page_table_init(int my_rank, int confsize);

void dsm_set_no_access();

void dsm_pseudo_static_set_no_access();

void dsm_set_write_access();

void dsm_pseudo_static_set_write_access();

void dsm_set_uniform_access(dsm_access_t access);

void dsm_pseudo_static_set_uniform_access(dsm_access_t access);

void *dsm_get_pseudo_static_dsm_start_addr();

void dsm_protect_page (void *addr, dsm_access_t access);

unsigned int dsm_page_offset(void *addr);

dsm_page_index_t dsm_page_index(void *addr);

dsm_page_index_t dsm_isoaddr_page_index(int index);

unsigned long dsm_get_nb_static_pages();

unsigned long dsm_get_nb_pseudo_static_pages();

void dsm_set_prob_owner(dsm_page_index_t index, dsm_node_t owner);

dsm_node_t dsm_get_prob_owner(dsm_page_index_t index);

void dsm_set_next_owner(dsm_page_index_t index, dsm_node_t next_owner);
 
void dsm_clear_next_owner(dsm_page_index_t index);

dsm_node_t dsm_get_next_owner(dsm_page_index_t index);

void dsm_store_pending_request(dsm_page_index_t index, dsm_node_t owner);

dsm_node_t dsm_get_next_pending_request(dsm_page_index_t index);

boolean dsm_next_owner_is_set(dsm_page_index_t index);

void dsm_set_access(dsm_page_index_t index, dsm_access_t access);

void dsm_set_access_without_protect(dsm_page_index_t index, dsm_access_t access);

dsm_access_t dsm_get_access(dsm_page_index_t index);

void dsm_set_pending_access(dsm_page_index_t index, dsm_access_t access);

dsm_access_t dsm_get_pending_access(dsm_page_index_t index);

void dsm_set_page_size(dsm_page_index_t index, unsigned long size);

unsigned long dsm_get_page_size(dsm_page_index_t index);

void dsm_set_page_addr(dsm_page_index_t index, void *addr);

void *dsm_get_page_addr(dsm_page_index_t index);

void dsm_wait_for_page(dsm_page_index_t index);

void dsm_signal_page_ready(dsm_page_index_t index);

void dsm_lock_page(dsm_page_index_t index);

void dsm_unlock_page(dsm_page_index_t index);

void dsm_wait_for_inv(dsm_page_index_t index);

void dsm_signal_inv_done(dsm_page_index_t index);

void dsm_lock_inv(dsm_page_index_t index);

void dsm_unlock_inv(dsm_page_index_t index);

boolean dsm_pending_invalidation(dsm_page_index_t index);

void dsm_set_pending_invalidation(dsm_page_index_t index);

void dsm_clear_pending_invalidation(dsm_page_index_t index);

void dsm_wait_ack(dsm_page_index_t index, int value);

void dsm_signal_ack(dsm_page_index_t index, int value);

int dsm_get_copyset_size(dsm_page_index_t index);

void dsm_add_to_copyset(dsm_page_index_t index, dsm_node_t node);

dsm_node_t dsm_get_next_from_copyset(dsm_page_index_t index);

void dsm_remove_from_copyset(dsm_page_index_t index, dsm_node_t node);

void pm2_set_dsm_page_distribution(int mode, ...);

void dsm_display_page_ownership();

#define DSM_CENTRALIZED 0
#define DSM_BLOCK       1
#define DSM_CYCLIC      2
#define DSM_CUSTOM      3

#define DSM_OWNER_WRITE_ACCESS_OTHER_NO_ACCESS 0
#define DSM_OWNER_READ_ACCESS_OTHER_NO_ACCESS 1
#define DSM_UNIFORM_PROTECT 2
#define DSM_CUSTOM_PROTECT 3

void pm2_set_dsm_page_protection(int mode, ...);

int dsm_get_user_data1(dsm_page_index_t index, int rank);

void dsm_set_user_data1(dsm_page_index_t index, int rank, int value);

void dsm_increment_user_data1(dsm_page_index_t index, int rank);

void dsm_decrement_user_data1(dsm_page_index_t index, int rank);

void dsm_alloc_user_data2(dsm_page_index_t index, int size);

void dsm_free_user_data2(dsm_page_index_t index);

void *dsm_get_user_data2(dsm_page_index_t index);

void dsm_set_user_data2(dsm_page_index_t index, void *addr);

void dsm_alloc_twin(dsm_page_index_t index);

void dsm_free_twin(dsm_page_index_t index);

void *dsm_get_twin(dsm_page_index_t index);

void dsm_set_twin(dsm_page_index_t index, void *addr);

typedef void (*dsm_user_data1_init_func_t)(int *userdata); /* init func for userdata1 */

typedef void (*dsm_user_data2_init_func_t)(void **userdata); /* init func for userdata2 */

void dsm_set_user_data1_init_func(dsm_user_data1_init_func_t func);

void dsm_set_user_data2_init_func(dsm_user_data2_init_func_t func);

void dsm_set_pseudo_static_area_size(unsigned size);

boolean dsm_empty_page_entry(dsm_page_index_t index);

void dsm_alloc_page_entry(dsm_page_index_t index);

void dsm_validate_page_entry(dsm_page_index_t index);

void dsm_enable_page_entry(dsm_page_index_t index, dsm_node_t owner, int protocol, void *addr, size_t size, boolean map);

void dsm_set_default_protocol(int protocol);

void dsm_set_page_protocol(dsm_page_index_t index, int protocol);

int dsm_get_page_protocol(dsm_page_index_t index);

void dsm_set_page_ownership(void *addr, dsm_node_t owner);

/*********************** Hyperion stuff: ****************************/

void dsm_invalidate_not_owned_pages();

void dsm_pseudo_static_invalidate_not_owned_pages();

void dsm_invalidate_not_owned_pages_without_protect();

void dsm_pseudo_static_invalidate_not_owned_pages_without_protect();

void dsm_alloc_page_bitmap(dsm_page_index_t index); 

void dsm_free_page_bitmap(dsm_page_index_t index);

void dsm_mark_dirty(void *addr, int length);

void *dsm_get_next_modified_data(dsm_page_index_t index, int *size);

boolean dsm_page_bitmap_is_empty(dsm_page_index_t index);

void dsm_page_bitmap_clear(dsm_page_index_t index) ;

boolean dsm_page_bitmap_is_allocated(dsm_page_index_t index);

/* user code to migrate to hyperion files: */

#define dsm_get_waiter_count(index)  dsm_get_user_data1(index, 0)

#define dsm_clear_waiter_count(index)  dsm_set_user_data1(index, 0, 0)

#define dsm_increment_waiter_count(index) dsm_increment_user_data1(index, 0)

#define dsm_get_page_arrival_count(index)  dsm_get_user_data1(index, 1)

#define dsm_clear_page_arrival_count(index)  dsm_set_user_data1(index, 1, 0)

#define dsm_increment_page_arrival_count(index) dsm_increment_user_data1(index, 1)

#endif


