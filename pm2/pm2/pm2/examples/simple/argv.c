
#include "pm2.h"

int pm2_main(int argc, char **argv)
{
  int i;

  pm2_init(&argc, argv);

  printf("argc = %d\n", argc);
  for(i=0; i<argc; i++)
    printf("argv[%d] = <%s>\n", i, argv[i]);

  if(pm2_self() == 0)
    pm2_halt();

  pm2_exit();
  return 0;
}
