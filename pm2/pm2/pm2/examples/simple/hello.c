
#include "pm2.h"

int pm2_main(int argc, char **argv)
{
  pm2_init(&argc, argv);

  if(pm2_config_size() < 2) {
    fprintf(stderr,
	    "This program requires at least two processes.\n"
	    "Please rerun pm2conf.\n");
    exit(1);
  }

  printf("Hello world!\n");

  if(pm2_self() == pm2_config_size()-1)
    pm2_halt();

  pm2_exit();
  return 0;
}
