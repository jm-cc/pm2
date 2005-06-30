#include <stdio.h>
#include <stdlib.h>
#include "time.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <fcntl.h>
#include <unistd.h>
#include "supertracebuffer.h"
#include "assert.h"
#include "superlwpthread.h"

#include "fkt.h"
#include <fkt/block.h>
#include <fkt/names.h>
#include <fkt/sysmap.h>
#include <fkt/pids.h>

#include "fut.h"

#define TRUE 1
#define FALSE 0
#define CORRUPTED_FUT(s) do {if (s) {fprintf(stderr,"Corrupted user trace file\n"); abort();}} while(0)
#define CORRUPTED_FKT(s) do {if (s) {fprintf(stderr,"Corrupted kernel trace file\n"); abort();}} while(0)

#define NB_MAX_CPU  16

static trace_buffer buf;          // Buffer of traces

int smp;                          // If the user trace is in smp or not

int abs_num;                      // Shouldn't be here it's a bug

static int dec;                   // Whether dec option is on or no
static int rel;                   // Whether rel option is on or no
static u_64 diff_time;            // time to be substracted to the current event
static u_64 min_fut;              // Minimum time for calibration for fut
static u_64 min_fkt;              // Minimum time for calibration for fkt

static int fd_fut;
static FILE *f_fut;               // file where to read fut events
static size_t n_fut;		  // position in file
static size_t pos_fut;		  // size to read in a page
static size_t fut_page_size;	  // size of a page
static int fd_fkt;
static FILE *f_fkt;               // file where to read fkt events
static size_t n_fkt;		  // size to read in a page
static size_t pos_fkt;		  // position in file
static size_t fkt_page_size;	  // size of a page
static int thread = -1;           /* Number of thread "currently running" 
				     in non SMP mode */
static int pid;                   // pid given by fut trace file

static int fut_eof;               // indicates the end of fut trace file
static int fkt_eof;               // indicates the end of fkt trace file
static trace fut_buf;             // 1 trace of pre-buffer for fut
static trace fkt_buf;             /* 1 trace of pre-buffer for fkt
				     So we can always take the first event
				     in time between fut and fkt */

static int pid_table[NB_MAX_CPU]; // Gives which pid is running on which cpu

static short int nb_cpu = 1;      // Number of cpus given by fkt
static u_64 begin_total;          // Time of the first event
static u_64 end_total;            // Time of the last event
static short int is_begining = 1; /* Indicates if it is the first event we see
				     (for begin_total) */
static double cpu_cycles;         /* read from fut or fkt to know the speed of 
				     the cpu */
static int highest_thr = 0;       // To know the higest number of threads

#define zone_flou 0x01000000      // fuzzy zone for the comparison function

int le(u_64 clock1, u_64 clock2)  /* Returns TRUE if clock1 <= clock2, 
				     handles zero problem */
{
  unsigned a;
  unsigned b;
  a = (unsigned) clock1;
  b = (unsigned) clock2;
  if (a < b) return TRUE;
  if ((b < zone_flou) && (a > 0xFFFFFFFF - zone_flou)) return TRUE;
  return FALSE;
}

/*
  reads an event from f_fut and places it in tr.
  If no more events in fut, returns 1 else returns 0
*/
static int read_user_trace(trace *tr)
{
  unsigned i;
  unsigned j;
  if (pos_fut>=n_fut) {
    fseek(f_fut,fut_page_size-pos_fut,SEEK_CUR);
    if (fread(&n_fut, sizeof(unsigned), 1, f_fut) < 1) {
      fut_eof=1;
      return -1;
    }
    pos_fut=sizeof(unsigned);
  }

  CORRUPTED_FUT(fread(&(tr->clock), sizeof(u_64), 1, f_fut) <= 0);
  pos_fut+=sizeof(u_64);

  CORRUPTED_FUT(fread(&(tr->code), sizeof(unsigned), 1, f_fut) <= 0);
  pos_fut+=sizeof(unsigned);
  tr->type = USER;

  j = 0;
  i = tr->nbargs;
  while (i != 0) {                                 // reads arguments
    CORRUPTED_FUT(fread(&(tr->args[j]), sizeof(unsigned), 1, f_fut) <= 0);
    pos_fut+=sizeof(unsigned);
    i--;
    j++;
  }
  return 0;
}

