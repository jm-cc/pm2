#include <stdio.h>
#include <pm2.h>

int
pm2_main (int argc, char *argv[])
{
  pm2_init (&argc, argv);

  if (pm2_self () == 1)
    tprintf ("Hello World from node 1!\n");

  if (pm2_self () == 2)
    pm2_halt ();

  pm2_exit ();
  return 0;
}
