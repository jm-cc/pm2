#include "leoparse.h"

void
usage(void)
{
  fprintf(stderr, "usage: pm2_load leoparse_test <conf_file>\n");
  exit(1);
}

int
main(int    argc,
     char **argv)
{
  p_tbx_htable_t result = NULL;

  tbx_init(argc, argv);
  leoparse_init(argc, argv);

  tbx_purge_cmd_line(&argc, argv);
  leoparse_purge_cmd_line(&argc, argv);

  argc--; argv++;
  if (!argc)
    usage();

  result = leoparse_parse_local_file(*argv);
  printf("Parsing succeeded\n");
  
  return 0;
}

