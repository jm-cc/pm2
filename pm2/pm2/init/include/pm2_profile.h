
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

#  define FUT_HEADER(code,...) fut_header(code, ##__VA_ARGS__)

#define PROF_PROBE(keymask, code, ...)                    \
  do {                                                    \
    if((keymask) & fut_active)                            \
      FUT_HEADER(code, ##__VA_ARGS__);                    \
  } while(0)

#define PROF_ALWAYS_PROBE(code, ...)                      \
  do {                                                    \
    if(__pm2_profile_active)                              \
      FUT_HEADER(code, ##__VA_ARGS__);                    \
  } while(0)

#define GEN_PREPROC(name) _GEN_PREPROC(name,__LINE__)
#define _GEN_PREPROC(name,line) __GEN_PREPROC(name,line)
#define GEN_PREPROC1(name,arg1) _GEN_PREPROC1(name,__LINE__,arg1)
#define _GEN_PREPROC1(name,line,arg1) __GEN_PREPROC1(name,line,arg1)
#define GEN_PREPROC2(name,arg1,arg2) _GEN_PREPROC2(name,__LINE__,arg1,arg2)
#define _GEN_PREPROC2(name,line,arg1,arg2) __GEN_PREPROC2(name,line,arg1,arg2)

#ifdef PREPROC

#define __GEN_PREPROC(name,line)                          \
  do {                                                    \
    extern int foo##line asm ("this_is_the_fut_" name "_code"); \
    foo##line = 1;                                              \
  } while(0)
#define __GEN_PREPROC1(name,line,arg1) __GEN_PREPROC(name,line)
#define __GEN_PREPROC2(name,line,arg1,arg2) __GEN_PREPROC(name,line)

#else // ifndef PREPROC

#define __GEN_PREPROC(name,line)                          \
  do {                                                    \
    extern unsigned __code##line asm("fut_" name "_code");\
    PROF_PROBE(PROFILE_KEYMASK, __code##line);            \
  } while(0)

#define __GEN_PREPROC1(name,line,arg1)                    \
  do {                                                    \
    extern unsigned __code##line asm("fut_" name "_code");\
    PROF_PROBE(PROFILE_KEYMASK, __code##line, arg1);      \
  } while(0)

#define __GEN_PREPROC2(name,line,arg1,arg2)               \
  do {                                                    \
    extern unsigned __code##line asm("fut_" name "_code");\
    PROF_PROBE(PROFILE_KEYMASK, __code##line, arg1,arg2); \
  } while(0)

#endif // PREPROC

#if 0
/* __FUNCTION__ can't be catenated with strings litterals, since it may be a
 * variable */
#define PROF_IN()            GEN_PREPROC(__FUNCTION__ "_entry")
#define PROF_OUT()           GEN_PREPROC(__FUNCTION__ "_exit")
#else
#define PROF_IN()					\
  do {							\
    extern void fun(void) asm(__FUNCTION__);		\
    /*__cyg_profile_func_enter(fun,NULL);	*/	\
    PROF_PROBE(PROFILE_KEYMASK, ((FUT_GCC_INSTRUMENT_ENTRY_CODE)<<8)|FUT_SIZE(1), fun);	\
  } while(0)
#define PROF_OUT()					\
  do {							\
    extern void fun(void) asm(__FUNCTION__);		\
    /*__cyg_profile_func_exit(fun,NULL);	*/	\
    PROF_PROBE(PROFILE_KEYMASK, ((FUT_GCC_INSTRUMENT_EXIT_CODE)<<8)|FUT_SIZE(1), fun);	\
  } while(0)
#endif

#define PROF_IN_EXT(name)             GEN_PREPROC(#name "_entry")
#define PROF_OUT_EXT(name)            GEN_PREPROC(#name "_exit")
#define PROF_EVENT(name)              GEN_PREPROC(#name "_single")
#define PROF_EVENT1(name, arg1)       GEN_PREPROC1(#name "_single1", arg1)
#define PROF_EVENT2(name, arg1, arg2) GEN_PREPROC2(#name "_single2", arg1, arg2)

#else // ifndef DO_PROFILE

#define PROF_PROBE(keymask, code, ...)        (void)0
#define PROF_ALWAYS_PROBE(code, ...)          (void)0

#define PROF_IN()                             (void)0
#define PROF_OUT()                            (void)0

#define PROF_IN_EXT(name)                     (void)0
#define PROF_OUT_EXT(name)                    (void)0
#define PROF_EVENT(name)                      (void)0
#define PROF_EVENT1(name, arg1)               (void)0
#define PROF_EVENT2(name, arg1, arg2)         (void)0

#endif // DO_PROFILE

// In the following calls to fut_header, the keymask is ignored in
// PROF_SWITCH_TO() because this trace-instruction should _always_ be
// active...

// Must be called each time a context switch is performed between two
// user-level threads. The parameters are the values of the 'number'
// fields of the task_desc structure.
#define PROF_SWITCH_TO(thr1, thr2)                               \
  PROF_ALWAYS_PROBE((((unsigned int)(FUT_SWITCH_TO_CODE))<<8) | FUT_SIZE(2), \
                 (unsigned int)(MA_PROFILE_TID(thr2)),           \
                 (unsigned int)thr2->number)

// Must be called when a new kernel threads (SMP or activation flavors
// only) is about to execute the very first thread (usually the
// idle_task).

#ifdef USE_FKT

extern int fkt_new_lwp(unsigned int thread_num, unsigned int lwp_logical_num);

#define PROF_NEW_LWP(num, thr)                                      \
 do {                                                               \
   if (__pm2_profile_active) {                                      \
      fut_header((((unsigned int)(FUT_NEW_LWP_CODE))<<8) | FUT_SIZE(2), \
                 (unsigned int)(LWP_SELF), num);                    \
      fkt_new_lwp(MA_PROFILE_TID(thr), num);                        \
   }                                                                \
 } while(0)

#else /* USE_FKT */

#define PROF_NEW_LWP(num, thr)                                      \
 do {                                                             \
 } while(0)

#endif /* USE_FKT */


#define PROF_THREAD_BIRTH(thr)                                      \
  PROF_ALWAYS_PROBE((((unsigned int)(FUT_THREAD_BIRTH_CODE))<<8) | FUT_SIZE(1), \
                 (unsigned int)(MA_PROFILE_TID(thr)))

#define PROF_THREAD_DEATH(thr)                                      \
  PROF_ALWAYS_PROBE((((unsigned int)(FUT_THREAD_DEATH_CODE))<<8) | FUT_SIZE(1), \
                 (unsigned int)(MA_PROFILE_TID(thr)))

#define PROF_SET_THREAD_NAME()                                      \
 do {                                                               \
   if(__pm2_profile_active) {                                       \
      unsigned int *__name = (unsigned int *) &MARCEL_SELF->name;   \
      fut_header((((unsigned int)(FUT_SET_THREAD_NAME_CODE))<<8) | FUT_SIZE(4), \
                 __name[0],                                         \
                 __name[1],                                         \
                 __name[2],                                         \
                 __name[3]);                                        \
   }                                                                \
 } while (0)


void profile_init(void);

void profile_set_tracefile(char *fmt, ...);

void profile_activate(int how, unsigned user_keymask, unsigned kernel_keymask);

void profile_stop(void);

void profile_exit(void);

extern volatile unsigned __pm2_profile_active;

#else // ifndef PROFILE

#define PROF_PROBE(keymask, code, ...)        (void)0
#define PROF_ALWAYS_PROBE(code, ...)          (void)0

#define PROF_SWITCH_TO(thr1, thr2)            (void)0
#define PROF_NEW_LWP(num, thr)                (void)0
#define PROF_THREAD_BIRTH(thr)                (void)0
#define PROF_THREAD_DEATH(thr)                (void)0
#define PROF_SET_THREAD_NAME()	              (void)0

#define PROF_IN()                             (void)0
#define PROF_OUT()                            (void)0

#define PROF_IN_EXT(name)                     (void)0
#define PROF_OUT_EXT(name)                    (void)0
#define PROF_EVENT(name)                      (void)0
#define PROF_EVENT1(name, arg1)               (void)0
#define PROF_EVENT2(name, arg1, arg2)         (void)0

#endif // PROFILE

#endif
