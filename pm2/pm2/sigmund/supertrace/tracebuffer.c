#include <stdio.h>
#include <stdlib.h>
#include "time.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "tracebuffer.h"
#include "assert.h"
#include "lwpthread.h"

#include "fkt.h"

#include "fut_code.h"

#define TRUE 1
#define FALSE 0

#define NB_MAX_CPU  16

static trace_buffer buf;

int smp;

int abs_num;

static int dec;
static int rel;
static u_64 diff_time;
static u_64 min_fut;
static u_64 min_fkt;

static FILE *f_fut;
static FILE *f_fkt;
static int thread = -1;
static int pid;

static int fut_eof;
static int fkt_eof;
static trace fut_buf;
static trace fkt_buf;

static int pid_table[NB_MAX_CPU];

#define zone_flou 0x01000000

int le(u_64 clock1, u_64 clock2)
{
  unsigned a;
  unsigned b;
  a = (unsigned) clock1;
  b = (unsigned) clock2;
  if (a < b) return TRUE;
  if ((b < zone_flou) && (a > 0xFFFFFFFF - zone_flou)) return TRUE;
  return FALSE;
}

// Returns 0 if OK 1 if EOF
static int read_user_trace(trace *tr)
{
  int i;
  int j;
  if (fread(&(tr->clock), sizeof(u_64), 1, f_fut) == 0) {
    fut_eof = 1;
    return 1;
  }  
  if (fread(&(tr->code), sizeof(int), 1, f_fut) == 0) {
    fprintf(stderr,"Corrupted user trace file\n");
    exit(1);
  } 
  if (smp) {

  } else tr->thread = thread;
  tr->pid = lwp_of_thread(tr->thread);
  if (tr->pid == -1) tr->pid = pid;
  tr->type = USER;

  j = 0;
  i = ((tr->code & 0xff) - 12) / 4;
  if ((tr->code >> 8) == FUT_SWITCH_TO_CODE) {
    if (fread(&(tr->args[0]), sizeof(int), 1, f_fut) == 0) {
      fprintf(stderr,"Corrupted user trace file\n");
      exit(1);
    }
    if (fread(&(tr->args[1]), sizeof(int), 1, f_fut) == 0) {
      fprintf(stderr,"Corrupted user trace file\n");
      exit(1);
    }
    set_switch(tr->args[0], tr->args[1]);
    i-=2;
    j+=2;
    if (!smp) thread = tr->args[1];
  } else if ((tr->code >> 8) == FUT_NEW_LWP_CODE) {
    int n;
    if (fread(&(tr->args[0]), sizeof(int), 1, f_fut) == 0) {
      fprintf(stderr,"Corrupted user trace file\n");
      exit(1);
    }
    if (fread(&(tr->args[1]), sizeof(int), 1, f_fut) == 0) {
      fprintf(stderr,"Corrupted user trace file\n");
      exit(1);
    }
    if (fread(&(tr->args[2]), sizeof(int), 1, f_fut) == 0) {
      fprintf(stderr,"Corrupted user trace file\n");
      exit(1);
    }
    add_lwp(tr->args[0], tr->args[2], tr->args[1]);
    for(n = 0; n < NB_MAX_CPU; n++)
      if (pid_table[n] == tr->args[0]) {
	set_cpu(tr->args[0], n); 
	break;}
    if (!smp) thread = tr->args[2]; // Is this true, I have no clue
    i-=3;
    j+=3;
  }
  while (i != 0) {
    if (fread(&(tr->args[j]), sizeof(int), 1, f_fut) == 0) {
      fprintf(stderr,"Corrupted user trace file\n");
      exit(1);
    }
    i--;
    j++;
  }
  tr->cpu = cpu_of_lwp(tr->pid);
  tr->relevant = 1;
  return 0;
}

