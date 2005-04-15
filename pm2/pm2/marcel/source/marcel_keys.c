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

#include "marcel.h"

DEF_MARCEL_POSIX(int, setspecific, (marcel_key_t key,
				    __const void* value), (key, value))
{
#ifdef MA__DEBUG
   if((key < 0) || (key >= marcel_nb_keys))
      RAISE(CONSTRAINT_ERROR);
#endif
   marcel_self()->key[key] = (any_t)value;
   return 0;
}
DEF_PTHREAD(int, setspecific, (pthread_key_t key,
				    __const void* value), (key, value))
DEF___PTHREAD(int, setspecific, (pthread_key_t key,
				    __const void* value), (key, value))

DEF_MARCEL_POSIX(any_t, getspecific, (marcel_key_t key), (key))
{
#ifdef MA__DEBUG
   if((key < 0) || (key>=MAX_KEY_SPECIFIC) || (!marcel_key_present[key]))
      RAISE(CONSTRAINT_ERROR);
#endif
   return marcel_self()->key[key];
}
DEF_PTHREAD(any_t, getspecific, (pthread_key_t key), (key))
DEF___PTHREAD(any_t, getspecific, (pthread_key_t key), (key))
