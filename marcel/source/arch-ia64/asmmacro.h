
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

/* Come from include/asm-ia64/asmmacro.h */

#define ENTRY(name)                             \
        .align 32;                              \
        .proc name;                             \
name:

#define ENTRY_MIN_ALIGN(name)                   \
        .align 16;                              \
        .proc name;                             \
name:

#define GLOBAL_ENTRY(name)                      \
        .global name;                           \
        ENTRY(name)

#define END(name)                               \
        .endp name

