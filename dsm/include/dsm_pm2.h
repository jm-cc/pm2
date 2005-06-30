
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

#ifndef DSM_PM2_IS_DEF
#define DSM_PM2_IS_DEF

#include "dsm_const.h" 
#include "dsm_sysdep.h"

/* the following 2 need to be included by the user
 * program to allow:
 *  - protocol initialization using library functions
 *  - user-defined protocol epecification
 */

#include "dsm_protocol_lib.h" 
#include "dsm_protocol_policy.h"
#include "dsm_lock.h"
#include "token_lock.h"
#include "hierarch_topology.h"


/* to enable the user to configure the distribution of dsm pages */
#include "dsm_page_manager.h"

/* to enable the user to use dsm mutex */
#include "dsm_mutex.h"

#define BEGIN_DSM_DATA \
  asm (".data"); \
  asm (DSM_ASM_PAGEALIGN); \
  char dsm_data_begin = {0,}; 

#define END_DSM_DATA \
  char dsm_data_end = {0,}; \
  asm (DSM_ASM_PAGEALIGN);

#define DSM_NEWPAGE \
  asm (DSM_ASM_PAGEALIGN);

//void dsm_pm2_init(int my_rank, int confsize);

void dsm_pm2_init_before_startup_funcs(int my_rank, int confsize);
void dsm_pm2_init_after_startup_funcs(int my_rank, int confsize);

void dsm_pm2_exit();

#endif