/*
  reads an event from f_fkt and places it in tr.
  If no more events in fkt, returns 1 else returns 0
*/
static int read_kernel_trace(trace *tr)
{
  unsigned i;
  unsigned j;
  if (pos_fkt>=n_fkt) {
    fseek(f_fkt,fkt_page_size-pos_fkt,SEEK_CUR);
    if (fread(&n_fkt, sizeof(unsigned), 1, f_fkt) < 1 || n_fkt == FKT_EOF) {
      fkt_eof=1;
      return -1;
    }
    pos_fkt=sizeof(unsigned);
  }

  CORRUPTED_FKT(fread(&j, sizeof(unsigned), 1, f_fkt) < 1);
  pos_fkt+=sizeof(unsigned);
  tr->clock = j;
  tr->type = KERNEL;

  CORRUPTED_FKT(fread(&j, sizeof(unsigned), 1, f_fkt) < 1);
  pos_fkt+=sizeof(unsigned);
  tr->cpu = j >> 16;
  tr->pid = j & 0xffff;

  if (pid_table[tr->cpu] == -1) {
    pid_table[tr->cpu] = tr->pid;
  }

  CORRUPTED_FKT(fread(&(tr->code), sizeof(unsigned), 1, f_fkt) < 1);
  pos_fkt+=sizeof(unsigned);
  if (tr->code > FKT_UNSHIFTED_LIMIT_CODE) {
    j = 0;
    i = tr->nbargs = ((tr->code & 0xff) - 12) / 4;
    while (i != 0) {
      CORRUPTED_FKT(fread(&(tr->args[j]), sizeof(unsigned), 1, f_fkt) < 1);
      pos_fkt+=sizeof(unsigned);
      i--;
      j++;
    }
  }
  return 0;
}


/* calculate all the informations (cpu number...)  for an user event */
static void fut_trace_calc(trace *tr)
{
  if (smp) {                                       // thread calculation
    if (tr->code >> 8 == FUT_SWITCH_TO_CODE) {
      tr->thread = tr->args[0];
    } else if (tr->code >> 8 == FUT_NEW_LWP_CODE) {
      tr->thread = tr->args[2];
    } else if (tr->code >> 8 == FUT_THREAD_BIRTH_CODE) {
      tr->thread = 0;  // For now on
    } else if (tr->code >> 8 == FUT_KEYCHANGE_CODE) {
      tr->thread = 0;  // For now on
    } else
      tr->thread = tr->args[0];
  } else
    tr->thread = thread;

  tr->pid = lwp_of_thread(tr->thread);              // Pid of this thread
  if (tr->pid == -1)
    tr->pid = pid;

  if ((tr->code >> 8) == FUT_SWITCH_TO_CODE) {      /* If switch_to, sets the 
						      new thread to lwp */
    set_switch(tr->args[0], tr->args[1]);

    if (!smp)
      thread = tr->args[1];                 // If !smp, updates thread
  } 

  //  else if ((tr->code >> 8) == FUT_NEW_LWP_CODE) { // IN case of FUT_NEW_LWP_CODE
  //    int n;
  //    add_lwp(tr->args[0], tr->args[2], tr->args[1]); /* adds this new lwp to
  //						       the lwp list */
  //    for(n = 0; n < NB_MAX_CPU; n++)                 /* tries to find the cpu
  //						       number */
  //      if (pid_table[n] == tr->args[0]) {
  //	set_cpu(tr->args[0], n);                    // sets the cpu to this lwp
  //	break;
  //      }
  //    if (!smp) thread = tr->args[2];                 // This is false!!!!!!!
  //  }

  if(smp)
    tr->cpu = cpu_of_lwp(tr->pid);                    // gets the cpu of this event
  else
    tr->cpu = 0;
  if ((tr->cpu >= NB_MAX_CPU) || (tr->cpu < 0)) {   /* Bad luck, no way of 
						       finding the cpu number */
    printf("Oups %d %d %d\n",tr->cpu, tr->pid, tr->thread);
  }
  tr->relevant = 1;
}


/* calculates all the informations (thread number, pid)... for a kernel event*/
static void fkt_trace_calc(trace *tr)
{
  tr->relevant = 0;
  if (pid_table[tr->cpu] == -1) {               // We found usefull information
    pid_table[tr->cpu] = tr->pid;
    //    if (is_lwp(tr->pid)) set_cpu(tr->pid, tr->cpu);   /* Sets this lwp to its 
    //							 correct cpu */
  }
  if (tr->code > FKT_UNSHIFTED_LIMIT_CODE) {
    if (tr->code >> 8 == FKT_SWITCH_TO_CODE) {
      tr->relevant = 1;
      pid_table[tr->cpu] = tr->args[0];
      if (is_lwp(tr->args[0])) {
	set_cpu(tr->args[0], tr->cpu);
	if (!smp) thread = thread_of_lwp(tr->args[0]);
      }
    } else if (tr->code >> 8 == FKT_USER_FORK_CODE) {
      tr->relevant = 1;
      add_lwp(tr->pid, tr->args[0], tr->args[1]);
      set_cpu(tr->pid, tr->cpu);
    }
  }
  if (is_lwp(tr->pid)) {               // If this event is an event from PM2
    tr->relevant = 1;
    tr->thread = thread_of_lwp(tr->pid);
  } else
    tr->thread = -1;
}

