
/* once.c */

#include "marcel.h"

void f(void)
{
   tprintf("Salut les gars !\n");
}

int marcel_main(int argc, char *argv[])
{
  marcel_once_t once = marcel_once_init;

  marcel_init(&argc, argv);

  marcel_once(&once, f);
  marcel_once(&once, f);
  marcel_once(&once, f);
  marcel_once(&once, f);

  marcel_end();
  return 0;
}
