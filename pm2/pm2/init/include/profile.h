
#ifndef PROFILE_IS_DEF
#define PROFILE_IS_DEF

#define MARCEL_PROF_MASK  0x01
#define MAD_PROF_MASK     0x02
#define PM2_PROF_MASK     0x04
#define DSM_PROF_MASK     0x08
#define TBX_PROF_MASK     0x10
#define NTBX_PROF_MASK    0x20

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
#endif

#ifdef PROFILE

#define CONFIG_FUT

#include "fut.h"

#ifdef DO_PROFILE

#define PROF_PROBE0(keymask, code)   (((keymask) & fut_active) ? fut_header(code) : 0)

#ifdef PREPROC

#define GEN_PREPROC(name)   { extern int foo asm ("this_is_the_fut_" \
                                                  name "_code"); \
                               foo=1; }

#else // ifndef PREPROC

#define GEN_PREPROC(name)   { extern unsigned __code asm("fut_" name "_code"); \
                               PROF_PROBE0(PROFILE_KEYMASK, __code); }
#endif // PREPROC

#define PROF_IN()            GEN_PREPROC(__FUNCTION__ "_entry")
#define PROF_OUT()           GEN_PREPROC(__FUNCTION__ "_exit")

#define PROF_IN_EXT(name)    GEN_PREPROC(#name "_entry")
#define PROF_OUT_EXT(name)   GEN_PREPROC(#name "_exit");

#else // ifndef DO_PROFILE

#define PROF_PROBE0(keymask, code)   (void)0

#define PROF_IN()                    (void)0
#define PROF_OUT()                   (void)0

#define PROF_IN_EXT(name)            (void)0
#define PROF_OUT_EXT(name)           (void)0

#endif // DO_PROFILE

// The keymask of PROF_SWITCH_TO() is -1 because this
// trace-instruction should be activated whenever one other
// trace-instruction is active...
#define PROF_SWITCH_TO(thr) ((fut_active && thr != marcel_self()) ? \
                fut_header((((unsigned int)(FUT_SWITCH_TO_CODE))<<8) | 16, \
                           (unsigned int)((thr)->number)) : 0)

void profile_init(void);

void profile_set_tracefile(char *fmt, ...);

void profile_activate(int how, unsigned keymask);

void profile_stop(void);

#else // ifndef PROFILE

#define PROF_PROBE0(keymask, code)   (void)0
#define PROF_SWITCH_TO(thr)          (void)0

#define PROF_IN()                    (void)0
#define PROF_OUT()                   (void)0

#define PROF_IN_EXT(name)            (void)0
#define PROF_OUT_EXT(name)           (void)0

#endif // PROFILE

#endif
