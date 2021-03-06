
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

#ifndef FUT_PM2_IS_DEF
#define FUT_PM2_IS_DEF

#include "tbx_types.h"
#include "tbx_compiler.h"
#define CONFIG_FUT
#include "fxt/fxt.h"
#include "fxt/fut.h"
#include "fxt/fxt-tools.h"

#define MARCEL_PROF_MASK  0x01
#define MAD_PROF_MASK     0x02
#define PM2_PROF_MASK     0x04
#define TBX_PROF_MASK     0x10
#define NTBX_PROF_MASK    0x20
#define PIOMAN_PROF_MASK  0x40
#define USER_APP_MASK     0x80

#ifndef PROFILE_KEYMASK
#  define PROFILE_KEYMASK USER_APP_MASK
#endif

#define PROF_START                                                  \
 do {                                                               \
   if (__pm2_profile_active) {
#define PROF_END                                                    \
   }                                                                \
 } while(0)

#ifdef DO_PROFILE

#define PROF_PROBE(keymask, code, ...)                    \
  FUT_PROBE(keymask, code , ##__VA_ARGS__)

#define PROF_ALWAYS_PROBE(code, ...)                      \
 PROF_START                                               \
      FUT_DO_PROBE(code, ##__VA_ARGS__);                  \
 PROF_END

#define GEN_PREPROC(name,str) _GEN_PREPROC(name,str,__LINE__)
#define _GEN_PREPROC(name,str,line) __GEN_PREPROC(name,str,line)
#define GEN_PREPROC1(name,str,arg1) _GEN_PREPROC1(name,str,__LINE__,arg1)
#define _GEN_PREPROC1(name,str,line,arg1) __GEN_PREPROC1(name,str,line,arg1)
#define GEN_PREPROC2(name,str,arg1,arg2) _GEN_PREPROC2(name,str,__LINE__,arg1,arg2)
#define _GEN_PREPROC2(name,str,line,arg1,arg2) __GEN_PREPROC2(name,str,line,arg1,arg2)
#define GEN_PREPROC3(name,str,arg1,arg2,arg3) _GEN_PREPROC3(name,str,__LINE__,arg1,arg2,arg3)
#define _GEN_PREPROC3(name,str,line,arg1,arg2,arg3) __GEN_PREPROC3(name,str,line,arg1,arg2,arg3)
#define GEN_PREPROC4(name,str,arg1,arg2,arg3,arg4) _GEN_PREPROC4(name,str,__LINE__,arg1,arg2,arg3,arg4)
#define _GEN_PREPROC4(name,str,line,arg1,arg2,arg3,arg4) __GEN_PREPROC4(name,str,line,arg1,arg2,arg3,arg4)
#define GEN_PREPROC5(name,str,arg1,arg2,arg3,arg4,arg5) _GEN_PREPROC5(name,str,__LINE__,arg1,arg2,arg3,arg4,arg5)
#define _GEN_PREPROC5(name,str,line,arg1,arg2,arg3,arg4,arg5) __GEN_PREPROC5(name,str,line,arg1,arg2,arg3,arg4,arg5)
#define GEN_PREPROCSTR(name,str,s,...) _GEN_PREPROCSTR(name,str,__LINE__,s,##__VA_ARGS__)
#define _GEN_PREPROCSTR(name,str,line,s,...) __GEN_PREPROCSTR(name,str,line,s,##__VA_ARGS__)
#define GEN_PREPROC_ALWAYS(name,str) _GEN_PREPROC_ALWAYS(name,str,__LINE__)
#define _GEN_PREPROC_ALWAYS(name,str,line) __GEN_PREPROC_ALWAYS(name,str,line)
#define GEN_PREPROC1_ALWAYS(name,str,arg1) _GEN_PREPROC1_ALWAYS(name,str,__LINE__,arg1)
#define _GEN_PREPROC1_ALWAYS(name,str,line,arg1) __GEN_PREPROC1_ALWAYS(name,str,line,arg1)
#define GEN_PREPROC2_ALWAYS(name,str,arg1,arg2) _GEN_PREPROC2_ALWAYS(name,str,__LINE__,arg1,arg2)
#define _GEN_PREPROC2_ALWAYS(name,str,line,arg1,arg2) __GEN_PREPROC2_ALWAYS(name,str,line,arg1,arg2)
#define GEN_PREPROC3_ALWAYS(name,str,arg1,arg2,arg3) _GEN_PREPROC3_ALWAYS(name,str,__LINE__,arg1,arg2,arg3)
#define _GEN_PREPROC3_ALWAYS(name,str,line,arg1,arg2,arg3) __GEN_PREPROC3_ALWAYS(name,str,line,arg1,arg2,arg3)
#define GEN_PREPROC4_ALWAYS(name,str,arg1,arg2,arg3,arg4) _GEN_PREPROC4_ALWAYS(name,str,__LINE__,arg1,arg2,arg3,arg4)
#define _GEN_PREPROC4_ALWAYS(name,str,line,arg1,arg2,arg3,arg4) __GEN_PREPROC4_ALWAYS(name,str,line,arg1,arg2,arg3,arg4)
#define GEN_PREPROC5_ALWAYS(name,str,arg1,arg2,arg3,arg4,arg5) _GEN_PREPROC5_ALWAYS(name,str,__LINE__,arg1,arg2,arg3,arg4,arg5)
#define _GEN_PREPROC5_ALWAYS(name,str,line,arg1,arg2,arg3,arg4,arg5) __GEN_PREPROC5_ALWAYS(name,str,line,arg1,arg2,arg3,arg4,arg5)
#define GEN_PREPROCSTR_ALWAYS(name,str,s,...) _GEN_PREPROCSTR_ALWAYS(name,str,__LINE__,s,##__VA_ARGS__)
#define _GEN_PREPROCSTR_ALWAYS(name,str,line,s,...) __GEN_PREPROCSTR_ALWAYS(name,str,line,s,##__VA_ARGS__)

#ifdef PREPROC

#define __GEN_PREPROC(name,str,line)                          \
  do {                                                    \
  TBX_VISIBILITY_PUSH_DEFAULT				\
    extern int foo##name##line asm ("this_is_the_fut_" str "_code"); \
  TBX_VISIBILITY_POP					\
    foo##name##line = 1;                                              \
  } while(0)
#define __GEN_PREPROC1(name,str,line,arg1) __GEN_PREPROC(name,str,line)
#define __GEN_PREPROC2(name,str,line,arg1,arg2) __GEN_PREPROC(name,str,line)
#define __GEN_PREPROC3(name,str,line,arg1,arg2,arg3) __GEN_PREPROC(name,str,line)
#define __GEN_PREPROC4(name,str,line,arg1,arg2,arg3,arg4) __GEN_PREPROC(name,str,line)
#define __GEN_PREPROC5(name,str,line,arg1,arg2,arg3,arg4,arg5) __GEN_PREPROC(name,str,line)
#define __GEN_PREPROCSTR(name,str,line,s,...) __GEN_PREPROC(name,str,line)
#define __GEN_PREPROC_ALWAYS(name,str,line) __GEN_PREPROC(name,str,line)
#define __GEN_PREPROC1_ALWAYS(name,str,line,arg1) __GEN_PREPROC(name,str,line)
#define __GEN_PREPROC2_ALWAYS(name,str,line,arg1,arg2) __GEN_PREPROC(name,str,line)
#define __GEN_PREPROC3_ALWAYS(name,str,line,arg1,arg2,arg3) __GEN_PREPROC(name,str,line)
#define __GEN_PREPROC4_ALWAYS(name,str,line,arg1,arg2,arg3,arg4) __GEN_PREPROC(name,str,line)
#define __GEN_PREPROC5_ALWAYS(name,str,line,arg1,arg2,arg3,arg4,arg5) __GEN_PREPROC(name,str,line)
#define __GEN_PREPROCSTR_ALWAYS(name,str,line,s,...) __GEN_PREPROC(name,str,line)

#else // ifndef PREPROC

#if defined(DARWIN_SYS) || defined(WIN_SYS)
#define FUT_SYM_PREFIX "_"
#else
#define FUT_SYM_PREFIX ""
#endif

#define __GEN_PREPROC(name,str,line)                          \
  do {                                                    \
  TBX_VISIBILITY_PUSH_DEFAULT				\
    extern unsigned __code_##name##_##line asm(FUT_SYM_PREFIX "fut_" str "_code");\
  TBX_VISIBILITY_POP					\
    FUT_PROBE0(PROFILE_KEYMASK, __code_##name##_##line);            \
  } while(0)

#define __GEN_PREPROC1(name,str,line,arg1)                    \
  do {                                                    \
  TBX_VISIBILITY_PUSH_DEFAULT				\
    extern unsigned __code_##name##_##line asm(FUT_SYM_PREFIX "fut_" str "_code");\
  TBX_VISIBILITY_POP					\
    FUT_PROBE1(PROFILE_KEYMASK, __code_##name##_##line, arg1);      \
  } while(0)

#define __GEN_PREPROC2(name,str,line,arg1,arg2)               \
  do {                                                    \
  TBX_VISIBILITY_PUSH_DEFAULT				\
    extern unsigned __code_##name##_##line asm(FUT_SYM_PREFIX "fut_" str "_code");\
  TBX_VISIBILITY_POP					\
    FUT_PROBE2(PROFILE_KEYMASK, __code_##name##_##line, arg1,arg2); \
  } while(0)

#define __GEN_PREPROC3(name,str,line,arg1,arg2,arg3)	  \
  do {                                                    \
  TBX_VISIBILITY_PUSH_DEFAULT				\
    extern unsigned __code_##name##_##line asm(FUT_SYM_PREFIX "fut_" str "_code");\
  TBX_VISIBILITY_POP					\
    FUT_PROBE3(PROFILE_KEYMASK, __code_##name##_##line, arg1,arg2,arg3);	\
  } while(0)

#define __GEN_PREPROC4(name,str,line,arg1,arg2,arg3,arg4)	  \
  do {                                                    \
  TBX_VISIBILITY_PUSH_DEFAULT				\
    extern unsigned __code_##name##_##line asm(FUT_SYM_PREFIX "fut_" str "_code");\
  TBX_VISIBILITY_POP					\
    FUT_PROBE4(PROFILE_KEYMASK, __code_##name##_##line, arg1,arg2,arg3,arg4); \
  } while(0)

#define __GEN_PREPROC5(name,str,line,arg1,arg2,arg3,arg4,arg5)	\
  do {                                                    \
  TBX_VISIBILITY_PUSH_DEFAULT				\
    extern unsigned __code_##name##_##line asm(FUT_SYM_PREFIX "fut_" str "_code");\
  TBX_VISIBILITY_POP					\
    FUT_PROBE5(PROFILE_KEYMASK, __code_##name##_##line, arg1,arg2,arg3,arg4,arg5); \
  } while(0)

#define __GEN_PREPROCSTR(name,str,line,s,...)                     \
  do {                                                    \
  TBX_VISIBILITY_PUSH_DEFAULT				\
    extern unsigned __code_##name##_##line asm(FUT_SYM_PREFIX "fut_" str "_code");\
  TBX_VISIBILITY_POP					\
    char __s[FXT_MAX_DATA];                                   \
    snprintf(__s,sizeof(__s)-1,s,##__VA_ARGS__);              \
    __s[sizeof(__s)-1] = 0;                                   \
    FUT_PROBESTR(PROFILE_KEYMASK, __code_##name##_##line, __s);       \
  } while(0)

#define __GEN_PREPROC_ALWAYS(name,str,line)                   \
  do {                                                    \
  TBX_VISIBILITY_PUSH_DEFAULT				\
    extern unsigned __code_##name##_##line asm(FUT_SYM_PREFIX "fut_" str "_code");\
  TBX_VISIBILITY_POP					\
    FUT_DO_PROBE0(__code_##name##_##line);                          \
  } while(0)

#define __GEN_PREPROC1_ALWAYS(name,str,line,arg1)             \
  do {                                                    \
  TBX_VISIBILITY_PUSH_DEFAULT				\
    extern unsigned __code_##name##_##line asm(FUT_SYM_PREFIX "fut_" str "_code");\
  TBX_VISIBILITY_POP					\
    FUT_DO_PROBE1(__code_##name##_##line, arg1);                    \
  } while(0)

#define __GEN_PREPROC2_ALWAYS(name,str,line,arg1,arg2)        \
  do {                                                    \
  TBX_VISIBILITY_PUSH_DEFAULT				\
    extern unsigned __code_##name##_##line asm(FUT_SYM_PREFIX "fut_" str "_code");\
  TBX_VISIBILITY_POP					\
    FUT_DO_PROBE2(__code_##name##_##line, arg1,arg2);               \
  } while(0)

#define __GEN_PREPROC3_ALWAYS(name,str,line,arg1,arg2,arg3)	  \
  do {                                                    \
  TBX_VISIBILITY_PUSH_DEFAULT				\
    extern unsigned __code_##name##_##line asm(FUT_SYM_PREFIX "fut_" str "_code");\
  TBX_VISIBILITY_POP					\
    FUT_DO_PROBE3(__code_##name##_##line, arg1,arg2,arg3);	\
  } while(0)

#define __GEN_PREPROC4_ALWAYS(name,str,line,arg1,arg2,arg3,arg4)	\
  do {                                                    \
  TBX_VISIBILITY_PUSH_DEFAULT				\
    extern unsigned __code_##name##_##line asm(FUT_SYM_PREFIX "fut_" str "_code");\
  TBX_VISIBILITY_POP					\
    FUT_DO_PROBE4(__code_##name##_##line, arg1,arg2,arg3,arg4);	\
  } while(0)

#define __GEN_PREPROC5_ALWAYS(name,str,line,arg1,arg2,arg3,arg4,arg5)	\
  do {                                                    \
  TBX_VISIBILITY_PUSH_DEFAULT				\
    extern unsigned __code_##name##_##line asm(FUT_SYM_PREFIX "fut_" str "_code");\
  TBX_VISIBILITY_POP					\
    FUT_DO_PROBE5(__code_##name##_##line, arg1,arg2,arg3,arg4,arg5);	\
  } while(0)

#define __GEN_PREPROCSTR_ALWAYS(name,str,line,s,...)      \
  do {                                                    \
  TBX_VISIBILITY_PUSH_DEFAULT				\
    extern unsigned __code_##name##_##line asm(FUT_SYM_PREFIX "fut_" str "_code");\
  TBX_VISIBILITY_POP					\
    char __s[FXT_MAX_DATA];                                   \
    snprintf(__s,sizeof(__s)-1,s,##__VA_ARGS__);              \
    __s[sizeof(__s)-1] = 0;                                   \
    FUT_DO_PROBESTR(__code_##name##_##line, __s);                     \
  } while(0)

#endif // PREPROC

#ifdef __GNUC__
#  if __GNUC__ >= 4
#    define PROF_IN()		__builtin_profile_func_enter()
#    define PROF_OUT()		__builtin_profile_func_exit()
#  elif (__GNUC__ == 3 && __GNUC_MINOR__ < 4) || (__GNUC__ <= 2)
/* Starting from gcc 3.4, __FUNCTION__ really can't be catenated with string litterals */
#    define PROF_IN()		GEN_PREPROC(function, __FUNCTION__ "_entry")
#    define PROF_OUT()		GEN_PREPROC(function, __FUNCTION__ "_exit")
#  endif
#endif

#ifndef PROF_IN
#warning "No way to set profiling probes with your compiler. Please use gcc <= 3.3 or gcc >= 4.0"
#define PROF_IN() (void) 0
#define PROF_OUT() (void) 0
#endif

#define PROF_IN_EXT(name)             GEN_PREPROC(name, #name "_entry")
#define PROF_OUT_EXT(name)            GEN_PREPROC(name, #name "_exit")
#define PROF_EVENT(name)              GEN_PREPROC(name, #name "_single")
#define PROF_EVENT1(name, arg1)       GEN_PREPROC1(name, #name "_single", arg1)
#define PROF_EVENT2(name, arg1, arg2) GEN_PREPROC2(name, #name "_single", arg1, arg2)
#define PROF_EVENT3(name, arg1, arg2, arg3) GEN_PREPROC3(name, #name "_single", arg1, arg2, arg3)
#define PROF_EVENT4(name, arg1, arg2, arg3, arg4) GEN_PREPROC4(name, #name "_single", arg1, arg2, arg3, arg4)
#define PROF_EVENT5(name, arg1, arg2, arg3, arg4, arg5) GEN_PREPROC5(name, #name "_single", arg1, arg2, arg3, arg4, arg5)
#define PROF_EVENTSTR(name, s, ...)   GEN_PREPROCSTR(name, #name "_single", s, ##__VA_ARGS__)
#define PROF_EVENT_ALWAYS(name)              GEN_PREPROC_ALWAYS(name, #name "_single")
#define PROF_EVENT1_ALWAYS(name, arg1)       GEN_PREPROC1_ALWAYS(name, #name "_single", arg1)
#define PROF_EVENT2_ALWAYS(name, arg1, arg2) GEN_PREPROC2_ALWAYS(name, #name "_single", arg1, arg2)
#define PROF_EVENT3_ALWAYS(name, arg1, arg2, arg3) GEN_PREPROC3_ALWAYS(name, #name "_single", arg1, arg2, arg3)
#define PROF_EVENT4_ALWAYS(name, arg1, arg2, arg3, arg4) GEN_PREPROC4_ALWAYS(name, #name "_single", arg1, arg2, arg3, arg4)
#define PROF_EVENT5_ALWAYS(name, arg1, arg2, arg3, arg4, arg5) GEN_PREPROC5_ALWAYS(name, #name "_single", arg1, arg2, arg3, arg4, arg5)
#define PROF_EVENTSTR_ALWAYS(name, s, ...)   GEN_PREPROCSTR_ALWAYS(name, #name "_single", s, ##__VA_ARGS__)

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
#define PROF_EVENT3(name, arg1, arg2, arg3)                     (void)0
#define PROF_EVENT4(name, arg1, arg2, arg3, arg4)               (void)0
#define PROF_EVENT5(name, arg1, arg2, arg3, arg4, arg5)         (void)0
#define PROF_EVENTSTR(name, s, ...)           (void)0
#define PROF_EVENT_ALWAYS(name)               (void)0
#define PROF_EVENT1_ALWAYS(name, arg1)        (void)0
#define PROF_EVENT2_ALWAYS(name, arg1, arg2)  (void)0
#define PROF_EVENT3_ALWAYS(name, arg1, arg2, arg3)              (void)0
#define PROF_EVENT4_ALWAYS(name, arg1, arg2, arg3, arg4)        (void)0
#define PROF_EVENT5_ALWAYS(name, arg1, arg2, arg3, arg4, arg5)  (void)0
#define PROF_EVENTSTR_ALWAYS(name, s, ...)    (void)0

#endif // DO_PROFILE

// In the following calls to fut_header, the keymask is ignored in
// PROF_SWITCH_TO() because this trace-instruction should _always_ be
// active...

// Must be called each time a context switch is performed between two
// user-level threads. The parameters are the values of the 'number'
// fields of the task_desc structure.
#define PROF_SWITCH_TO(thr1, thr2)                                  \
 PROF_START                                                         \
 FUT_DO_PROBE3(FUT_SWITCH_TO_CODE, MA_PROFILE_TID(thr2), thr2->number, marcel_current_vp() == -1 ? -1 : marcel_topo_vp_level[marcel_current_vp()].os_cpu); \
 PROF_END

// Must be called when a new kernel threads (SMP or activation flavors
// only) is about to execute the very first thread (usually the
// idle_task).

#define PROF_NEW_LWP(num, thr)                                      \
 PROF_START                                                         \
      FUT_DO_PROBE2(FUT_NEW_LWP_CODE, num, thr);                    \
 PROF_END


#define PROF_THREAD_BIRTH(thr)                                      \
 PROF_START                                                         \
 FUT_DO_PROBE2(FUT_THREAD_BIRTH_CODE, MA_PROFILE_TID(thr), marcel_current_vp() == -1 ? -1 : marcel_topo_vp_level[marcel_current_vp()].os_cpu);        \
 PROF_END

#define PROF_THREAD_DEATH(thr)                                      \
 PROF_START                                                         \
 FUT_DO_PROBE2(FUT_THREAD_DEATH_CODE, MA_PROFILE_TID(thr), marcel_current_vp() == -1 ? -1 : marcel_topo_vp_level[marcel_current_vp()].os_cpu);        \
 PROF_END

#define PROF_SET_THREAD_NAME(thr)                                   \
 PROF_START                                                         \
      unsigned long *__name = (unsigned long *) &(thr->as_entity).name;         \
      FUT_DO_PROBE5(FUT_SET_THREAD_NAME_CODE,                       \
		 thr,                                               \
                 __name[0],                                         \
                 __name[1],                                         \
                 __name[2],                                         \
                 __name[3]);                                        \
 PROF_END


TBX_VISIBILITY_PUSH_DEFAULT
extern volatile tbx_bool_t __pm2_profile_active;
extern fxt_t fut;

void profile_init(void);
void profile_set_tracefile(char *fmt, ...);
void profile_activate(int how, unsigned user_keymask, unsigned kernel_keymask);
void profile_stop(void);
void profile_exit(void);
TBX_VISIBILITY_POP

#endif
