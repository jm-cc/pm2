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

#define FUT_SETUP_CODE				0x210
#define FUT_KEYCHANGE_CODE			0x211
#define FUT_RESET_CODE				0x212
#define FUT_CALIBRATE0_CODE			0x220
#define FUT_CALIBRATE1_CODE			0x221
#define FUT_CALIBRATE2_CODE			0x222
#define FUT_SWITCH_TO_CODE			0x230
#define FUT_MAIN_ENTRY_CODE			0x240
#define FUT_MAIN_EXIT_CODE			0x340

struct code_name code_table[1000];
int nb_code;

static trace_buffer buf;

static FILE *f_fut;
static FILE *f_fkt;
static int thread;
static int pid;
  
void code_copy(int code, char *name)
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
  parser_start(fd);
  parser_run();
  parser_stop();
  close(fd);
  code_copy(0, NULL);
}

void add_buffer(trace tr)
{
  int i;
  trace_list tr_item;
  tr_item = (trace_list) malloc(sizeof(struct trace_item_st));
  assert(tr_item != NULL);
  tr_item->tr.clock = tr.clock;
  tr_item->tr.code = tr.code;
  tr_item->tr.thread = tr.thread;
  tr_item->tr.pid = tr.pid;
  tr_item->tr.proc = tr.proc;
  tr_item->tr.type = tr.type;
  for(i = 0; i < MAX_NB_ARGS; i++)
    tr_item->tr.args[i] = tr.args[i];
  tr_item->prev = buf.last;
  tr_item->next = EMPTY_LIST;
  buf.last = tr_item;
  if (buf.first == EMPTY_LIST)
    buf.first = tr_item;
}

int init_trace_buffer(char *futin, char *fktin)
{
  int n = 0;
  int i = 0;
  trace tr;
  char header[200];
  init();
  if ((f_fut = fopen(futin,"r")) == NULL) {
    fprintf(stderr,"Erreur dans l'ouverture du fichier\n");
    exit(1);
  } 
  fread(header, sizeof(unsigned long) + sizeof(double) + 
	2*sizeof(time_t) + sizeof(unsigned int), 1, f_fut);
  buf.first = EMPTY_LIST;
  buf.last = EMPTY_LIST;
  thread = 0;
  fread(&(tr.clock), sizeof(u_64), 1, f_fut);
  while(!feof(f_fut) && (n < TRACE_BUFFER_SIZE)) {
    int i;
    int j;
    fread(&(tr.code), sizeof(int), 1, f_fut);
    tr.thread = thread;
    j = 0;
    i = tr.code & 0xff;
    if ((tr.code >> 8) == FUT_SWITCH_TO_CODE) {
      fread(&(tr.args[0]), sizeof(int), 1, f_fut);
      i--;
      j++;
      thread = tr.args[0];
    }
    while (i != 0) {
      fread(&(tr.args[j]), sizeof(int), 1, f_fut);
      i--;
      j++;
    }
    add_buffer(tr);
    n++;
    fread(&(tr.clock), sizeof(u_64), 1, f_fut);
  }
  fclose(f_fut);
}
