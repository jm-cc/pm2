/*	names.h	*/
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

#define GCC_TRACED_FUNCTION_MINI 0x08000000UL
#define GCC_TRACED_FUNCTION_EXIT 0x80000000UL

#define	NTRAPS	20
extern char	*traps[NTRAPS+1];
#define NSYS_IRQS	17
extern char	*sys_irqs[NSYS_IRQS];
#define NSYS_CALLS		274
extern char	*sys_calls[NSYS_CALLS+1];

extern unsigned int nirqs;

extern void record_irqs( int fd );
extern void load_irqs( int fd );
extern char *find_irq( unsigned int code );
extern int find_syscall( int code, char *category, char **name, int *should_be_hex );
extern struct code_name fkt_code_table[];
#ifdef FUT
extern struct code_name fut_code_table[];
#endif
extern char *find_name( unsigned int code, int keep_entry, int maxlen, struct code_name *code_table );
