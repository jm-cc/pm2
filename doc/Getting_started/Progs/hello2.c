#include <pm2.h>

int pm2_main(int argc, char *argv[]) {
  pm2_init(argc, argv);

  if (pm2_self() == 1) {
    pm2_printf("Hello World !\n");	/* Here! */
    pm2_halt();
  }

  pm2_exit();
  return 0;
}
