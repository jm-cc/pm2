
#include "marcel.h"
#include <stdio.h>
#include <unistd.h>

static int tube[2];

any_t f(any_t arg)
{
  char chaine[1024];

  marcel_detach(marcel_self());

  marcel_delay(100);

  marcel_printf("I'm waiting for a keyboard input\n");
  marcel_read(STDIN_FILENO, chaine, 1024);
  marcel_printf("Input from [keyboard] detected on LWP %d: %s",
		marcel_current_vp(), chaine);
   
  return NULL;
}

any_t g(any_t arg)
{
  unsigned i;

  marcel_detach(marcel_self());

  marcel_delay(100);

  marcel_printf("I'm waiting for a pipe input\n");
  marcel_read(tube[0], &i, sizeof(i));
  marcel_printf("Input from [pipe] detected on LWP %d: %x\n",
		marcel_current_vp(), i);

  return NULL;
}

int marcel_main(int argc, char *argv[])
{
  unsigned long i;
  unsigned magic = 0xdeadbeef;

  marcel_trace_on();

  marcel_init(&argc, argv);

  pipe(tube);

  marcel_create(NULL, NULL, f, NULL);
  marcel_create(NULL, NULL, g, NULL);

  mdebug("busy waiting...\n");

  i = 200000000L; while(--i);

  mdebug("marcel_delay...\n");

  marcel_delay(5000);

  write(tube[1], &magic, sizeof(magic));

  marcel_end();
  return 0;
}

