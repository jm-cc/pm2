
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

#define CONFIG_FUT

#include "fut.h"

#define PROF_PROBE0(keymask, code) do { \
                                     if(keymask & fut_active ) { \
                                        fut_header(code); \
                                     } \
                                   } while(0)

void profile_init(void);

void profile_activate(int how, unsigned keymask);

void profile_stop(void);

#endif
