
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

#if defined(UNICOS_SYS)
        .ident asm_asm ; debut du module
 
        .psect get_sp@code,code,cache
get_sp::
        stq r30, 0(r16)
        ret r31, (r26)
        .endp
 
 
        .psect set_sp@code,code,cache
set_sp::
        addq r16,r31,r30
        ret r31, (r26)
 
        .endp    ;fin du module
 
        .psect set_fp@code,code,cache
set_fp::
        addq r16,r31,r15
        ret r31, (r26)
 
        .endp    ;fin du module
 
 
        .psect get_fp@code,code,cache
get_fp::
        stq r15, 0(r16)
        ret r31, (r26)
        .endp
 
        .end
#endif

#if defined(OSF_SYS)
        .align 2
        .globl dummy
        .ent dummy
dummy:
        .frame $15,16,$26,0
        .mask 0x4008000,-16
        lda $30,-16($30)
        stq $26,0($30)
        stq $15,8($30)
        mov $30,$15
        .prologue 0
        mov $15,$30
        ldq $26,0($30)
        ldq $15,8($30)
        lda $30,16($30)
        ret $31,($26),1
        .end dummy
#endif
