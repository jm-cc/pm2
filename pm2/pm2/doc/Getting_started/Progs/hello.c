#include <stdio.h>
#include <pm2.h>

int
pm2_main (int argc, char *argv[])
{
  pm2_init (&argc, argv);

  tprintf ("Hello World!\n");

  if (pm2_self () == 0)
    pm2_halt ();

  pm2_exit ();
  return 0;
}