static int read_kernel_trace(trace *tr)
{
  int i;
  int j;
  tr->relevant = 0;
  if (fread(&j, sizeof(unsigned int), 1, f_fkt) == 0) {
    fkt_eof = 1;
    return 1;
  }
  tr->clock = j;
  tr->type = KERNEL;
  if (fread(&j, sizeof(unsigned int), 1, f_fkt) == 0) {
    fprintf(stderr,"Corrupted kernel trace file\n");
    exit(1);
  }
  tr->cpu = j >> 16;
  tr->pid = j & 0xffff;
  if (pid_table[tr->cpu] == -1) {
    pid_table[tr->cpu] = tr->pid;
    if (is_lwp(tr->pid)) set_cpu(tr->pid, tr->cpu);
  }
  if (fread(&(tr->code), sizeof(int), 1, f_fkt) == 0) {
    fprintf(stderr,"Corrupted kernel trace file\n");
    exit(1);
  }
  if (tr->code > FKT_UNSHIFTED_LIMIT_CODE) {
    j = 0;
    i = ((tr->code & 0xff) - 12) / 4;
    while (i != 0) {
      if (fread(&(tr->args[j]), sizeof(int), 1, f_fkt) == 0) {
	fprintf(stderr,"Corrupted kernel trace file\n");
	exit(1);
      }
      i--;
      j++;
    }
    if (tr->code >> 8 == FKT_SWITCH_TO_CODE) {
      tr->relevant = 1;
      assert(tr->args[1] < NB_MAX_CPU);
      pid_table[tr->args[1]] = tr->args[0];
      if (is_lwp(tr->args[0])) {
	set_cpu(tr->args[0], tr->args[1]);
	if (!smp) thread = thread_of_lwp(tr->args[0]);
      }
      //      if (tr->args[0] == pid) cpu = tr->args[1]; // Modifier bazar
    }
  }
  if (is_lwp(tr->pid)) {
    tr->relevant = 1;
    tr->thread = thread_of_lwp(tr->pid);
  } else tr->thread = -1;
  return 0;
}

static void read_fut_header()
{  
  u_64 last;
  int n;
  char header[200];
  if (f_fut != NULL) {
    // Lecture du header de fut : Attention non gestion de la corruption de fichier
    fread(&smp, sizeof(int), 1, f_fut);
    fread(&pid, sizeof(unsigned long), 1, f_fut);
    fread(header, sizeof(double) + 
	  2*sizeof(time_t) + sizeof(unsigned int), 1, f_fut);  
    if (read_user_trace(&fut_buf) != 0) {
      fprintf(stderr,"Corrupted user trace file\n");
      exit(1);
    }
    if (read_user_trace(&fut_buf) != 0) {
      fprintf(stderr,"Corrupted user trace file\n");
      exit(1);
    }
    last = fut_buf.clock;
    for(n = 0; n < 9; n++) {
      if (read_user_trace(&fut_buf) != 0) {
	fprintf(stderr,"Corrupted user trace file\n");
	exit(1);
      }
      if (min_fut == 0) min_fut = fut_buf.clock - last;
      else if (fut_buf.clock - last < min_fut) min_fut = fut_buf.clock - last;
    }
  }
}

static void read_fkt_header()
{
  unsigned int ncpus;
  double *mhz;
  u_64 last;
  unsigned long fkt_pid;
  unsigned long fkt_kidpid;
  time_t t1, t2;
  clock_t start_jiffies, stop_jiffies;
  unsigned nirqs, nints;
  unsigned int code;
  unsigned int i;
  int n;
  unsigned int max_nints;
  char name[400];
  if (f_fkt != NULL) {
    fread(&ncpus, sizeof(ncpus), 1, f_fkt);
    mhz = (double *) malloc(sizeof(double)*ncpus);
    assert(mhz != NULL);
    fread(mhz, sizeof(double), ncpus, f_fkt);
    fread(&fkt_pid, sizeof(unsigned int), 1, f_fkt);
    fread(&fkt_kidpid, sizeof(unsigned int), 1, f_fkt);
    fread(&t1, sizeof(time_t), 1, f_fkt);
    fread(&t2, sizeof(time_t), 1, f_fkt);
    fread(&start_jiffies, sizeof(clock_t), 1, f_fkt);
    fread(&stop_jiffies, sizeof(clock_t), 1, f_fkt);
    fread(&nirqs, sizeof(unsigned int), 1, f_fkt);
    fread(&ncpus, sizeof(unsigned int), 1, f_fkt);
    for(n = 0; n < nirqs; n++) {
      unsigned k;
      fread(&code, sizeof(unsigned int), 1, f_fkt);
      fread(&i, sizeof(unsigned int), 1, f_fkt);
      k = (i + 3) & ~3;
      fread(name, sizeof(char), k, f_fkt);
    }
    fread(&max_nints, sizeof(unsigned int), 1, f_fkt);
    fread(&nints, sizeof(unsigned int), 1, f_fkt);
    if (read_kernel_trace(&fkt_buf) != 0) {
      fprintf(stderr,"Corrupted kernel trace file\n");
      exit(1);
    } 
    if (read_kernel_trace(&fkt_buf) != 0) {
      fprintf(stderr,"Corrupted kernel trace file\n");
      exit(1);
    }
    last = fkt_buf.clock;
    for(n = 0; n < 9; n++) {
      if (read_kernel_trace(&fkt_buf) != 0) {
	fprintf(stderr,"Corrupted kernel trace file\n");
	exit(1);
      }
      if (min_fkt == 0) min_fkt = fkt_buf.clock - last;
      else if (fkt_buf.clock - last < min_fkt) min_fkt = fkt_buf.clock - last;
    }
  }
}



