
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

#ifndef PM2_PROFILE_IS_DEF
#define PM2_PROFILE_IS_DEF

#define MARCEL_PROF_MASK  0x01
#define MAD_PROF_MASK     0x02
#define PM2_PROF_MASK     0x04
#define DSM_PROF_MASK     0x08
#define TBX_PROF_MASK     0x10
#define NTBX_PROF_MASK    0x20
#define USER_APP_MASK     0x40

#if defined(MARCEL_KERNEL)
#define PROFILE_KEYMASK MARCEL_PROF_MASK
#elif defined(MAD2_KERNEL) || defined(MAD1_KERNEL)
#define PROFILE_KEYMASK MAD_PROF_MASK
#elif defined(PM2_KERNEL)
#define PROFILE_KEYMASK PM2_PROF_MASK
#elif defined(DSM_KERNEL)
#define PROFILE_KEYMASK DSM_PROF_MASK
#elif defined(TBX_KERNEL)
#define PROFILE_KEYMASK TBX_PROF_MASK
#elif defined(NTBX_KERNEL)
#define PROFILE_KEYMASK NTBX_PROF_MASK
#else
#define PROFILE_KEYMASK USER_APP_MASK
#endif

#ifdef PROFILE

#if !defined(MODULE) && defined(COMMON_PROFILE)
// Application utilisateur
#define DO_PROFILE
#endif

#define CONFIG_FUT

#include "fut.h"

#ifdef DO_PROFILE

#define PROF_PROBE0(keymask, code)                        \
  do {                                                    \
    if((keymask) & fut_active)                            \
      fut_header(code);                                   \
  } while(0)

#define PROF_PROBE1(keymask, code, arg)                   \
  do {                                                    \
    if((keymask) & fut_active)                            \
      fut_header(code, arg);                              \
  } while(0)

#ifdef PREPROC

#define GEN_PREPROC(name)                                 \
  do {                                                    \
    extern int foo asm ("this_is_the_fut_" name "_code"); \
    foo = 1;                                              \
  } while(0)

#else // ifndef PREPROC

#if defined(MARCEL_SMP) || defined(MARCEL_ACTSMP)

#define GEN_PREPROC(name)                                        \
  do {                                                           \
    extern unsigned __code asm("fut_" name "_code");             \
    PROF_PROBE1(PROFILE_KEYMASK, __code, marcel_self()->number); \
  } while(0)

#else

#define GEN_PREPROC(name)                                 \
  do {                                                    \
    extern unsigned __code asm("fut_" name "_code");      \
    PROF_PROBE0(PROFILE_KEYMASK, __code);                 \
  } while(0)

#endif

#endif // PREPROC

#define PROF_IN()            GEN_PREPROC(__FUNCTION__ "_entry")
#define PROF_OUT()           GEN_PREPROC(__FUNCTION__ "_exit")

#define PROF_IN_EXT(name)    GEN_PREPROC(#name "_entry")
#define PROF_OUT_EXT(name)   GEN_PREPROC(#name "_exit")
#define PROF_EVENT(name)     GEN_PREPROC(#name "_single")

#else // ifndef DO_PROFILE

#define PROF_PROBE0(keymask, code)      (void)0
#define PROF_PROBE1(keymask, code, arg) (void)0

#define PROF_IN()                       (void)0
#define PROF_OUT()                      (void)0

#define PROF_IN_EXT(name)               (void)0
#define PROF_OUT_EXT(name)              (void)0
#define PROF_EVENT(name)                (void)0

#endif // DO_PROFILE

// In the following calls to fut_header, the keymask is ignored in
// PROF_SWITCH_TO() because this trace-instruction should _always_ be
// active...

// Must be called each time a context switch is performed between two
// user-level threads. The parameters are the values of the 'number'
// fields of the task_desc structure.
#define PROF_SWITCH_TO(thr1, thr2)                               \
 do {                                                            \
   if(__pm2_profile_active) {                                    \
      fut_header((((unsigned int)(FUT_SWITCH_TO_CODE))<<8) | 20, \
                 (unsigned int)(thr1),                           \
                 (unsigned int)(thr2));                          \
   }                                                             \
 } while(0)

// Must be called when a new kernel threads (SMP or activation flavors
// only) is about to execute the very first thread (usually the
// idle_task).

#ifdef USE_FKT

#define PROF_NEW_LWP(num, thr)                                      \
 do {                                                               \
   if (__pm2_profile_active) {                                      \
      fkt_new_lwp(thr, num);                                        \
   }                                                                \
 } while(0)

#else /* USE_FKT */

#define PROF_NEW_LWP(num, thr)                                      \
 do {                                                             \
 } while(0)

#endif /* USE_FKT */


#define PROF_THREAD_BIRTH(thr)                                      \
 do {                                                               \
   if(__pm2_profile_active) {                                       \
      fut_header((((unsigned int)(FUT_THREAD_BIRTH_CODE))<<8) | 16, \
                 (unsigned int)(thr));                              \
   }                                                                \
 } while(0)

#define PROF_THREAD_DEATH(thr)                                      \
 do {                                                               \
   if(__pm2_profile_active) {                                       \
      fut_header((((unsigned int)(FUT_THREAD_DEATH_CODE))<<8) | 16, \
                 (unsigned int)(thr));                              \
   }                                                                \
 } while(0)


void profile_init(void);

void profile_set_tracefile(char *fmt, ...);

void profile_activate(int how, unsigned user_keymask, unsigned kernel_keymask);

void profile_stop(void);

void profile_exit(void);

extern volatile unsigned __pm2_profile_active;

#else // ifndef PROFILE

#define PROF_PROBE0(keymask, code)      (void)0
#define PROF_PROBE1(keymask, code, arg) (void)0

#define PROF_SWITCH_TO(thr1, thr2)      (void)0
#define PROF_NEW_LWP(num, thr)     (void)0
#define PROF_THREAD_BIRTH(thr)          (void)0
#define PROF_THREAD_DEATH(thr)          (void)0

#define PROF_IN()                       (void)0
#define PROF_OUT()                      (void)0

#define PROF_IN_EXT(name)               (void)0
#define PROF_OUT_EXT(name)              (void)0
#define PROF_EVENT(name)                (void)0

#endif // PROFILE

#endif
