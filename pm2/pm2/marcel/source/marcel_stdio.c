
/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 "the PM2 team" (pm2-dev@listes.ens-lyon.fr)
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

#include <stdarg.h>

static marcel_lock_t __io_lock = MARCEL_LOCK_INIT;

static __inline__ void io_lock()
{
  disable_preemption();
  marcel_lock_acquire(&__io_lock);
}

static __inline__ void io_unlock()
{
  marcel_lock_release(&__io_lock);
  enable_preemption();
}

int marcel_printf(char *format, ...)
{
  static va_list args;
  int retour;

  io_lock();

  va_start(args, format);
  retour = vprintf(format, args);
  va_end(args);

  io_unlock();

  return retour;
}

int marcel_fprintf(FILE *stream, char *format, ...)
{
  static va_list args;
  int retour;

  io_lock();

  va_start(args, format);
  retour = vfprintf(stream, format, args);
  va_end(args);

  io_unlock();
  return retour;
}

