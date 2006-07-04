
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

#section common
/*
 * Similar to:
 * include/asm-i386/linkage.h
 */
#section marcel_macros

#ifdef WIN_SYS
#define asmlinkage CPP_ASMLINKAGE __attribute__((__stdcall__))
#define FASTCALL(x)	x __attribute__((__fastcall__))
#define fastcall	__attribute__((__fastcall__))
#else
#define asmlinkage CPP_ASMLINKAGE __attribute__((regparm(0)))
#define FASTCALL(x)	x __attribute__((regparm(3)))
#define fastcall	__attribute__((regparm(3)))
#endif

//#ifdef CONFIG_X86_ALIGNMENT_16
//#define __ALIGN .align 16,0x90
//#define __ALIGN_STR ".align 16,0x90"
//#endif

