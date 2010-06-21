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

#ifndef __MARCEL_PROFILE__
#define __MARCEL_PROFILE__


#ifdef PROFILE

#include "fut_pm2.h"

#else

#  define PROF_PROBE(keymask, code, ...)        (void)0
#  define PROF_ALWAYS_PROBE(code, ...)          (void)0

#  define PROF_IN()                             (void)0
#  define PROF_OUT()                            (void)0

#  define PROF_IN_EXT(name)                     (void)0
#  define PROF_OUT_EXT(name)                    (void)0
#  define PROF_EVENT(name)                      (void)0
#  define PROF_EVENT1(name, arg1)               (void)0
#  define PROF_EVENT2(name, arg1, arg2)         (void)0
#  define PROF_EVENTSTR(name, s, ...)           (void)0
#  define PROF_EVENT_ALWAYS(name)               (void)0
#  define PROF_EVENT1_ALWAYS(name, arg1)        (void)0
#  define PROF_EVENT2_ALWAYS(name, arg1, arg2)  (void)0
#  define PROF_EVENTSTR_ALWAYS(name, s)         (void)0

#  define PROF_SWITCH_TO(thr1, thr2)            (void)0
#  define PROF_NEW_LWP(num, thr)                (void)0
#  define PROF_THREAD_BIRTH(thr)                (void)0
#  define PROF_THREAD_DEATH(thr)                (void)0
#  define PROF_SET_THREAD_NAME(thr)             (void)0

#endif /** PROFILE **/


#endif /** __MARCEL_PROFILE_H__ **/
