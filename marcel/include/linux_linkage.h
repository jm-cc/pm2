
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
#depend "asm/linux_linkage.h[marcel_macros]"

#section marcel_macros

#ifdef __cplusplus
#define CPP_ASMLINKAGE extern "C"
#else
#define CPP_ASMLINKAGE
#endif

#ifndef asmlinkage
#define asmlinkage CPP_ASMLINKAGE
#endif

//#ifndef __ALIGN
//#define __ALIGN		.align 4,0x90
//#define __ALIGN_STR	".align 4,0x90"
//#endif

#ifdef __ASSEMBLY__

//#define ALIGN __ALIGN
//#define ALIGN_STR __ALIGN_STR

//#define ENTRY(name) 
//  .globl name; 
//  ALIGN; 
//  name:

#endif

//#define NORET_TYPE    /**/
//#define ATTRIB_NORET  __attribute__((noreturn))
//#define NORET_AND     noreturn,

#ifndef FASTCALL
#define FASTCALL(x)	x
#define fastcall
#endif

