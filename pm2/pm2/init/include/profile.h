/*
                      PM2 HIGH-PERF/ISOMALLOC
           High Performance Parallel Multithreaded Machine
                           version 3.0

     Gabriel Antoniu, Olivier Aumage, Luc Bouge, Vincent Danjean,
       Christian Perez, Jean-Francois Mehaut, Raymond Namyst

            Laboratoire de l'Informatique du Parallelisme
                        UMR 5668 CNRS-INRIA
                 Ecole Normale Superieure de Lyon

                      External Contributors:
                 Yves Denneulin (LMC - Grenoble),
                 Benoit Planquelle (LIFL - Lille)

                    1998 All Rights Reserved


                             NOTICE

 Permission to use, copy, modify, and distribute this software and
 its documentation for any purpose and without fee is hereby granted
 provided that the above copyright notice appear in all copies and
 that both the copyright notice and this permission notice appear in
 supporting documentation.

 Neither the institutions (Ecole Normale Superieure de Lyon,
 Laboratoire de L'informatique du Parallelisme, Universite des
 Sciences et Technologies de Lille, Laboratoire d'Informatique
 Fondamentale de Lille), nor the Authors make any representations
 about the suitability of this software for any purpose. This
 software is provided ``as is'' without express or implied warranty.

______________________________________________________________________________
$Log: profile.h,v $
Revision 1.3  2000/09/15 17:37:13  rnamyst
Tracefiles are now generated correctly with PM2 applications using multiple processes

Revision 1.2  2000/09/15 02:04:51  rnamyst
fut_print is now built by the makefile, rather than by pm2-build-fut-entries

Revision 1.1  2000/09/14 02:08:27  rnamyst
Put profile.h into common/include and added few FUT_SWITCH_TO calls

______________________________________________________________________________
*/

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

#else // ifndef DO_PROFILE

#define PROF_PROBE0(keymask, code)   (void)0

#endif

// The keymask of PROF_SWITCH_TO() is -1 because this
// trace-instruction should be activated whenever one other
// trace-instruction is active...
#define PROF_SWITCH_TO(thr) ((fut_active) ? \
                fut_header((((unsigned int)(FUT_SWITCH_TO_CODE))<<8) | 16, \
                           (unsigned int)((thr)->number)) : 0)

void profile_init(void);

void profile_set_tracefile(char *fmt, ...);

void profile_activate(int how, unsigned keymask);

void profile_stop(void);

#else // ifndef PROFILE

#define PROF_PROBE0(keymask, code)   (void)0
#define PROF_SWITCH_TO(thr)          (void)0

#endif

#endif