static void add_buffer(trace tr)
{
  int i;
  trace_list tr_item;
  trace_list tmp;
  tr_item = (trace_list) malloc(sizeof(struct trace_item_st));
  assert(tr_item != NULL);
  tr_item->tr.clock = tr.clock;
  tr_item->tr.code = tr.code;
  tr_item->tr.thread = tr.thread;
  tr_item->tr.pid = tr.pid;
  tr_item->tr.cpu = tr.cpu;
  tr_item->tr.type = tr.type;
  tr_item->tr.relevant = tr.relevant;
  for(i = 0; i < MAX_NB_ARGS; i++)
    tr_item->tr.args[i] = tr.args[i];
  tmp = buf.last;
  while (tmp != EMPTY_LIST) {
    // Beware pb of 0
    if (le(tmp->tr.clock,tr_item->tr.clock)) break;
    printf("One up %d(%u - %x - %u) / %d(%u - %x - %u)\n",tmp->tr.type, \
	   tmp->tr.cpu, tmp->tr.code, (unsigned) tmp->tr.clock, \
	   tr_item->tr.type, tr_item->tr.cpu, tr_item->tr.code, \
	   (unsigned)tr_item->tr.clock);
    tmp = tmp->prev;
  }
  tr_item->prev = tmp;
  if (tmp == EMPTY_LIST)
    buf.first = tr_item;
  else {
    tr_item->next = tmp->next;
    if (tmp != buf.last) {
      (tmp->next)->prev = tr_item;
    }
    tmp->next = tr_item;
  }
  if (tmp == buf.last)
    buf.last = tr_item;
}

// returns 0 if OK 1 if list is empty (after removing the last trace)
static int get_buffer(trace *tr)
{
  assert(buf.first != NULL);
  *tr = buf.first->tr;
  if (rel) {
    diff_time = tr->clock;
    rel = 0;
  }
  tr->clock = tr->clock - diff_time;
  if (dec) {
    if (tr->type == USER) diff_time += min_fut;
    else diff_time += min_fkt;
  }
  tr->number = abs_num++;
  if (buf.first->next == EMPTY_LIST) {
    free(buf.first);
    buf.first = EMPTY_LIST;
    buf.last = EMPTY_LIST;
    return 1;
  }
  else {
    trace_list tmp;
    tmp = buf.first->next;
    free(buf.first);
    tmp->prev = EMPTY_LIST;
    buf.first = tmp;
  }
  return 0;
}

// Returns 0 if good 1 if EOF
static int load_trace()
{
  if (fut_eof && fkt_eof) return 1;
  if ((f_fkt == NULL) || fkt_eof) {
    add_buffer(fut_buf);
    read_user_trace(&fut_buf);
  }
  else {
    if ((f_fut == NULL) || fut_eof){
      add_buffer(fkt_buf);
      read_kernel_trace(&fkt_buf);
    }
    else {
      // Attention au pb du 0 clock
      if (le(fut_buf.clock, fkt_buf.clock)) {
	add_buffer(fut_buf);
	read_user_trace(&fut_buf);
      }
      else {
	add_buffer(fkt_buf);
	read_kernel_trace(&fkt_buf);
      }
    }
  }
  return 0;
}

void init_trace_buffer(char *fut_name, char *fkt_name, int relative, int dec_opt)
{
  int n = 0;
  f_fut = NULL;
  f_fkt = NULL;
  rel = relative;
  dec = dec_opt;
  min_fut = 0;
  min_fkt = 0;
  diff_time = 0;
  fkt_eof = 1;
  fut_eof = 1;
  abs_num = 0;
  for(n = 0; n < NB_MAX_CPU; n++)
    pid_table[n] = -1;
  n = 0;
  assert((fut_name != NULL) || (fkt_name != NULL));
  if (fut_name != NULL) {
    if ((f_fut = fopen(fut_name,"r")) == NULL) {
      fprintf(stderr,"Erreur dans l'ouverture du fichier %s\n", fut_name);
      exit(1);
    } 
    fut_eof = 0;
  }
  if (fkt_name != NULL) {
    if ((f_fkt = fopen(fkt_name,"r")) == NULL) {
      fprintf(stderr,"Erreur dans l'ouverture du fichier %s\n", fkt_name);
      exit(1);
    } 
    fkt_eof = 0;
  }
  thread = 0; // Et oh, c'est de la bidouille, 
              // mais bon corrigée par FUT_NEW_LWP
  buf.first = EMPTY_LIST;
  buf.last = EMPTY_LIST;
  read_fut_header();
  read_fkt_header();
  while(n < TRACE_BUFFER_SIZE) {
    if (load_trace() != 0) break;
    n++;
  }
}

void close_trace_buffer()
{
  fclose(f_fut);
  fclose(f_fkt);
}

int get_next_trace(trace *tr)
{
  load_trace();
  return get_buffer(tr);
}
