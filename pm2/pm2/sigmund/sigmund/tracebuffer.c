#include <stdio.h>
#include "sigmund.h"
#include "parser.h"
#include <stdlib.h>
#include "time.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "tracebuffer.h"
#include "assert.h"

#include "fkt.h"
#include "fut_code.h"


#define NB_MAX_PROC  16

struct code_name code_table[1000];
int nb_code;

static FILE *f_str;

static void code_copy(int code, char *name)
{
  code_table[nb_code].code = code;
  code_table[nb_code].name = name;
  nb_code++;
}


void init()
{ 
  int fd; 
  nb_code = 0;
  code_copy(FUT_SETUP_CODE, "fut_setup");
  code_copy(FUT_KEYCHANGE_CODE, "fut_keychange");
  code_copy(FUT_RESET_CODE, "fut_reset");
  code_copy(FUT_CALIBRATE0_CODE, "fut_calibrate0");
  code_copy(FUT_CALIBRATE1_CODE, "fut_calibrate1");
  code_copy(FUT_CALIBRATE2_CODE, "fut_calibrate2");
  code_copy(FUT_SWITCH_TO_CODE, "fut_switch_to");
  code_copy(FUT_MAIN_ENTRY_CODE, "main_entry");
  code_copy(FUT_MAIN_EXIT_CODE, "main_exit");
  fd = open("../../../build/marcel-prof/profile/include/fut_print.h",O_RDONLY);
  if (fd == -1) {
    fprintf(stderr, "../../../build/marcel-prof/profile/include/fut_print.h could not be opened\n");
    exit(1);
  }
  parser_start(fd);
  parser_run();
  parser_stop();
  close(fd);
  code_copy(0, NULL);
}

void read_str_header()
{

}

// Returns 0 if OK 1 if EOF
int get_next_trace(trace *tr)
{
  int i;
  if (fread(&(tr->clock), sizeof(u_64), 1, f_str) == 0)
    return 1;
  if (fread(&(tr->thread), sizeof(int), 1, f_str) == 0) {
    fprintf(stderr, "Corrupted trace file\n");
    exit(1);
  }
  if (fread(&(tr->pid), sizeof(int), 1, f_str) == 0) {
    fprintf(stderr, "Corrupted trace file\n");
    exit(1);
  }
  if (fread(&(tr->proc), sizeof(short int), 1, f_str) == 0) {
    fprintf(stderr, "Corrupted trace file\n");
    exit(1);
  }
  if (fread(&(tr->type), sizeof(mode), 1, f_str) == 0) {
    fprintf(stderr, "Corrupted trace file\n");
    exit(1);
  }
  if (fread(&(tr->number), sizeof(unsigned int), 1, f_str) == 0) {
    fprintf(stderr, "Corrupted trace file\n");
    exit(1);
  }
  if (fread(&(tr->code), sizeof(int), 1, f_str) == 0) {
    fprintf(stderr, "Corrupted trace file\n");
    exit(1);
  }
  if ((tr->type == USER) || (tr->code > FKT_UNSHIFTED_LIMIT_CODE)) {
    for (i = 0; i < ((tr->code & 0xff) - 12) / 4; i++) {
      if (fread(&(tr->args[i]), sizeof(int), 1, f_str) == 0) {
	fprintf(stderr, "Corrupted trace file\n");
	exit(1);
      }
    }
  }
  return 0;
}

void init_trace_file(char *supertrace)
{
  if (supertrace == NULL)
    supertrace = "prof_file";
  if ((f_str = fopen(supertrace ,"r")) == NULL) {
    fprintf(stderr,"Erreur dans l'ouverture du fichier %s\n", supertrace);
    exit(1);
  } 
  read_str_header();
}

void close_trace_file()
{
  fclose(f_str);
}
