
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

#ifndef DSM_SYSDEP_IS_DEF
#define DSM_SYSDEP_IS_DEF

#include "dsm_page_size.h" /* generated file */
#include "dsm_const.h"
#include "dsm_page_manager.h"

typedef void ( *dsm_pagefault_handler_t)(int sig, void *addr, dsm_access_t access);
typedef void (*dsm_std_handler_t )(int); 

void dsm_install_pagefault_handler(dsm_pagefault_handler_t handler);

void dsm_uninstall_pagefault_handler();

void dsm_setup_secondary_SIGSEGV_handler(dsm_std_handler_t handler_func);

typedef void (*dsm_safe_ss_t)(char*t);

void dsm_safe_ss(dsm_safe_ss_t f);

#endif