/* aiguillage vers fut_trace_calc ou fkt_trace_calc */
static void trace_calc(trace *tr)
{
  if (tr->type == USER)
    fut_trace_calc(tr);
  else
    fkt_trace_calc(tr);

  if (tr->thread > highest_thr)
    highest_thr = tr->thread;
}


/* read the fkt header */
static void read_fkt_header(int fkt)
{
  unsigned int ncpus;
  double *mhz;
  u_64 last;
  unsigned long fkt_pid;
  unsigned long fkt_kidpid;
  time_t t1, t2;
  clock_t start_jiffies, stop_jiffies;
  int n;
  size_t size,page_size;
  struct stat st;
  int fd=fkt?fd_fkt:fd_fut;

  if (fd >= 0) {
    ENTER_BLOCK(fd); /* main block */
    ENTER_BLOCK(fd);
    CORRUPTED_FKT(read(fd, &ncpus, sizeof(ncpus)) < sizeof(ncpus));
    nb_cpu = (short int) ncpus;
    printf("nb_cpu = %d\n",nb_cpu);
    mhz = (double *) malloc(sizeof(double)*ncpus);
    assert(mhz != NULL);
    read(fd, mhz, sizeof(double)*ncpus);
    cpu_cycles = mhz[0];
    CORRUPTED_FKT(read(fd,&fkt_pid, sizeof(fkt_pid)) < sizeof(fkt_pid));
    CORRUPTED_FKT(read(fd,&fkt_kidpid, sizeof(fkt_kidpid)) < sizeof(fkt_kidpid));
    CORRUPTED_FKT(read(fd,&t1, sizeof(t1)) < sizeof(t1));
    CORRUPTED_FKT(read(fd,&t2, sizeof(t2)) < sizeof(t2));
    CORRUPTED_FKT(read(fd,&start_jiffies, sizeof(start_jiffies)) < sizeof(start_jiffies));
    CORRUPTED_FKT(read(fd,&stop_jiffies, sizeof(stop_jiffies)) < sizeof(stop_jiffies));
    CORRUPTED_FKT(read(fd,&page_size, sizeof(page_size)) < sizeof(page_size));
    LEAVE_BLOCK(fd);

    ENTER_BLOCK(fd);
    fkt_load_irqs(fd);
    LEAVE_BLOCK(fd);

    ENTER_BLOCK(fd);
    CORRUPTED_FKT(read(fd,&n,sizeof(n)) < sizeof(n));
    {
      char uname[n+1];
      CORRUPTED_FKT(read(fd,uname,n) < n);
      uname[n]='\0';
      printf("%s\n",uname);
    }
    LEAVE_BLOCK(fd);
    
    ENTER_BLOCK(fd);
    fkt_load_sysmap(fd);
    LEAVE_BLOCK(fd);

    ENTER_BLOCK(fd);
    fkt_load_pids(fd);
    LEAVE_BLOCK(fd);

    LEAVE_BLOCK(fd);

    if (fstat(fd, &st)) {
      perror("fstat");
      exit(1);
    }
    if (S_ISBLK(st.st_mode)) {
      int sectors;
      if (ioctl(fd, BLKGETSIZE, &sectors)) {
	perror("getting device size\n");
	exit(1);
      }
      if (sectors >= 0xffffffffUL >> 9 )
	size = 0xffffffffUL;
      else
	size = sectors << 9;
    } else
	size = st.st_size;

    if (fkt) {
      if ((f_fkt = fdopen(fd,"r")) == NULL) {
        fprintf(stderr,"Error in fdopening file\n");
        exit(1);
      } 
      pos_fkt=n_fkt=fkt_page_size=page_size;
      for(n = 0; n < 3; n++) {
	/* get calibration minimum */
        do {
          last = fkt_buf.clock;
          CORRUPTED_FKT(read_kernel_trace(&fkt_buf) < 0);
	} while ((fkt_buf.code>>8) != FKT_CALIBRATE0_CODE);
        if (min_fkt == 0) min_fkt = fkt_buf.clock - last;
        else if (fkt_buf.clock - last < min_fkt) min_fkt = fkt_buf.clock - last;
      }
      do {
	CORRUPTED_FKT(read_kernel_trace(&fkt_buf) < 0);
      /* this is the real beginning */
      } while ((fkt_buf.code>>8) != FKT_KEYCHANGE_CODE);
      CORRUPTED_FKT(read_kernel_trace(&fkt_buf) < 0);
    } else {
      if ((f_fut = fdopen(fd,"r")) == NULL) {
        fprintf(stderr,"Error in fdopening file\n");
        exit(1);
      } 
      pos_fut=n_fut=fut_page_size=page_size;
      CORRUPTED_FUT(read_user_trace(&fut_buf) < 0);
      last = fut_buf.clock;
      CORRUPTED_FUT((fut_buf.code>>8) != FUT_SETUP_CODE);
      for(n = 0; n < 3; n++) {
	/* get calibration minimum */
        CORRUPTED_FUT(read_user_trace(&fut_buf) < 0);
        CORRUPTED_FUT((fut_buf.code>>8) != FUT_CALIBRATE0_CODE);
        if (min_fut == 0) min_fut = fut_buf.clock - last;
        else if (fut_buf.clock - last < min_fut) min_fut = fut_buf.clock - last;
      }
      for(n = 0; n < 3; n++) {
        CORRUPTED_FUT(read_user_trace(&fut_buf) < 0);
        CORRUPTED_FUT((fut_buf.code>>8) != FUT_CALIBRATE1_CODE);
      }
      for(n = 0; n < 3; n++) {
        CORRUPTED_FUT(read_user_trace(&fut_buf) < 0);
        CORRUPTED_FUT((fut_buf.code>>8) != FUT_CALIBRATE2_CODE);
      }
      CORRUPTED_FUT(read_user_trace(&fut_buf) < 0);
    }
  }
}


