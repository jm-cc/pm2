
#ifndef DSM_SYSDEP_IS_DEF
#define DSM_SYSDEP_IS_DEF

#include "dsm_page_size.h" /* generated file */
#include "dsm_const.h"

typedef void ( *dsm_pagefault_handler_t)(int sig, void *addr, dsm_access_t access);
typedef void (*dsm_std_handler_t )(int); 

void dsm_install_pagefault_handler(dsm_pagefault_handler_t handler);

void dsm_uninstall_pagefault_handler();

void dsm_setup_secondary_SIGSEGV_handler(dsm_std_handler_t handler_func);

typedef void (*dsm_safe_ss_t)(char*t);

void dsm_safe_ss(dsm_safe_ss_t f);

#endif

