
#include <iostream.h>

#include "pm2.h"

int pm2_main(int argc, char *argv[])
{
  pm2_init(&argc, argv);

  cout << "Hello World!" << endl;

  if(pm2_self() == 0)
    pm2_halt();

  pm2_exit();

  return 0;
}