/* add an event in the buffer 
   Takes care of reordering by time
*/
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
    if (le(tmp->tr.clock,tr_item->tr.clock)) break;
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
  tr->number = abs_num++;                       // NO, shouldn't be there!!
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

/*
    Loads an event in the buffer, returns 1 if no more to add, 0 if ok
    Adds to the buffer the first event between the events in the pre_buffers. 
    Recharge the pre-buffer used
*/
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

/* initialise the buffer and open the trace*/
void init_trace_buffer(char *fut_name, char *fkt_name, int relative, 
		       int dec_opt)
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
  abs_num = 0;                                         // Should move
  for(n = 0; n < NB_MAX_CPU; n++)
    pid_table[n] = -1;
  n = 0;
  assert((fut_name != NULL) || (fkt_name != NULL));    /* One trace file must
							  be precised */
  if (fut_name != NULL) {
    if ((fd_fut = open(fut_name,O_RDONLY)) < 0) {
      fprintf(stderr,"Error in opening file %s\n", fut_name);
      exit(1);
    }
    fut_eof = 0;
  }
  if (fkt_name != NULL) {
    if ((fd_fkt = open(fkt_name,O_RDONLY)) < 0) {
      fprintf(stderr,"Error in opening file %s\n", fkt_name);
      exit(1);
    }
    fkt_eof = 0;
  }
  thread = 0; 
  buf.first = EMPTY_LIST;
  buf.last = EMPTY_LIST;
  read_fkt_header(0);
  read_fkt_header(1);
  while(n < TRACE_BUFFER_SIZE) {                    // Loads the buffer
    if (load_trace() != 0) break;
    n++;
  }
}

/* close the 2 traces */
void close_trace_buffer()
{
  fclose(f_fut);
  fclose(f_fkt);
}

/* give the next event to write in the supertrace */
int get_next_trace(trace *tr)
{
  int r;
  load_trace();
  r = get_buffer(tr);
  if (is_begining) {
    is_begining = 0;
    begin_total = tr->clock;
  }
  if (r != 0)
    end_total = tr->clock;
  trace_calc(tr);
  if ((tr->cpu >= NB_MAX_CPU) || (tr->cpu < 0)) {
    fprintf(stderr, "Trouble in computing. Maybe due to non corresponding\n");
    fprintf(stderr, "user and kernel traces\n");
    exit(1);
  }
  return r;
}


/* generate the supertrace header 
   short int        number of cpus 
                    (problem in mono mode always 1 but maybe 4 real cpus)
   u_64             time of beginning
   u_64             time of last event
   double           cpu_cycles
   int              highest number of thread
   int              number of lwp
   int[]            numeros of lwp
*/
void supertrace_end(FILE *f)
{
  int i,n,a;
  fwrite(&nb_cpu, sizeof(short int), 1, f);
  fwrite(&begin_total, sizeof(u_64), 1, f);
  fwrite(&end_total, sizeof(u_64), 1, f);
  fwrite(&cpu_cycles, sizeof(double), 1, f);
  fwrite(&highest_thr, sizeof(int), 1, f);
  n = number_lwp();
  assert(n < 116);
  fwrite(&n, sizeof(int), 1, f);
  for(i = 0; i < n; i++) {
    a = get_next_lwp();
    if (a == -1) {
      fprintf(stderr, "Please report bugs to cmenier@ens-lyon.fr\n");
      exit(1);
    }
    fwrite(&a, sizeof(int), 1, f);
  }
  a = get_next_lwp();
  if (a != -1) {
    fprintf(stderr, "Please report bugs to cmenier@ens-lyon.fr\n");
    exit(1);
  }
}
