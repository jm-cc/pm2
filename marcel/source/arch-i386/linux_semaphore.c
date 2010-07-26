
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

#include "marcel.h"

/*
 * rw spinlock fallbacks
 */
#ifdef MA__LWPS
asm(".text\n"
    ".align	4\n"
    ".globl	__ma_write_lock_failed\n"
    ".internal	__ma_write_lock_failed\n"
    "__ma_write_lock_failed:\n\t"
    MA_LOCK_PREFIX "addl	$" MA_RW_LOCK_BIAS_STR ",(%eax)\n"
    "1:	rep; nop\n\t"
    "cmpl	$" MA_RW_LOCK_BIAS_STR ",(%eax)\n\t"
    "jne	1b\n\t"
    MA_LOCK_PREFIX "subl	$" MA_RW_LOCK_BIAS_STR ",(%eax)\n\t"
    "jnz	__ma_write_lock_failed\n\t" "ret\n\t"
#ifndef DARWIN_SYS
    ".size __ma_write_lock_failed,.-__ma_write_lock_failed"
#endif
    );

asm(".text\n"
    ".align	4\n"
    ".globl	__ma_read_lock_failed\n"
    ".internal	__ma_read_lock_failed\n"
    "__ma_read_lock_failed:\n\t"
    MA_LOCK_PREFIX "incl	(%eax)\n"
    "1:	rep; nop\n\t"
    "cmpl	$1,(%eax)\n\t"
    "js	1b\n\t"
    MA_LOCK_PREFIX "decl	(%eax)\n\t" "js	__ma_read_lock_failed\n\t" "ret\n\t"
#ifndef DARWIN_SYS
    ".size __ma_read_lock_failed,.-__ma_read_lock_failed"
#endif
    );
#endif
