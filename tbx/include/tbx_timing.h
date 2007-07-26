/*! \file tbx_timing.h
 *  \brief TBX timing data structures and macros
 *
 *  This file contains the support data structures and macros for the
 *  TBX timing facilities.
 * 
 */

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

/*
 * Tbx_timing.h
 * ============
 */

#ifndef TBX_TIMING_H
#define TBX_TIMING_H

#include <sys/types.h>
#include <sys/time.h>

/** \defgroup timing_interface timing interface
 *
 * This is the timing interface
 *
 * @{
 */

/* Hum hum... Here we suppose that X86ARCH => Pentium! */

#if defined(X86_ARCH) || defined(X86_64_ARCH)

typedef unsigned long long tbx_tick_t, *p_tbx_tick_t;

#ifdef X86_ARCH
#define TBX_GET_TICK(t) \
   __asm__ volatile("rdtsc" : "=A" (t))
#else
#define TBX_GET_TICK(t) do { \
     unsigned int __a,__d; \
     asm volatile("rdtsc" : "=a" (__a), "=d" (__d)); \
     (t) = ((unsigned long)__a) | (((unsigned long)__d)<<32); \
} while(0)
#endif
#define TBX_TICK_RAW_DIFF(t1, t2) \
   ((t2) - (t1))

#elif defined(ALPHA_ARCH)

typedef unsigned long tbx_tick_t, *p_tbx_tick_t;

#define TBX_GET_TICK(t) \
   __asm__ volatile("rpcc %0\n\t" : "=r"(t))
#define TBX_TICK_RAW_DIFF(t1, t2) \
   (((t2) & 0xFFFFFFFF) - ((t1) & 0xFFFFFFFF))

#elif defined(IA64_ARCH)

typedef unsigned long tbx_tick_t, *p_tbx_tick_t;

#define TBX_GET_TICK(t) \
   __asm__ volatile("mov %0=ar%1" : "=r" ((t)) : "i"(44))
#define TBX_TICK_RAW_DIFF(t1, t2) \
   ((t2) - (t1))

#elif defined(PPC_ARCH)

typedef unsigned long tbx_tick_t, *p_tbx_tick_t;

#define TBX_GET_TICK(t) __asm__ volatile("mftb %0" : "=r" (t))

#define TBX_TICK_RAW_DIFF(t1, t2) \
   ((t2) - (t1))

#elif defined(PPC64_ARCH)

typedef unsigned long tbx_tick_t, *p_tbx_tick_t;

#define TBX_GET_TICK(t) __asm__ volatile("mftb %0" : "=r" (t))

#define TBX_TICK_RAW_DIFF(t1, t2) \
   ((t2) - (t1))

#else

/* fall back to imprecise but portable way */
#define TBX_TIMING_GETTIMEOFDAY

typedef struct timeval tbx_tick_t, *p_tbx_tick_t;

#define TBX_GET_TICK(t) \
   gettimeofday(&(t), NULL)
#define TBX_TICK_RAW_DIFF(t1, t2) \
   ((t2.tv_sec * 1000000L + t2.tv_usec) - \
    (t1.tv_sec * 1000000L + t1.tv_usec))

#endif

#define TBX_TICK_DIFF(t1, t2) (TBX_TICK_RAW_DIFF(t1, t2) - tbx_residual)
#define TBX_TIMING_DELAY(t1, t2) tbx_tick2usec(TBX_TICK_DIFF(t1, t2))

extern unsigned long long tbx_residual;
extern tbx_tick_t         tbx_new_event;
extern tbx_tick_t         tbx_last_event;

char *tbx_tick2str(long long t);

/* @} */

#endif /* TBX_TIMING_H */
