/* Copyright (C) 2000 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
     Contributed by David Mosberger-Tang <davidm@hpl.hp.com>

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

//#include <sysdeps/generic/sysdep.h>
#define C_LABEL(name)           name##:
#define C_SYMBOL_NAME(name)     name


#ifdef __ASSEMBLER__

/* Macros to help writing .prologue directives in assembly code.  */
#define ASM_UNW_PRLG_RP			0x8
#define ASM_UNW_PRLG_PFS		0x4
#define ASM_UNW_PRLG_PSP		0x2
#define ASM_UNW_PRLG_PR			0x1
#define ASM_UNW_PRLG_GRSAVE(ninputs)	(32+(ninputs))

#define ENTRY(name)				\
	.text;					\
	.align 32;				\
	.proc C_SYMBOL_NAME(name);		\
	.global C_SYMBOL_NAME(name);		\
	C_LABEL(name)				\
	CALL_MCOUNT

#define LOCAL_ENTRY(name)			\
	.text;					\
	.align 32;				\
	.proc C_SYMBOL_NAME(name);		\
	C_LABEL(name)				\
	CALL_MCOUNT

#define LEAF(name)				\
  .text;					\
  .align 32;					\
  .proc C_SYMBOL_NAME(name);			\
  .global name;					\
  C_LABEL(name)

#define LOCAL_LEAF(name)			\
  .text;					\
  .align 32;					\
  .proc C_SYMBOL_NAME(name);			\
  C_LABEL(name)

/* Mark the end of function SYM.  */
#undef END
#define END(sym)	.endp C_SYMBOL_NAME(sym)

#undef CALL_MCOUNT
#ifdef PROF
# define CALL_MCOUNT                                                    \
        .data;                                                          \
1:      data8 0;        /* XXX fixme: use .xdata8 once labels work */   \
        .previous;                                                      \
        .prologue;                                                      \
        .save ar.pfs, r40;                                              \
        alloc out0 = ar.pfs, 8, 0, 4, 0;                                \
        mov out1 = gp;                                                  \
        .save rp, out2;                                                 \
        mov out2 = rp;                                                  \
        .body;                                                          \
        ;;                                                              \
        addl out3 = @ltoff(1b), gp;                                     \
        br.call.sptk.many rp = _mcount                                  \
        ;;
#else
# define CALL_MCOUNT    /* Do nothing. */
#endif

#undef END
#define END(name)                                               \
        .size   C_SYMBOL_NAME(name), . - C_SYMBOL_NAME(name) ;  \
        .endp   C_SYMBOL_NAME(name)

#define ret                     br.ret.sptk.few b0


#endif /* ASSEMBLER */
