
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

