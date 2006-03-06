#include "pm2_common.h" /* Here! */

int pm2_main (int argc, char *argv[]) {
  common_pre_init(&argc, argv, NULL); /* Here! */
  common_post_init(&argc, argv, NULL);/* Here! */

  tprintf ("Hello World!\n");

  /* Start */ 
  if (pm2_self () == 0)
    pm2_halt ();
  /* End */

  common_exit(NULL);/* Here! */
  exit(EXIT_SUCCESS);
}
