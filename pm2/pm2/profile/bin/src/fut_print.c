/*	fkt_print.c

	fkt_print --	program to analyse and print trace files produced by
					fkt_record using fkt (fast kernel tracing) in the kernel.

	Copyright (C) 2000, 2001  Robert D. Russell -- rdr@unh.edu
				  2003, 2004  Samuel Thibault -- samuel.thibault@ens-lyon.org

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA


*/

/*	As of 30-Jul-03,
 *
	unshifted codes (for traps/syscalls/irqs) MUST be less than
		FKT_UNSHIFTED_LIMIT_CODE.

	singleton codes (i.e., those that are NOT paired in entry-exit pairs)
		MUST be greater than FKT_UNPAIRED_LIMIT_CODE
		and be less thant GCC_TRACED_FUNCTION_X86_MINI
		FKT_SETUP_CODE and less than FKT_LAST_FKT_CORE.

	function entry and exit codes MUST be greater than
		FKT_UNSHIFTED_LIMIT_CODE.
	function entry codes MUST have the FKT_GENERIC_EXIT_OFFSET bit set.

	gcc traced functions codes are greater than GCC_TRACED_FUNCTION_X86_MINI
		entry codes must have GCC_TRACED_FUNCTION_X86_EXIT bit set
*/

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/sendfile.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <limits.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <linux/unistd.h>
#include <linux/fkt.h>
#include "fxt.h"
#include "fut.h"

/* one day, FUT and FKT could be both enabled... */
#if !defined(FUT)
#  define FKT
#endif

#include "fxt-tools.h"
//#include "fxt/names.h"
#ifdef FKT
#  define find_name(code, keep, maxlen) ({ \
	const char *_res; \
	if (code >= FKT_I386_FUNCTION_MINI) \
		_res=fxt_lookup_symbol(fxt,code|FKT_I386_FUNCTION_EXIT); \
	else \
		_res=fxt_find_name(fxt, code, keep, maxlen, fkt_code_table); \
	_res; \
	})
#endif
#ifdef FUT
struct fxt_code_name fut_code_table [] = 
	{
#  if !defined(PREPROC) && !defined(DEPEND)
#    include "fut_print.h"
#  endif
	{0, NULL }
	};
#  define find_name(code, keep, maxlen) ({ \
	const char *_res; \
	if (code >= FUT_I386_FUNCTION_MINI) \
		_res=fxt_lookup_symbol(fxt,code&~FUT_I386_FUNCTION_EXIT); \
	else \
		_res=fxt_find_name(fxt, code, keep, maxlen, fut_code_table); \
	_res; \
	})
#endif

#define MALLOC(t,fmt,...) \
	do \
	if( (t = malloc(sizeof(typeof(*t)))) == NULL ) \
		{ \
		fprintf(stderr, "unable to malloc " fmt "\n", ## __VA_ARGS__); \
		exit(EXIT_FAILURE); \
		} \
	while (0);

#define CALLOC(t,size) \
	do \
	if( (t = calloc(size, sizeof(typeof(*t)))) == NULL ) \
		{ \
		fprintf(stderr, "unable to calloc "#t"[%d]\n", size); \
		exit(EXIT_FAILURE); \
		} \
	while (0);

#if 1
#define printf(fmt, ...) do {\
	printf(fmt, ## __VA_ARGS__); \
	fflush(stdout); \
} while (0)

#define fprintf(f, fmt, ...) do {\
	fprintf(f, fmt, ## __VA_ARGS__); \
	fflush(f); \
} while (0)
#endif

#define HISTO_SIZE	512
#define MAX_HISTO_BAR_LENGTH	25
#define HISTO_CHAR	'*'

#define NAMELEN	28
#define NAMELENONLY	23		/* leave some space for " ONLY" */
#define CYCLEN 14
#define COUNTLEN 9
#define AVGLEN 7

#define _S(x) #x
#define S(x) _S(x)
#define NAMESTR "%" S(NAMELEN) "s"
#define NAMESTRONLY "%" S(NAMELENONLY) "s"
#define CYCSTR "%" S(CYCLEN) "s"
#define CYCNUM "%" S(CYCLEN) "llu"
#define COUNTSTR "%" S(COUNTLEN) "s"
#define COUNTNUM "%" S(COUNTLEN) "u"
#define AVGSTR "%" S(AVGLEN) "s"
#define AVGNUM "%" S(AVGLEN) ".0f"
#define COMMSTR	" %" S(FXT_MAXCOMMLEN) "s"
#define COMMNAME	"Command"
#define COMMUNDERLINE "-------"
#ifdef FKT
#  define SWITCH_TO_CODE FKT_SWITCH_TO_CODE
#  define CALIBRATE0_CODE FKT_CALIBRATE0_CODE
#  define CALIBRATE1_CODE FKT_CALIBRATE1_CODE
#  define CALIBRATE2_CODE FKT_CALIBRATE2_CODE
#  define GCC_INSTRUMENT_ENTRY_CODE FKT_GCC_INSTRUMENT_ENTRY_CODE
#  define GCC_INSTRUMENT_EXIT_CODE FKT_GCC_INSTRUMENT_EXIT_CODE
#  define GCC_TRACED_FUNCTION_X86_EXIT FKT_I386_FUNCTION_EXIT
#  define fxt_lookup_pid fkt_lookup_pid
#endif
#ifdef FUT
#  define SWITCH_TO_CODE FUT_SWITCH_TO_CODE
#  define CALIBRATE0_CODE FUT_CALIBRATE0_CODE
#  define CALIBRATE1_CODE FUT_CALIBRATE1_CODE
#  define CALIBRATE2_CODE FUT_CALIBRATE2_CODE
#  define GCC_INSTRUMENT_ENTRY_CODE FUT_GCC_INSTRUMENT_ENTRY_CODE
#  define GCC_INSTRUMENT_EXIT_CODE FUT_GCC_INSTRUMENT_EXIT_CODE
#  define GCC_TRACED_FUNCTION_X86_EXIT FUT_I386_FUNCTION_EXIT
#  define fxt_lookup_pid fut_lookup_pid
#endif

#define UNAMESTRLEN 256

/*	use 64-bit arithmetic for all times */
typedef unsigned long long u_64;

/*
 *
 * Settings variables
 *
 */

/* set by -a switch to know which pid to take statistics for */
static int statspid = -1;

/* set by -p switch to know which pid to print */
static int printpid = -1;
/* if the pid is prepended with +, only that pid will be printed */
static int printonlypid;
static int printing;

/* set to stdout by -d, /dev/null else */
static FILE *stdbug;

/* set to 1 by -k switch to ignore user time */
static int kernel_only;

/* set by -b to record the page buffering statistics */
static char *pagebufstats;

/* set by -t to record pages filling time */
static char *pagetimestats;

/*
 * lib fxt structure
 */
static fxt_t fxt;
static struct fxt_infos *info;

/*
 * 
 * Some limits and their actual use
 *
 */

/* number of function probes */
#define MAX_FUNS 2000
static unsigned int	actual_max_funs_pos = 0;

/* number of times the same function could be called (nested or between
 * processes) */
#ifdef FKT
#  define MAX_EACH_FUN_RECUR 32
#endif
#ifdef FUT
#  define MAX_EACH_FUN_RECUR 256
#endif
static unsigned int	actual_max_cycles_pos = 0;

/* nesting level of processes stacks */
#define MAX_FUN_RECUR 64
static unsigned int	actual_max_pos = 0;

/*
 *
 * Machine information
 *
 */

/* per-cpu speed */
static unsigned int	lastcpu = -1;

/* fun_time is initially 0.
 * set to 1 when hit first entry for kidpid task
 * while 1, allows accumulation of entry/exit statistics
 * set to -1 when hit burying of kidpid task and it is already set to 1.
 */

static size_t		page_size;

static int fun_time = 0;

static unsigned int trace_format = 1;
static const char* trace_format_name[] = { 
	[0]="No format",
	[FKT_FORMAT]="Kernel traces",
	[FUT_FORMAT_MONO]="User mono traces",
	[FUT_FORMAT_SMP]="User SMP traces",
};


/*	called once to start statistics taking */
void start_taking_stats( void )
	{
	fprintf(stdbug, "start taking stats, lastcpu %u\n", lastcpu);
	fun_time = 1;			/* start the statistics taking */
	}


/*	called once to stop statistics taking */
void stop_taking_stats( void )
	{/* this times() occurred while taking statistics */
	fprintf(stdbug, "stop taking stats\n");
	fun_time = -1;			/* stop the statistics taking */
	}


/*
 *
 * Per-process total cycles accounting (both user and kernel level)
 *
 */

struct pid_cycles_item	{
	int						pid;
	u_64					total_cycles;
	int						dead;
	char					name[FXT_MAXCOMMLEN+1];
	struct pid_cycles_item	*next;
};

static struct pid_cycles_item	*pid_cycles_list;
static int			pid;
static int			kidpid;

void find_category( int z, char *category, const char **name )
	{
	if( z == kidpid )
		{/* this is the user process */
		*name=fxt_lookup_pid(fxt,z);
		*category = 'u';
		}
	else if( z <= 0 )
		{/* this is an idle process (z <= 0) */
		*name = "idle process";	/* mustn't be longer than FXT_MAXCOMMLEN */
		*category = 'i';
		}
	else if( z == pid )
		{
		/* this is fkt_record process */
		*name=fxt_lookup_pid(fxt,z);
		*category = 'f';
		}
	else
		{/* this is some other process (z != 0) */
		*name=fxt_lookup_pid(fxt,z);
		*category = 'o';
		}
	}

void update_pid_cycles( u_64 cycles, int pid )
	{
	struct pid_cycles_item	*ptr;

	for( ptr = pid_cycles_list;  ptr != NULL;  ptr = ptr->next )
		{
		if( ptr->pid == pid && !ptr->dead )
			{/* found item for this pid, just increment its cycles */
			ptr->total_cycles += cycles;
			fprintf(stdbug, "add %llu cycles to pid %d, total now %llu\n",
				cycles, pid, ptr->total_cycles);
			return;
			}
		}
	/* loop finishes when item for this pid not found, make a new one */
	MALLOC(ptr,"pid_cycles_item");
	ptr->pid = pid;
	ptr->total_cycles = cycles;
	ptr->dead = 0;
	strcpy(ptr->name,fxt_lookup_pid(fxt,pid));
	ptr->next = pid_cycles_list;		/* add to front of list */
	pid_cycles_list = ptr;
	fprintf(stdbug,"create new cycles for pid %d, initial %llu\n",pid,cycles);
	}

void update_pid_name(int pid, char *name)
	{
	struct pid_cycles_item	*ptr;

	for( ptr = pid_cycles_list;  ptr != NULL;  ptr = ptr->next )
		{
		if( ptr->pid == pid && !ptr->dead )
			{
			strcpy(ptr->name, name);
			return;
			}
		}
	/* loop finishes when item for this pid not found, make a new one */
	MALLOC(ptr,"pid_cycles_item");
	ptr->pid = pid;
	ptr->total_cycles = 0;
	ptr->dead = 0;
	strcpy(ptr->name,name);
	ptr->next = pid_cycles_list;		/* add to front of list */
	pid_cycles_list = ptr;
	fprintf(stdbug,"create new cycles for pid %d\n",pid);
	}

void bury_pid( int pid )
	{
	struct pid_cycles_item	*ptr;

	if( statspid>0 && pid == statspid )
		stop_taking_stats();
	
	if( printpid>0 && pid == printpid )
		printing = -1;

	for( ptr = pid_cycles_list; ptr; ptr = ptr->next )
		{
		if( ptr->pid == pid && !ptr->dead)
			{/* found the process to bury */
			ptr->dead=1;
			break;
			}
		}
	/* if it was not found, well, no problem, we hadn't seen it alive,
	 * that's all */
	}

/*	draw a horizontal line of 80 c characters onto stdout */
void my_print_line( int c )
	{
	int		i;

	for( i = 0;  i < 80;  i++ )
		putchar(c);
	putchar('\n');
	}

void print_cycles( u_64 ratio )
	{
	struct pid_cycles_item	*ptr;
	double					t, r, jtot, ptot;
	u_64					total;
	char					category;
	const char				*name;

	r = (double)ratio;
	total = 0ULL;
	jtot = 0.0;
	ptot = 0.0;
	printf("\n\n%48s\n\n","PROCESSES CYCLES");
	my_print_line('*');
	printf("\n");
	printf("%c"COMMSTR" %6s "CYCSTR" %15s %11s\n", ' ', COMMNAME, "pid", "cycles", "jiffies", "percent");
	printf("%c"COMMSTR" %6s "CYCSTR" %15s %11s\n", ' ', COMMUNDERLINE, "---", "------", "-------", "-------");
	for( ptr = pid_cycles_list;  ptr != NULL;  ptr = ptr->next )
		{
		t = ((double)ptr->total_cycles);
		total += ptr->total_cycles;
		jtot += t/r;
		}
	for( ptr = pid_cycles_list;  ptr != NULL;  ptr = ptr->next )
		{
		t = ((double)ptr->total_cycles);
		find_category(ptr->pid,&category,&name);
		printf("%c"COMMSTR" %6d "CYCNUM" %15.2f %10.2f%%\n",
					category, 
					ptr->name,
					ptr->pid, ptr->total_cycles, t/r, t*100.0/((double)total));
		ptot += t*100.0/((double)total);
		}
	printf("%c"COMMSTR" %6s "CYCNUM" %15.2f %10.2f%%\n", ' ', "", "TOTAL", total, jtot, ptot);
	}

/*
 *
 * Page buffering statistics
 *
 * We count the pages flushing and write a data file in order to be able to
 * draw a nice gnuplot graph
 *
 */

/* record the state of fkt pages at the end of each record page */
struct page_status	{
	u_64		time;			/* time of the end of this page on CPU 0 */
	unsigned	writing;		/* currently writing pages*/
	unsigned	nbsyncs;		/* nb of synchronizations done */
	unsigned	written;		/* actually written pages */
	unsigned	extra_written;	/* written pages in a hurry */
	unsigned	nbpages;		/* nb of pages */
};
static struct page_status *page_status, *current_page_status;

/* count the number of syncs we had to do */
static u_64 nbsyncs;

void page_stats(unsigned long code, int arg)
	{
	switch(code)
		{
#ifndef FUT
		case FKT_SETUP_CODE:
#else
		case FUT_SETUP_CODE:
#endif
			current_page_status->nbpages=arg;
			break;
#ifndef FUT
		case FKT_RECORD_WRITING_CODE:
			current_page_status->writing++;
			break;
		case FKT_RECORD_WRITTEN_CODE:
			current_page_status->written++;
			break;
		case FKT_RECORD_EXTRA_WRITTEN_CODE:
			current_page_status->extra_written++;
			break;
		case FKT_RECORD_EXTRA_PAGE_CODE:
			current_page_status->nbpages++;
			break;
		case FKT_FKT_FORCE_SYNC_ENTRY_CODE:
			current_page_status->nbsyncs++;
			nbsyncs++;
			break;
#endif
		}
	}

void page_stats_print(unsigned long nbpages)
	{
			/* bad: we're assuming every cpu is at the same speed */
	unsigned i;
	FILE *stats;
	u_64 lasttime=0ULL;
	if( pagebufstats )
		{
		if( !(stats=fopen(pagebufstats,"w")) )
			{
			fprintf(stderr,"can't open buffer stats data file %s\n",pagebufstats);
			exit(EXIT_FAILURE);
			}
		for (i=0;i<nbpages;i++)
			fprintf(stats,"%u %llu %u %u %u %u %u\n",i,
				page_status[i].time,
				page_status[i].writing,
				page_status[i].nbsyncs,
				page_status[i].written,
				page_status[i].extra_written,
				page_status[i].nbpages
			);
		fclose(stats);
		}
	if (pagetimestats)
		{
		if( !(stats=fopen(pagetimestats,"w")) )
			{
			fprintf(stderr,"can't open page time stats data file %s\n",pagetimestats);
			exit(EXIT_FAILURE);
			}
		if (nbpages)
			lasttime=page_status[0].time;
		for (i=0;i<nbpages;i++)
			{
			fprintf(stats,"%i %llu %llu\n",i,
				page_status[i].time,
				page_status[i].time-lasttime);
			lasttime=page_status[i].time;
			}
		fclose(stats);
		}
	printf("\n\nbuffer: %d -> %d\n",page_status[0].nbpages,page_status[nbpages].nbpages);
	printf("total synchronizations: %llu\n",nbsyncs);
	}

/*

   Each function has its own fun_item element in the funs[] array.
   start_cycle

Exit codes for each function are 0x100 more than the entry probe codes.
Therefore, the way that the time spent on executing a function is calculated
is as follows.  When a function is entered for the first time, store
it in the array "funs", with its entry code, and cycles in the start_cycle.
when the exit code is seen for that function, index into the "funs" array
and subtract the entry cycles from the exit cycle.  The difference is stored
in the field "total" cumulatively. If before seeing an exit, another entry
for the same function is seen, no problem, this can also be stored
in the start_cycle and popped out appropriately... caller_pid lets
parallel calls properly handled: start_cycle gets shifted when the top
of calls for a process is not the top of calls for this function.

*/


/*	One of these items in funs[] array for each function called */
struct fun_item	{
	/* the code identifying this func */
	unsigned long	code;

	/* total number of calls to this func */
	unsigned int	counter;

	/* total number of cycles in this func */
	u_64			total;

	/* total number of cycles in this func ONLY */
	u_64			self_total;

	/* stack of start times of nested or parallel open calls to
	   this func (-1 if not open call) */
	u_64			start_cycle[MAX_EACH_FUN_RECUR];

	/* stack of pid that made open call */
	/* a pid is signed: negative values are for idle processes, and denote */
	/* the cpu */
	int				caller_pid[MAX_EACH_FUN_RECUR];

	/* ith slot is cycles spent in func i
	   when called from this func */
	u_64			called_funs_cycles[MAX_FUNS];

	/* ith slot is number of calls to
	   func i from this func */
	unsigned int	called_funs_ctr[MAX_FUNS];
};

/* The funs[] array stores information for unique functions actually called */
static int newfun_pos;			/* index to next slot to use in funs[] */
static struct fun_item funs[MAX_FUNS];

/*	stacks keep track of start cycle and code for nested function calls.
	One stack per processus, kept in linked-list headed by stack_head.
	Current stack is pointed to by cpu_stack[lastcpu]. */

/*	Keep kernel function nesting statistics in the stack arrays.
	Every cycle is accounted for exactly once.

	Each time a kernel function is entered, push (pos++)
	onto the code stack its entry code,
	and onto the start_cycle stack the time it occurred.

	When the corresponding kernel function returns, pop (pos--)
	the stacks, using the difference between the start_cycle and the
	time of the return to compute the time spent in the function.
	Add this time to all the other start_cycle times in this stack
	(thereby removing the cycles spent in this nested function from those
	that called it)

	Note that the first entry of the array is used to keep the starting
	time of the process (considered as the main function to some extends) */

struct stack_item {
	/* the pid to whom this item belongs */
	int					pid;
	/* the last cpu that worked on this item */
	unsigned int		cpu;
	/* total number of cycles used in user level by this pid */
	u_64				user_cycles;
	/* last reltime seen for this pid */
	u_64				lastreltime;

	/* index to next unused slot on v3 stacks */
	unsigned int		pos;
	/* stack of start cycles for open function calls */
	u_64				start_cycle[MAX_FUN_RECUR];
	/* stack of entry codes for open function calls */
	unsigned long		code[MAX_FUN_RECUR];

	/* stack of indexes to entry codes in funs array */
	unsigned int		fun_index[MAX_FUNS];
	/* index to next unused slot on index_array */
	unsigned int		next_fun_index;

	/* link to item for next pid */
	struct stack_item	*next;
};

static struct stack_item	*stack_head;
static struct stack_item	**cpu_stack;

/* once a process is dead, it's useless to keep its stack, only keep its
 * process cycle count and pid
 * This also avoids mixing processes with the same pid */

struct dead_process {
	/* the pid the process had, only for displaying */
	int					pid;

	/* total number of cycles used in user level by this pid */
	u_64				user_cycles;

	char				name[FXT_MAXCOMMLEN+1];

	/* next dead process */
	struct dead_process	*next;
};

static struct dead_process *grave_head;

#ifndef FUT
/*	arrays to keep track of number of occurrences and time accumulated */
static u_64			syscall_cycles[FKT_UNSHIFTED_LIMIT_CODE];
static unsigned int	syscall_counter[FKT_UNSHIFTED_LIMIT_CODE];
#endif

/*	basetime[thiscpu] is first absolute time ever seen for thiscpu */
/*	lasttime[thiscpu] is last  absolute time ever seen for thiscpu */
/*	lastreltime[thiscpu] is last relative time ever seen for thiscpu */
/*	should have lastreltime = lasttime - basetime */
/*	basehigh is the current 32bit hi part of the 64bit time */
static u_64		basetime,*lasttime;
static u_64		*basehigh;
static u_64		*lastreltime;

/*	start_time_cpu[thiscpu] is 1st relative time for thiscpu once taking stats*/
/*	end_time_cpu[thiscpu] is 1st relative time for thiscpu after taking stats */
static u_64		*start_time_cpu, *end_time_cpu;

/*	already_saw[thiscpu] is 0 before any slot for thiscpu is seen, else 1 */
/*	fun_time_cpu[thiscpu] is 1 after thiscpu has started taking stats,
							-1 after thiscpu has finished taking stats */
static int			*already_saw, *fun_time_cpu;
static int			*lastpid;
static clock_t		start_jiffies, stop_jiffies;
static int			fd;

/* this gets incremented each time we charge a a function with
 * cycles, to detect incoherencies as soon as possible */
static u_64			accounted;

/* first probe for this pid, create and initialize a new stack item for it */
struct stack_item *create_stack( int thispid, unsigned int thiscpu, u_64 stack_start_time )
	{
	struct stack_item	*stack;

	fprintf(stdbug, "create_stack for pid %d, cpu %u, rel time %llu\n",
		thispid, thiscpu, stack_start_time);

	MALLOC(stack,"stack for pid %d", thispid);

	stack->pid = thispid;
	stack->cpu = thiscpu;
	stack->user_cycles = 0LL;
	stack->pos = 0;
	stack->code[0] = 0;
	stack->start_cycle[0] = stack_start_time;
	stack->next_fun_index = 0;
	stack->fun_index[0] = 0;

	stack->next = stack_head;	/* push on stack list */
	stack_head = stack;
	return stack;
	}

/* shift function start cycles when a function returns (including switch_to) */
void update_stack( struct stack_item	*stack, u_64 delta, unsigned long code )
	{
	unsigned int	i, k;

	/* move up start time of all previous in nesting */
	for( i = 0;  i < stack->pos;  i++ )
		stack->start_cycle[i] += delta;

	/* if this is a switch, update functions as well */
	if ( code == SWITCH_TO_CODE 
#ifdef FKT
	     || code < FKT_UNSHIFTED_LIMIT_CODE
#endif
		)
		{
		for( k = 0;  k < newfun_pos; k++ )
			for ( i = 0; i<MAX_EACH_FUN_RECUR && funs[k].start_cycle[i] != -1LL;
					i++ )
				if (funs[k].caller_pid[i] == stack->pid)
					funs[k].start_cycle[i] += delta;
		}

	/* then pop the stack */
	stack->pos--;
	fprintf(stdbug, "stack updated by %llu, pop stack for pid %d, pos %d\n",
		delta, stack->pid, stack->pos);
	}


/* setup stack for this pid, also update this pid's stack */
void setup_stack( u_64 cyc_time, int thispid )
	{
	u_64	delta, off_cyc_time = lastreltime[lastcpu];


	fprintf(stdbug, "setup_stack for pid %d, cpu %u\n", thispid, lastcpu);

	if( cpu_stack[lastcpu] == NULL )
		{/* first time through here, we need a stack */
		cpu_stack[lastcpu] = create_stack(thispid, lastcpu, cyc_time);
		}
	else if( cpu_stack[lastcpu]->pid != thispid )
		{/* next probe not the current pid, find a stack for the new pid */
		struct stack_item	*stack;

		/*	search list of existing items to find one for this pid */
		for( stack = stack_head; stack != NULL; stack = stack->next )
			{
			if( stack->pid == thispid)
				{/* found an existing stack for the new pid */
				fprintf(stdbug, "go back to stack for pid %d, cpu %u, pos %d\n",
					thispid, stack->cpu, stack->pos);
				if( stack->code[stack->pos] == SWITCH_TO_CODE )
					{/* top of previous stack is switch_to, pop it off */
					cpu_stack[lastcpu] = stack;
					/* and use the time of the last probe (this process might
					 * have handled an IRQ just before actually yielding) */
					delta = off_cyc_time - stack->lastreltime;
					/* move up start of all previous in nesting and pop stack */
					update_stack(stack, delta, SWITCH_TO_CODE);
					}
				else
				/* we've already seen this pid before, it should have switched*/
					EPRINTF("previous stack does not have switch_to on top,"
								" pos = %d\n", stack->pos);
				stack->cpu = lastcpu;	/* remember last cpu to work here */
				break;
				}
			}
		if( stack == NULL )
			/* first probe for this pid, create and initialize a new stack */
			stack = create_stack(thispid, lastcpu, lastreltime[lastcpu]);
		cpu_stack[lastcpu] = stack;
		}
		else
		{ /* same processus as before, but might not have really switched */
		}
	cpu_stack[lastcpu]->lastreltime=cyc_time;
	}

/* exiting the corresponding entry in funs array at slot k */
void exit_function( struct stack_item *stack, int k,
	int thispid, u_64 cyc_time )
	{
	int	i, j = -1;
	u_64	cycdiff;

	/* find top of start_cycle stack for this pid
	   (start time of last call to this function) */
	for ( i = 0; i < MAX_EACH_FUN_RECUR; i++ )
		{
		if ( funs[k].start_cycle[i] == -1LL )
			break;
		if ( funs[k].caller_pid[i] == thispid )
			j = i;
		}
	if( j == -1 )
		/* no open (unexited) call to this function for this pid !?! */
		EPRINTF("no open call to function %s from pid %d\n", find_name(funs[k].code,1,-1), thispid);
	else
		{/*	compute time spent in last call to this function */
		cycdiff = cyc_time - funs[k].start_cycle[j];

		/* add this time to total time ever spent in function */
		funs[k].total += cycdiff;
		funs[k].counter++;
		fprintf(stdbug,"adding %llu cycles to this function, reaching %llu\n",
					cycdiff, funs[k].total);

		/* "shift" the stack of calls to this function */
		memmove(&funs[k].start_cycle[j],&funs[k].start_cycle[j+1],
			((char *) (&funs[k].start_cycle[i]))-
			((char *) (&funs[k].start_cycle[j])));
		memmove(&funs[k].caller_pid[j],&funs[k].caller_pid[j+1],
			((char *) (&funs[k].caller_pid[i]))-
			((char *) (&funs[k].caller_pid[j])));

		if (k == stack->fun_index[stack->next_fun_index - 1])
			{
			if (stack->next_fun_index >= 2 )
				{/* update information for the caller function */
				j = stack->fun_index[stack->next_fun_index - 2];
				funs[j].called_funs_cycles[k] += cycdiff;
				}

			stack->fun_index[stack->next_fun_index]=-1;
			stack->next_fun_index--;
			}
		else
			EPRINTF("closing call to %s does not match the stack of pid %d\n",
							find_name(funs[k].code,1,-1), thispid);
		}
	}


/* code for return from kernel functions, updating the stack */
/* this is the exit corresponding to a previously stacked entry */
/* or a switch back to previously stacked switch from */
void return_from_function( struct stack_item	*stack, unsigned long code,
				u_64 cyc_time )
	{
	int		k;
	u_64	delta = cyc_time - stack->start_cycle[stack->pos];

	/* move up start time of all previous in nesting and pop stack */
	update_stack(stack, delta, code);

	/* and update this function self total cycles */
	for( k = 0;  k < newfun_pos;  k++ )
		{
		if ( code == funs[k].code )
			{
			funs[k].self_total += delta;
			accounted += delta;
			fprintf(stdbug, "update function %s ONLY by %llu, total now %llu\n",
				find_name(code,1,-1), delta, funs[k].self_total);
			return;
			}
		}
	EPRINTF("function %s wasn't opened\n",find_name(code,1,-1));
	}

#ifdef FKT
/* code for return from system calls */
void return_from_sys_call( struct stack_item	*stack, unsigned long code,
		u_64 cyc_time )
	{
	char		category;
	const char	*name;
	int			should_be_hex;
	u_64		delta;

	if( stack->pos > 0 && code<FKT_UNSHIFTED_LIMIT_CODE )
		{/* stacked code was a trap/syscall/IRQ */
		fkt_find_syscall(fxt,code, &category, &name, &should_be_hex);
		delta = cyc_time - stack->start_cycle[stack->pos];
		syscall_cycles[code] +=  delta;
		syscall_counter[code] += 1;
		accounted += delta;
		fprintf(stdbug, "update %c %s by %llu, total now %llu\n",
			category, name, delta, syscall_cycles[code]);

		/* move up start time of all previous in nesting & pop stack */
		update_stack(stack, delta, code);
		}
	else if( stack->pos == 0 )
		{/* return to a process that entered the kernel before we
			started to record trace info, just ignore this return */
		fprintf(stdbug, "unmatched return from sys_call, ignored\n");
		}
	else
		EPRINTF("return from system call improperly nested: "
			"code %08lx, depth %d\n", code,stack->pos);
	}
#endif /* FKT */

/*	here to close a non-empty v3 stack (pos>0) when we stop taking stats */
/*	cyc_time is current (relative) time since the starting base time */
void close_stack( struct stack_item	*stack, int thispid, u_64 cyc_time )
	{
	unsigned long	code;

	fprintf(stdbug, "close stack for pid %4d, pos %u, cyc_time %llu\n",
		thispid, stack->pos, cyc_time);

	if( stack->pos > 0 && stack->code[stack->pos] == SWITCH_TO_CODE )
		{/* top of current stack is switch_to, ie the process was blocked
		  * use its time and pop it */
		cyc_time = stack->lastreltime;
		stack->pos--;
		}

	while( stack->pos > 0 )
		{
		/*	code gets code at top of this stack */
		code = stack->code[stack->pos];
		fprintf(stdbug, "v3[%u] ", stack->pos);
#ifdef FKT
		if( code < FKT_UNSHIFTED_LIMIT_CODE )
			{/* this is a trap/syscall/irq entry */
			int				z, should_be_hex;
			char			category;
			const char		*name;
			z = fkt_find_syscall(fxt,code, &category, &name, &should_be_hex);
			fprintf(stdbug, "%c ", category);
			if( should_be_hex )
				fprintf(stdbug, "%04x", z);
			else
				fprintf(stdbug, "%4d", z);
			fprintf(stdbug, " %08lx %28s\n", code, name);
			}
		else
#endif
			fprintf(stdbug, "%13s %08lx %28s\n",
				"function", code, find_name(code,1,-1));
	/*	if( code == FKT_SWITCH_TO_CODE )
			EPRINTF("switch_to within a stack\n"); This *can* happend */
		stack->pos--; /* and do as if it was never called... */
		}

	stack->user_cycles = cyc_time-stack->start_cycle[0];

	fprintf(stdbug, "pid %4d process cycles %llu\n\n",
		thispid, stack->user_cycles);
	}

/* here we free the stack and put the process in grave */
void bury_stack( int pid, u_64 cyc_time )
	{
	struct dead_process	*grave;
	struct stack_item	*stack,*previous;
	u_64				delta;

	for( stack = stack_head, previous = NULL;
				stack != NULL;
				previous = stack, stack = stack->next )
		if ( stack->pid == pid )
			{
			fprintf(stdbug,"burying pid %d\n", pid);

			if ( stack->code[stack->pos] != SWITCH_TO_CODE)
				EPRINTF("dead pid %d has bad last code %lx\n", pid, stack->code[stack->pos]);

			/* simulate return from exit system call */
			delta = cyc_time - stack->lastreltime;
			update_stack(stack, delta, SWITCH_TO_CODE);
#ifdef FKT // TODO !!
			return_from_sys_call( stack, 1, cyc_time );
#endif

			stack->user_cycles = cyc_time-stack->start_cycle[0];

			MALLOC(grave,"grave for pid %d",pid);

			grave->pid = pid;
			grave->user_cycles = stack->user_cycles;
			strcpy(grave->name,fxt_lookup_pid(fxt,pid));
			fkt_remove_pid(fxt,pid);
			grave->next = grave_head;
			grave_head = grave;

			if (previous)
				previous->next = stack->next;
			else
				stack_head = stack->next;

			free(stack);
			return;
			}
	}

void do_stats_code( unsigned long code, u_64 cyc_time,
	int thispid, int param1, int param2, int *params )
	{
	unsigned long i;
	int		j, k;

	setup_stack(cyc_time, thispid);

/* now guaranteed that cpu_stack[lastcpu] points to stack for this pid */
	/* i gets code at top of this stack */
	i = cpu_stack[lastcpu]->code[cpu_stack[lastcpu]->pos];
#ifndef FUT
	if( code < FKT_UNSHIFTED_LIMIT_CODE)
		{
		/* this is a trap/syscall/irq entry, put it in v2 stack. */
		if( cpu_stack[lastcpu]->pos >= MAX_FUN_RECUR - 1 )
			EPRINTF("nesting stack overflow\n");
		else
			{/* push new item onto stack */
			cpu_stack[lastcpu]->pos++;
			if( actual_max_pos < cpu_stack[lastcpu]->pos )
				actual_max_pos = cpu_stack[lastcpu]->pos;
			cpu_stack[lastcpu]->start_cycle
				[cpu_stack[lastcpu]->pos] = cyc_time;
			cpu_stack[lastcpu]->code
				[cpu_stack[lastcpu]->pos] = code;
			fprintf(stdbug, "push stack for pid %d, pos %d, code %08lx, "
							"trap/syscall/irq\n",
				thispid, cpu_stack[lastcpu]->pos, code);
			}
		return;
		}
	else if( code == FKT_RET_FROM_SYS_CALL_CODE )
		{/* code for return from system calls */
		return_from_sys_call(cpu_stack[lastcpu], i, cyc_time);
		return;
		}
	else
#endif
#ifndef FUT
	if( code >= FKT_UNPAIRED_LIMIT_CODE && code < FKT_I386_FUNCTION_MINI )
#else
	if( code >= FUT_UNPAIRED_LIMIT_CODE && code < FUT_I386_FUNCTION_MINI )
#endif
		{
		switch( code )
			{
#ifndef FUT
			case FKT_END_OF_PID_CODE:
#else
			case FUT_THREAD_DEATH_CODE:
#endif
				/* this process buried a child, do it as well */
				/* total cycles count */
				bury_pid(param1);
				/* stack */
				bury_stack(param1, cyc_time);
				break;
#ifndef FUT
			case FKT_DO_FORK_CODE:
#else
			case FUT_THREAD_BIRTH_CODE:
#endif
				fkt_add_pid(fxt,param1,fxt_lookup_pid(fxt,thispid));
				break;
#ifndef FUT
			case FKT_DO_EXECVE_CODE:
#else
			case FUT_SET_THREAD_NAME_CODE:
#endif
			{
				char *name=(char *)params;
				if( fkt_add_pid(fxt,thispid,name) )
					EPRINTF("new pid execve()ing\n");
				update_pid_name(thispid,name);
				break;
			}
		}
		return; /* do nothing for unpaired entries */
		}
	else
		{/* code for the exit of a function entry/exit */
#ifndef FUT
		if( (!(code>=FKT_I386_FUNCTION_MINI) &&
				(code & FKT_GENERIC_EXIT_OFFSET) == 0  &&
				code == i + FKT_GENERIC_EXIT_OFFSET)
			||
			((code>=FKT_I386_FUNCTION_MINI) &&
			 	(code & FKT_I386_FUNCTION_EXIT) == 0 &&
				code == i - FKT_I386_FUNCTION_EXIT))
#else
		if( (!(code>=FUT_I386_FUNCTION_MINI) &&
				(code & FUT_GENERIC_EXIT_OFFSET) == 0  &&
				code == i + FUT_GENERIC_EXIT_OFFSET)
			||
			((code>=FUT_I386_FUNCTION_MINI) &&
			 	(code & FUT_I386_FUNCTION_EXIT) != 0 &&
				code == i + FUT_I386_FUNCTION_EXIT))
#endif
			{
			/* this is the exit corresponding to a previously stacked entry */
			return_from_function(cpu_stack[lastcpu], i, cyc_time);
			}
		else
			{/* this should be an entry code */
#ifndef FUT
			if( (!(code>=FKT_I386_FUNCTION_MINI) &&
					(code & FKT_GENERIC_EXIT_OFFSET) == 0)
				||
				( (code&FKT_I386_FUNCTION_MINI) &&
					(code & FKT_I386_FUNCTION_EXIT) == 0) )
#else
			if( (!(code>=FUT_I386_FUNCTION_MINI) &&
					(code & FUT_GENERIC_EXIT_OFFSET) == 0)
				||
				( (code&FUT_I386_FUNCTION_MINI) &&
					(code & FUT_I386_FUNCTION_EXIT) != 0) )
#endif
				{/* this is an exit code, not an entry code */
				/* if the stack is not empty, it means something bad happened */
				if (cpu_stack[lastcpu]->pos>0)
					EPRINTF("exiting from function code %08lx, %s,\n",
							code, find_name(code, 1, -1));
					EPRINTF("but code %08lx, %s was on the stack\n",
							i, find_name(i, 1, -1));
				/* else ignore it */
				return;
				}
			if( cpu_stack[lastcpu]->pos >= MAX_FUN_RECUR - 1 )
				EPRINTF("nesting stack overflow\n");
			else
				{/* this is a function entry, or a switch_to */
				cpu_stack[lastcpu]->pos++;
				if( actual_max_pos < cpu_stack[lastcpu]->pos )
					actual_max_pos = cpu_stack[lastcpu]->pos;
				cpu_stack[lastcpu]->start_cycle
					[cpu_stack[lastcpu]->pos] = cyc_time;
				cpu_stack[lastcpu]->code[cpu_stack[lastcpu]->pos]
					= code;
				fprintf(stdbug, "push stack for pid %d, pos %d, code %08lx, %s\n",
					thispid, cpu_stack[lastcpu]->pos, code,
					code==SWITCH_TO_CODE?"switch_to":"function");
				}
			}
		}
	if( code == SWITCH_TO_CODE )
		return;

	/*	this is a function entry or matched exit */
	/*	search the funs array to see if we have seen this code before */
	for( k = 0;  k < newfun_pos;  k++ )
		{
		if( code == funs[k].code  ||
#ifndef FUT
			(!(code>=FKT_I386_FUNCTION_MINI) && code == funs[k].code + FKT_GENERIC_EXIT_OFFSET) ||
			((code>=FKT_I386_FUNCTION_MINI) && code == funs[k].code - FKT_I386_FUNCTION_EXIT) )
#else
			(!(code>=FUT_I386_FUNCTION_MINI) && code == funs[k].code + FUT_GENERIC_EXIT_OFFSET) ||
			((code>=FUT_I386_FUNCTION_MINI) && code == funs[k].code + FUT_I386_FUNCTION_EXIT) )
#endif
			{/* funs[k] is the entry code that matches this code's entry or exit */
			if ( code == funs[k].code )
				{/* this code is the entry code */

				/*	push pointer to this code's funs array slot on fun_index */
				cpu_stack[lastcpu]->fun_index
					[cpu_stack[lastcpu]->next_fun_index]=k;
				if ( cpu_stack[lastcpu]->next_fun_index >= 1 )
					{/* update the caller stats */
					j = cpu_stack[lastcpu]->fun_index
						[cpu_stack[lastcpu]->next_fun_index-1];
					funs[j].called_funs_ctr[k]++;
					}
				cpu_stack[lastcpu]->next_fun_index++;

				/* find top of start_cycle stack in funs slot for this function */
				for ( i = 0; i < MAX_EACH_FUN_RECUR; i ++ )
					{
					if ( funs[k].start_cycle[i] == -1LL )
						break;
					}

				if( i >= MAX_EACH_FUN_RECUR )
					{
					EPRINTF("too many open calls to function %s, please increase MAX_EACH_FUN_RECUR\n",
						find_name(funs[k].code,1,-1));
					exit(EXIT_FAILURE);
					}

				if( actual_max_cycles_pos < i )
					actual_max_cycles_pos = i;
				/* time of last call to this function at top of start_cycle */
				funs[k].start_cycle[i] = cyc_time;
				funs[k].caller_pid[i] = thispid;

				/* count total number of calls to this function */
				}
			else
				/* this code is the exit corresponding to entry k in funs array */
				/* find top of start_cycle stack
				   (start time of last call to this function) */
				if( cpu_stack[lastcpu]->next_fun_index > 0 )
					exit_function(cpu_stack[lastcpu], k, thispid, cyc_time);
			return;
			}
		}

	/* if loop terminates this is first time we have seen this code */
	/* initialize and push a new slot at top of funs array */
	if( newfun_pos >= MAX_FUNS )
		{/* funs stack overflow */
		EPRINTF("too many unique functions called, max of %d\n", MAX_FUNS);
		exit(EXIT_FAILURE);
		}
	if( actual_max_funs_pos < newfun_pos )
		actual_max_funs_pos = newfun_pos;
	funs[newfun_pos].code = code;
	funs[newfun_pos].total = 0;
	funs[newfun_pos].counter = 0;
	for ( i = 0; i < MAX_EACH_FUN_RECUR; i++ )
		{
		funs[newfun_pos].start_cycle[i] = -1LL;
		funs[newfun_pos].caller_pid[i] = 0;
		}
	for( i = 0; i < MAX_FUNS; i++ )
		{
		funs[newfun_pos].called_funs_cycles[i] = 0LL;
		funs[newfun_pos].called_funs_ctr[i] = 0;
		}

#ifndef FUT
	if( (!(code >= FKT_I386_FUNCTION_MINI) && code & FKT_GENERIC_EXIT_OFFSET) ||
		( (code >= FKT_I386_FUNCTION_MINI) && code & FKT_I386_FUNCTION_EXIT) )
#else
	if( (!(code >= FUT_I386_FUNCTION_MINI) && code & FUT_GENERIC_EXIT_OFFSET) ||
		( (code >= FUT_I386_FUNCTION_MINI) && !(code & FUT_I386_FUNCTION_EXIT)) )
#endif
		{/* if this is an entry */
		/* also push pointer to this called function on fun_index */
		cpu_stack[lastcpu]->fun_index
				[cpu_stack[lastcpu]->next_fun_index] = newfun_pos;
		if ( cpu_stack[lastcpu]->next_fun_index >= 1 )
			{
			j = cpu_stack[lastcpu]->fun_index
				[cpu_stack[lastcpu]->next_fun_index - 1];
			if (funs[j].called_funs_ctr[k] == 0 )
				funs[j].called_funs_ctr[k]++;
			}
		cpu_stack[lastcpu]->next_fun_index++;

		/* save pid of caller and time of this call to this function */
		funs[newfun_pos].start_cycle[0] = cyc_time;
		funs[newfun_pos].caller_pid[0] = thispid;
		}
	newfun_pos++;
	}

/*	make sure this cpu is in the correct statistics taking state */
void adjust_cpu_state( unsigned int thiscpu, u_64 reltime )
	{
	if( fun_time != fun_time_cpu[thiscpu] )
		{/* first time this cpu is active since a stat-taking state change */
		if( fun_time_cpu[thiscpu] == 0 )
			{/* this cpu not active yet, must get a start time for it */
			start_time_cpu[thiscpu] = reltime;
			fprintf(stdbug, "cpu %u start time %llu\n", thiscpu, reltime);
			}
		if( fun_time < 0 )
			{/* this cpu still active but stat-taking has stopped */
			end_time_cpu[thiscpu] = reltime;
			fprintf(stdbug, "cpu %u end time %llu\n", thiscpu, reltime);
			}
		/*	force cpu state to be the same as the global state */
		fun_time_cpu[thiscpu] = fun_time;
		}
	}

/* check coherency between accounted, processes cycles and cpu cycles */
void check_coherency(void)
	{
	int i;
	long long total_accounted;
	struct stack_item *stack;

	if (fun_time<1) return;

	total_accounted=accounted;
	for ( i = 0; i < info->ncpus; i++ )
		{
		if (fun_time_cpu[i] == 1)
			total_accounted -= lastreltime[i]-start_time_cpu[i];
		}
	for ( stack = stack_head; stack; stack = stack->next )
		{
		if (stack->user_cycles)
			total_accounted += stack->user_cycles;
		else
			total_accounted += stack->lastreltime-stack->start_cycle[0];
		}
	if (total_accounted != 0)
		EPRINTF("incoherent computing: %lld cycles\n",total_accounted);
	}

static void error_format(const char* str)
{
	fprintf(stderr, "Format %i (%s) not yet supported (%s)\n",
					trace_format, trace_format_name[trace_format], str);
	exit(EXIT_FAILURE);
}

static int getcpu_from_pid(unsigned int pid)
{
		int cpu;
		for(cpu=0; cpu<info->ncpus; cpu++) {
				if (already_saw[cpu] && lastpid[cpu]==pid) {
						return cpu;
				}
		}
		return 0;
}

/*
bufptr[0] is the first item for each line ie cycles
bufptr[1] is pid in low half, processor number in high half
bufptr[2] is code
bufptr[3] is P1
bufptr[4] is P2
...
bufptr[n] is P(n-2)
*/

/*
 *
 * dump one slot
 *
 */

unsigned int *dumpslot( unsigned int *bufptr )
	{
	unsigned long	code, fullcode;
	unsigned int	params, i, thiscpu;
	u_64			reltime, r, x;
	int				thispid, print_this = printing;

	fprintf(stdbug,"\n");
#if 0
	printf("value: %16llx %8x %8x (%i, %i) %8x %8x %8x %8x %8x\n",
				*(u_64*)bufptr, bufptr[2], 
				bufptr[3], bufptr[3]>>8, bufptr[3]&0xff,
				bufptr[4], bufptr[5], bufptr[6], bufptr[7], bufptr[8]); 
#endif
	
	switch (trace_format) {
		case FKT_FORMAT:
			thiscpu = bufptr[1] >> 16;
			break;
		case FUT_FORMAT_MONO:
			thiscpu = 0;
			break;
		case FUT_FORMAT_SMP:
			if (bufptr[3]>>8 == FUT_NEW_LWP_CODE) {
					thiscpu = bufptr[5];
			} else {
				thiscpu = getcpu_from_pid(bufptr[2]);
			}
			break;
		default:
			error_format("thiscpu");
	}

	if (!basetime)
		{
		switch (trace_format) {
			case FKT_FORMAT:
				basetime=(u_64)bufptr[0];
				break;
			case FUT_FORMAT_MONO:
			case FUT_FORMAT_SMP:
				basetime=*(u_64*)bufptr;
				break;
			default:
				error_format("basetime");
		}
		fprintf(stdout, "initial: basetime %llu\n", basetime);
		}
	if( thiscpu >= info->ncpus )
		EPRINTF("thiscpu = %u, limit is %u\n", thiscpu, info->ncpus);
	else if( !already_saw[thiscpu] )
		{/* this is first time we have seen this cpu */
		switch (trace_format) {
			case FKT_FORMAT:
				lasttime[thiscpu] = (u_64)bufptr[0];
				break;
			case FUT_FORMAT_MONO:
			case FUT_FORMAT_SMP:
				lasttime[thiscpu] = *(u_64*)bufptr;
				break;
			default:
				error_format("lasttime");
		}
		already_saw[thiscpu] = 1;	/* mark that we have seen this cpu*/
		lastcpu = thiscpu;
		}
	else if( lastcpu != thiscpu )
		{/* changing cpus */
		fprintf(stdbug, "changing from cpu %u to cpu %u\n", lastcpu, thiscpu);
		lastcpu = thiscpu;
		}

	switch (trace_format) {
		case FKT_FORMAT:
			x = ((u_64)bufptr[0])+basehigh[thiscpu];/* cnv 32-bit cycles to 64-bit */
			if( x < lasttime[thiscpu] )
				{
				fprintf(stdbug,"cycle overflow: x %llu, lasttime %llu, basehigh %llu\n",
					x, lasttime[thiscpu], basehigh[thiscpu]);
				basehigh[thiscpu]+= 0x100000000LL;	/* add 1 to high 32-bits */
				x += 0x100000000LL;	/* add 1 to high 32-bits */
				fprintf(stdbug,"after overflow: x %llu, lasttime %llu, basehigh %llu\n",
					x, lasttime[thiscpu], basehigh[thiscpu]);
				}
			break;
		case FUT_FORMAT_MONO:
		case FUT_FORMAT_SMP:
			x = (*(u_64*)bufptr); /* read 64 bits value */
			break;
		default:
			 error_format("time");
	}
	lasttime[thiscpu] = x;
	reltime = x - basetime;
	r = reltime-lastreltime[thiscpu];
	switch (trace_format) {
		case FKT_FORMAT:
			thispid = bufptr[1]&0xffff;
			if( !thispid )	/* an idle process, give it the cpu number */
				thispid=-thiscpu;
			break;
		case FUT_FORMAT_MONO:
			thispid = lastpid[thiscpu];
			break;
		case FUT_FORMAT_SMP:
			thispid = bufptr[2];
			bufptr++;
			bufptr[2]-=4;
			break;
		default:
			 error_format("pid");
	}
	fullcode = bufptr[2];
	code = fullcode >> 8;
	if( code == CALIBRATE0_CODE  ||
		code == CALIBRATE1_CODE  ||
		code == CALIBRATE2_CODE )
		print_this = 1;

	if( !printing && (printpid == -1 || (printpid > 0 && thispid == printpid)) )
		{
		print_this = 1;
		if( !printonlypid )
			printing = 1;
		}

	fprintf(stdbug,
	"dumpslot: thiscpu %u, lastpid %d(%s), thispid %d(%s), code 0x%04lx, delta %llu\n",
		thiscpu, lastpid[thiscpu], fxt_lookup_pid(fxt,lastpid[thiscpu]),
		thispid, fxt_lookup_pid(fxt,thispid), fullcode, r);

	if( fun_time == 0 && (statspid == -1 || (statspid > 0 && thispid == statspid)) )
		start_taking_stats();		/* start taking stats */

	if( fun_time_cpu[thiscpu] > 0 )
		update_pid_cycles(r, thispid);

	/*	make sure this cpu is in the correct statistics taking state */
	adjust_cpu_state(thiscpu, reltime);

	if( print_this )
		switch (trace_format) {
			case FKT_FORMAT:
				printf( "%8llu "CYCNUM" %2u %5d"COMMSTR, r, reltime, 
								thiscpu, thispid, fxt_lookup_pid(fxt,thispid));
				break;
			case FUT_FORMAT_MONO:
			case FUT_FORMAT_SMP:
				printf( "%8llu "CYCNUM" %2u 0x%.08x"COMMSTR, r, reltime, 
								thiscpu, thispid, fxt_lookup_pid(fxt,thispid));
				break;
			default:
				error_format("printf");
		}

#ifndef FUT
	if( fullcode < FKT_UNSHIFTED_LIMIT_CODE )
		{/* unshifted code, no parameters */
		char			workspace[128];
		code = fullcode;
		if( fullcode <= FKT_SYS_CALL_MASK )
			if ( fullcode == FKT_LCALL7 || fullcode == FKT_LCALL27 )
				{
				i = (fullcode==FKT_LCALL7)?7:0x27;
				sprintf(workspace, "lcall%x",i);
				if( print_this )
					printf(" "NAMESTR"  %-s", workspace, workspace);
				}
			else
				{
				i = fullcode;
				sprintf(workspace, "system call %u", i);
				if( i > FKT_I386_NSYSCALLS )
					i = FKT_I386_NSYSCALLS;
				if( print_this )
					printf(" "NAMESTR"  %-s", workspace, fkt_i386_syscalls[i]);
				}
		else if( fullcode < FKT_TRAP_LIMIT_CODE )
			{
			i = fullcode - FKT_TRAP_BASE;
			sprintf(workspace, "trap %u", i);
			if( i > FKT_I386_NTRAPS )
				i = FKT_I386_NTRAPS;
			if( print_this )
				printf(" "NAMESTR"  %-s", workspace, fkt_i386_traps[i]);
			}
		else if( fullcode < FKT_IRQ_SYS )
			{
			i = fullcode - FKT_IRQ_TIMER;
			sprintf(workspace, "IRQ %u", i);
			if( print_this )
				printf(" "NAMESTR"  %-s", workspace, fkt_find_irq(fxt,i));
			}
		else
			{
			i = fullcode - FKT_IRQ_SYS;
			sprintf(workspace, "sysIRQ 0x%x", i+0xef);
			if( print_this )
				printf(" "NAMESTR"  %-s", workspace, fkt_i386_sysirqs[i]);
			}
		params = 3;
		}
	else
#endif
	if( code == GCC_INSTRUMENT_ENTRY_CODE ||
			 code == GCC_INSTRUMENT_EXIT_CODE)
		{
		const char *s;
		params = fullcode & 0xff;
		if( params & 0x03 )
			{
			EPRINTF( "header size not multiple of 4: %08lx, %04lx, %d\n",
				fullcode, code, params);
			exit(EXIT_FAILURE);
			}
		params = (params>>2);
		if( print_this )
			printf(" "NAMESTR, code==GCC_INSTRUMENT_ENTRY_CODE?
						"gcc-traced function entry":"gcc-traced function exit");
#ifndef FUT
		code=bufptr[3]-(code==GCC_INSTRUMENT_ENTRY_CODE?0:GCC_TRACED_FUNCTION_X86_EXIT);
		if( print_this && (s=fxt_lookup_symbol(fxt,code|GCC_TRACED_FUNCTION_X86_EXIT)) )
#else
		code=bufptr[3]+(code==GCC_INSTRUMENT_ENTRY_CODE?0:GCC_TRACED_FUNCTION_X86_EXIT);
		if( print_this && (s=fxt_lookup_symbol(fxt,code&(~(GCC_TRACED_FUNCTION_X86_EXIT)))) )
#endif
			{
			printf("  %s",s);
			bufptr++;
			params=3;
			}
		}
	else {/* shifted code or parameter */
		params = fullcode & 0xff;
		if( params & 0x03 )
			{
			EPRINTF( "header size not multiple of 4: %08lx, %04lx, %d\n",
				fullcode, code, params);
			exit(EXIT_FAILURE);
			}
		params = (params>>2);
		if( print_this )
			printf(" "NAMESTR, find_name(code, 1, NAMELEN));
		}
	if( print_this )
		{
#ifdef FKT // TODO
		if( code == FKT_DO_EXECVE_CODE || code == FKT_OPEN_ENTRY_CODE )
			{
				char *comm = (char *)(bufptr+3);
				comm[(params-3)*4-1]='\0';
				printf("  %s",comm);
			}
		else
		if( (code == SWITCH_TO_CODE) && !bufptr[3])
		{
				bufptr[3]= -thiscpu; /* Idle */
		}
#endif
		for( i = 3;  i < params;  i++ )
			{
			if( i == 3 )
				printf("  ");		/* first time only */
			else
				printf(", ");		/* not the first time */
			if( bufptr[i] > 0xffffff00 )	/* -256 */
				printf("%d", (int)bufptr[i]);
			else if( bufptr[i] <= 0xffffff )
				printf("%u", bufptr[i]);
			else
				printf("%#08x", bufptr[i]);
			}
		printf("\n");
		}

	page_stats(code,params >=7 ? bufptr[6] : 0);

	if( fun_time > 0 )
		do_stats_code(code, reltime, thispid,
			params >=4 ? bufptr[3] : 0,
			params >=5 ? bufptr[4] : 0,
			bufptr+3);

	lastreltime[thiscpu] = reltime;

#ifdef FUT
	if( code == SWITCH_TO_CODE )
		{
		//if( thispid != bufptr[3] )
		//	EPRINTF("switch_to from wrong pid\n");
		thispid = bufptr[3];
		}
#endif

	lastpid[thiscpu] = thispid;

/* uncomment this if you have incoherencies */
#if 0
	check_coherency();
#endif

	return &bufptr[params];
	}

/*
 *
 * Dump the file
 *
 */

/*	returns 0 if ok, else -1 */
int dumpit( int fd, unsigned long *npages )
	{
	unsigned int		n;
	unsigned int		*buffer, *bufptr, *pagelimit=NULL;
	struct stack_item	*stack;
	int					didread;
	int					shouldeof=0;

	if( (buffer = (unsigned int *)mmap(NULL, page_size,
			PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, 0, 0))
				== MAP_FAILED
		&& !(buffer = malloc(page_size)) )
		{
		fprintf(stderr, "unable to get memory\n");
		exit(EXIT_FAILURE);
		}

	if( printpid )
		{
		printf("%8s "  CYCSTR  " %3s %4s"   COMMSTR" %28s",
			"Delta", "Cycles", "Cpu","Pid", COMMNAME, "Name");
		printf("  P1, P2, P3, ...\n");
		printf("%8s "  CYCSTR  " %3s %4s"   COMMSTR" %28s",
			"------", "-----", "---","---", COMMUNDERLINE, "----");
		printf("  ---------------\n");
		}

	for( bufptr = buffer, n = 0;  n < *npages; )
		{
		if ( bufptr == buffer )
			{
			if( (didread = read(fd,buffer,page_size)) < 0 )
				{
				if( errno == EINTR ) continue;
				perror("read one page");
				exit(EXIT_FAILURE);
				}
			if( !didread )
				{/* premature end of file, no hurt */
				*npages=n;
				break;
				}
			else if( shouldeof )
				{
				EPRINTF("End of file expected\n");
				exit(EXIT_FAILURE);
				}
			if( didread < page_size )
				shouldeof=1;
			if( !*bufptr )
				{
				EPRINTF("null size !\n");
				pagelimit=bufptr+page_size/sizeof(unsigned int);
				bufptr++;
				}
#ifdef FKT
			else if( *bufptr == FKT_POISON )
				{
				/* just for debugging */
				EPRINTF("poisoned size !\n");
				pagelimit=bufptr+page_size/sizeof(unsigned int);
				bufptr++;
				}
			else if( *bufptr == FKT_EOF )
				{/* end of record */
				*npages=n;
				break;
				}
#endif
			else if( *bufptr >0x80000000 )
				{
				EPRINTF("big page size ! %#010x\n",*bufptr);
				exit(EXIT_FAILURE);
				}
			else
				{
				/* beginning of a page, record its fill length */
				if( *bufptr > didread )
					pagelimit = (unsigned int *) ((void *) bufptr + didread);
				else
					pagelimit = (unsigned int *) ((void *) bufptr + *bufptr);
				if( pagelimit>bufptr+page_size/sizeof(unsigned int) )
					{
					EPRINTF("size too big: %x\n",*bufptr);
					exit(EXIT_FAILURE);
					}
				bufptr++;
				}
		}
		bufptr = dumpslot(bufptr);
		if ( bufptr >= pagelimit ) {
			if (bufptr!=pagelimit)
				EPRINTF("page resync here: %p > %p\n",bufptr,pagelimit);

			/* end of a page, jump to the next one */
			n++;
			bufptr = buffer;
			current_page_status->time=lastreltime[0];
			if( current_page_status->nbsyncs )
					current_page_status->nbsyncs = current_page_status->written;
			*(current_page_status+1)=*current_page_status;
			current_page_status++;
			current_page_status->nbsyncs=0;
			fprintf(stdbug,"switch to page %d\n", n);
			fprintf(stderr,"\r%3.1f%%...", 100.*(((double)n)/((double)*npages)));
		}
	}

	if( fun_time > 0 )	/* never stopped stat taking */
		stop_taking_stats();

	/* end times on all cpus */
	for( n = 0;  n < info->ncpus;  n++ )
		adjust_cpu_state(n, lastreltime[n]);

	/* now close up all stacks */
	for(stack = stack_head; stack != NULL; stack = stack->next )
		close_stack(stack,stack->pid,lastreltime[stack->cpu]);

	printf("\n");
	return 0;
	}

/*
 *
 * Results printing functions
 *
 */

void draw_histogram( int n, u_64 histo_sum, u_64 histo_max, char **histo_name,
							u_64 *histo_cycles)
	{
	int		i, k, length;
	u_64	sum;

	printf("\n");
	printf("%28s %14s %7s\n", "Name", "Cycles", "Percent");
	printf("%28s %14s %7s\n", "----", "------", "-------");
	sum = 0LL;
	for( k = 0;  k < n;  k++ )
		{
		if (strlen(histo_name[k])>NAMELEN)
			histo_name[k][NAMELEN]='\0';
		printf(NAMESTR" "CYCNUM" %6.2f%% ",
			histo_name[k], histo_cycles[k],
				100.*((double)histo_cycles[k])/((double)histo_sum));
		length = (histo_cycles[k]*MAX_HISTO_BAR_LENGTH)/histo_max;
		sum += histo_cycles[k];
		for( i = 0;  i < length;  i++ )
			putchar(HISTO_CHAR);
		putchar('\n');
		}
	printf(NAMESTR" "CYCNUM" %6.2f%%\n\n", "Total elapsed cycles",
								sum, 100.*((double)sum)/((double)histo_sum));
	}


void print_stats(u_64 CycPerJiffy)
	{
	int				i, z, k;
	u_64			sum_other_cycles = 0LL;
	u_64			timediff, totaltimetaken, totalcounted, sum;
	/*****
	double			Timems;
	*****/
	double			Main_fun_sum;
	double			called_fun_percent, only_fun_percent;
	struct stack_item	*stack;
	struct dead_process	*grave;
	u_64			histo_max;
	u_64			histo_sum;
	char			category, commname[FXT_MAXCOMMLEN+2+5+1];
	const char		*name;
	char			*histo_name[HISTO_SIZE];
	u_64			histo_cycles[HISTO_SIZE];

	printf("%44s\n\n","CPU CYCLES");
	my_print_line('*');
	printf("\n");

	printf("%30s ", "");
	for( i = 0;  i < info->ncpus;  i++ )
		printf("%11s %2d", "cpu", i);
	printf("\n");

	printf ("%30s:", "Cycles at start of stat taking");
	for( i = 0;  i < info->ncpus;  i++ )
		printf(CYCNUM, start_time_cpu[i]);
	printf("\n");

	printf ("%30s:", "Cycles at end of stat taking");
	for( i = 0;  i < info->ncpus;  i++ )
		printf(CYCNUM, end_time_cpu[i]);
	printf("\n");

	totaltimetaken = 0ll;
	printf ("%30s:", "Number of elapsed stat cycles");
	for( i = 0;  i < info->ncpus;  i++ )
		{
		timediff = end_time_cpu[i] - start_time_cpu[i];
		totaltimetaken += timediff;
		printf(CYCNUM, timediff);
		}

	printf("\n\n");
	printf ("%30s:"CYCNUM"\n", "Total elapsed stat cycles", totaltimetaken);

	/*****
	printf ("%36s : %10llu\n", "Cycles Per Jiffy", CycPerJiffy);
	Timems = ((double)TimeDiff) * 10.0 / ((double)CycPerJiffy);
	printf ("%36s : %10.2fms\n", "Time taken is", Timems);
	*****/

	print_cycles(CycPerJiffy);

	printf("\n\n%48s\n\n","FUNCTIONS CYCLES");
	my_print_line('*');
	printf("\n");

/*	print table of all function names */
if( newfun_pos > 0 )
	{
	printf(NAMESTR" "CYCSTR" "COUNTSTR" "AVGSTR"\n",
			"Name", "Cycles", "Count", "Average");
	printf(NAMESTR" "CYCSTR" "COUNTSTR" "AVGSTR"\n",
			"----", "------", "-----", "-------");
	for ( k = 0;  k < newfun_pos;  k++ )
		{
		if( funs[k].code > 0 )
			{
			printf(NAMESTR" "CYCNUM" "COUNTNUM,
				   find_name(funs[k].code,0,NAMELEN), funs[k].total, funs[k].counter);
			if( funs[k].counter > 0 )
				printf(" "AVGNUM, (double)funs[k].total/funs[k].counter);
			printf("\n");
			}
		}

	/*
	duplicate
	printf("\n\n");
	printf("%4s %25s "CYCSTR" "COUNTSTR"\n",
			"From", "To", "Cycles", "Count");
	printf("%4s %25s "CYCSTR" "COUNTSTR"\n",
			"----", "--", "------", "-----");
	for ( k = 0;  k < newfun_pos;  k++ )
		{
		if( funs[k].code > 0 )
			{
			printf("%s\n", find_name(funs[k].code,0));
			for ( z = 0;  z < newfun_pos;  z++ )
				{
				if ( funs[k].called_funs_ctr[z] != 0 )
					{
					printf(NAMESTR" %04x "CYCNUM" "COUNTNUM"\n",
						find_name(funs[z].code,0,NAMELEN),
						funs[k].called_funs_cycles[z],
						funs[k].called_funs_ctr[z]);
					}
				}
			printf("\n");
			}
		}
	*/

	printf("\n\n");
	printf("%50s\n\n", "NESTING SUMMARY");
	my_print_line('*');
	printf("\n");
	for ( k = 0;  k < newfun_pos;  k++ )
		{
		if( funs[k].code > 0 )
			{
			printf(NAMESTR" "CYCSTR" "COUNTSTR" "AVGSTR" %7s\n",
					"Name", "Cycles", "Count", "Average", "Percent");
			printf(NAMESTR" "CYCSTR" "COUNTSTR" "AVGSTR" %7s\n",
					"----", "------", "-----", "-------", "-------");
			Main_fun_sum = (double)funs[k].total;

			for ( z = 0;  z < newfun_pos;  z++ )
				{
				if( funs[z].code > 0  &&  funs[k].called_funs_ctr[z] != 0 )
					{
					called_fun_percent = (double)funs[k].called_funs_cycles[z] /
						Main_fun_sum * 100.0;
					printf(NAMESTR" "CYCNUM" "COUNTNUM" "AVGNUM" %6.2f%%\n",
						find_name(funs[z].code,0,NAMELEN),
						funs[k].called_funs_cycles[z],
						funs[k].called_funs_ctr[z],
						(double) funs[k].called_funs_cycles[z] /
						funs[k].called_funs_ctr[z],
						called_fun_percent);
					sum_other_cycles += funs[k].called_funs_cycles[z];
					}
				}

			printf(NAMESTRONLY" ONLY ", find_name(funs[k].code,0,NAMELEN));

			if( sum_other_cycles > funs[k].total )
				printf(CYCNUM" ** sum of cycles too big compared to %llu **\n\n",sum_other_cycles,funs[k].total);
			else
				{
				only_fun_percent = (double)(funs[k].total - sum_other_cycles) /
														Main_fun_sum * 100.0;
				printf(CYCNUM" "COUNTNUM" "AVGNUM" %6.2f%%\n\n",
					funs[k].total - sum_other_cycles,
					funs[k].counter,
					(double)(funs[k].total-sum_other_cycles)/funs[k].counter,
					only_fun_percent);
				}

			printf(NAMESTR" "CYCNUM" "COUNTNUM" "AVGNUM" %6.2f%%\n",
   				find_name(funs[k].code,0,NAMELEN), funs[k].total,
   				funs[k].counter, (double)funs[k].total/funs[k].counter,
				100.0);
			sum_other_cycles = 0LL;
			printf("\n");
			my_print_line('-');
			printf("\n");
			}
		}
	}

	printf("\n\n");
	/*****
	printf(NAMESTR": "CYCNUM" cycles %9.3f ms %6.2f%%\n", "Total time taken",
		TimeDiff, Timems, 100.0);
	*****/

	/* make sure every stack was closed */
	for( stack = stack_head;  stack != NULL;  stack = stack->next )
		{
		if( stack->pos > 0 )
			EPRINTF("stack for pid %d still has pos %d\n",
				stack->pid, stack->pos);
		}

	totalcounted = totaltimetaken;
	/* substract user time if wanted */
	if (kernel_only)
		{
		for( stack = stack_head; stack; stack = stack->next )
			totalcounted -= stack->user_cycles;
		for( grave = grave_head; grave; grave = grave->next )
			totalcounted -= grave->user_cycles;
		printf (NAMESTR": "CYCNUM"\n", "Total kernel stat cycles", totalcounted);
		}

	/* now print out the precise accounting statistics from the v3 arrays */
	sum = 0LL;
	k = 0;
	histo_max = 1LL;
	histo_sum = 0LL;
	printf("\n\n\n");
	printf("%60s\n\n", "Accounting for every cycle exactly once");
	my_print_line('*');
	printf("\n%c "NAMESTR" "CYCSTR" "COUNTSTR" "AVGSTR" %7s\n",
			' ', "Name", "Cycles", "Count", "Average", "Percent");
	printf(  "%c "NAMESTR" "CYCSTR" "COUNTSTR" "AVGSTR" %7s\n",
			' ', "----", "------", "-----", "-------", "-------");
#ifdef FKT
	for( i = 0;  i < FKT_UNSHIFTED_LIMIT_CODE; i++ )
		{
		if( syscall_counter[i] > 0 )
			{/* this occurred at least once, print it */
			int	should_be_hex;

			z = fkt_find_syscall(fxt, i, &category, &name, &should_be_hex);
			if( (histo_name[k] = malloc(strlen(name)+10)) == NULL )
				{
				fprintf(stderr, "no space to malloc histo_name\n");
				exit(EXIT_FAILURE);
				}
			histo_cycles[k] = syscall_cycles[i];
			sum += histo_cycles[k];
			histo_sum += histo_cycles[k];
			if( histo_max < histo_cycles[k] )
				histo_max = histo_cycles[k];
			printf("%c "NAMESTR" ", category, name);
			strcpy(histo_name[k],name);
			printf(CYCNUM" "COUNTNUM" "AVGNUM" %6.2f%%\n",
				histo_cycles[k], syscall_counter[i],
				((double)histo_cycles[k])/((double)syscall_counter[i]),
				100.*((double)histo_cycles[k])/((double)totaltimetaken));
			if (++k>=HISTO_SIZE)
				{
				EPRINTF("please increase HISTO_SIZE\n");
				return;
				}
			}
		}
#endif
	for( i = 0;  i < newfun_pos; i++ )
		{
		z = funs[i].code;

		name = find_name(z,0,NAMELEN);
		if( (histo_name[k] = malloc(strlen(name)+10)) == NULL )
			{
			fprintf(stderr, "no space to malloc histo_name\n");
			exit(EXIT_FAILURE);
			}
		strcpy(histo_name[k],name);
		histo_cycles[k] = funs[i].self_total;
		sum += histo_cycles[k];
		histo_sum += histo_cycles[k];
		if( histo_max < histo_cycles[k] )
			histo_max = histo_cycles[k];
		printf("%c "NAMESTR" "CYCNUM" "COUNTNUM" "AVGNUM" %6.2f%%\n",
			'F', name, histo_cycles[k], funs[i].counter,
			((double)histo_cycles[k])/((double)funs[i].counter),
			100.0*((double)histo_cycles[k])/(double)totaltimetaken);
		if (++k>=HISTO_SIZE)
			{
			EPRINTF("please increase HISTO_SIZE\n");
			return;
			}
		}


	/*	now print out process information from stacks */
	for( stack = stack_head;  stack != NULL;  stack = stack->next )
		{
		find_category( (z=stack->pid), &category, &name );

		sum += stack->user_cycles;

		if( kernel_only )
			histo_name[k]=commname;
		else
			{
			if( (histo_name[k] = malloc(FXT_MAXCOMMLEN+2+5+1)) == NULL )
				{
				fprintf(stderr, "no space to malloc histo_name\n");
				exit(EXIT_FAILURE);
				}
			histo_cycles[k] = stack->user_cycles;
			histo_sum += histo_cycles[k];
			if( histo_max < histo_cycles[k] )
				histo_max = histo_cycles[k];
			}

		sprintf(histo_name[k], "%s(%d)",
				name,
				stack->pid);

		printf("%c "NAMESTR" "CYCNUM" "COUNTSTR" "AVGSTR" %6.2f%%\n",
			category, histo_name[k], stack->user_cycles, "", "",
			100.0*((double)stack->user_cycles)/(double)totaltimetaken);
		if (!kernel_only && ++k>=HISTO_SIZE)
			{
			EPRINTF("please increase HISTO_SIZE\n");
			return;
			}
		}

	for( grave = grave_head; grave; grave = grave->next )
		{
		sum += grave->user_cycles;
		if( kernel_only )
			histo_name[k]=commname;
		else
			{
			if( (histo_name[k] = malloc(FXT_MAXCOMMLEN+2+5+1)) == NULL )
				{
				fprintf(stderr, "no space to malloc histo_name\n");
				exit(EXIT_FAILURE);
				}
			histo_cycles[k] = grave->user_cycles;
			histo_sum += grave->user_cycles;

			if( histo_max < histo_cycles[k] )
				histo_max = histo_cycles[k];
			}
		sprintf(histo_name[k], "%s(%d)", grave->name, grave->pid);
		printf("%c "NAMESTR" "CYCNUM" "COUNTSTR" "AVGSTR" %6.2f%%\n",
 			'o', histo_name[k], grave->user_cycles, "", "",
			100.0*((double)grave->user_cycles)/(double)totaltimetaken);
		if( !kernel_only && ++k>=HISTO_SIZE)
			{
			EPRINTF("please increase HISTO_SIZE\n");
			return;
			}
		}

	printf("%c "NAMESTR" "CYCNUM" "COUNTSTR" "AVGSTR" %6.2f%%\n", 
		' ', "Total elapsed cycles", sum, "", "",
		100.0*((double)sum)/(double)totaltimetaken);
	
	if (sum!=totaltimetaken)
			EPRINTF("Final total not coherent\n");

	printf("\n\n%65s\n", "Histogram accounting for every cycle exactly once");
	if (kernel_only)
		printf("%64s\n", "excluding all user processes and idle processes");
	printf("\n");
	my_print_line('*');
	draw_histogram(k, histo_sum, histo_max, histo_name, histo_cycles);
	}



void get_the_time( time_t the_time, char *message )
	{
	struct tm	breakout;
	char		buffer[128];


	if( localtime_r(&the_time, &breakout) == NULL )
		{
		fprintf(stderr, "Unable to break out %s\n", message);
		exit(EXIT_FAILURE);
		}
	if( strftime(buffer, 128, "%d-%b-%Y %H:%M:%S", &breakout) == 0 )
		{
		fprintf(stderr, "Unable to convert %s\n", message);
		exit(EXIT_FAILURE);
		}
	printf("%14s = %s\n", message, buffer);
	}

#define OPTIONS ":f:p:s:dqhkb:t:"
#ifndef WHITE_SPACE
#define WHITE_SPACE " \t\n\f\v\r"
#endif

void usage(char *progname)
{
	printf("Usage: %s [-f record-file] [-p pid] [-s pid] [-c] [-d] [-k] [-q] [-b record.bufdat] [ -t record.timedat ]\n\
\n\
	if -f is not given, look for TRACE_FILE environment variable, and\n\
		if that is not given, read from \"" DEFAULT_TRACE_FILE "\".\n\
\n\
	if -p is given, printing starts with first item belonging to the given\n\
		pid. If the pid starts with +, printing will be done only for that\n\
		pid. If the pid is 0, no printing will be done. If the pid is -1,\n\
		printing will be always done (default).\n\
\n\
	if -s is given, statistics will accumulute between the first entry\n\
		belonging to the given pid, until its death. If the pid is 0, no\n\
		statistics will be done (but -b and -t options will still work).\n\
		If the pid is -1, statistics will be always done (default).\n\
\n\
	if -d is given, turn on debug printout to stdout for stack nesting\n\
		if that is not given, no debug printout is printed\n\
\n\
	if -k is given, percentages will be relative to the kernel time.\n\
		user time won't be taken into account\n\
\n\
	if -b is given, statistics about the page buffer will be printed in\n\
		the file, easily printable with gnuplot\n\
\n\
	if -t is given, statistics about the time each page took to be filled will\n\
		be printed in the file, easily printable with gnuplot\n\
", progname);
}

int main( int argc, char *argv[] )
{
	char			*infile,*ptr;
	int				c;
	unsigned long	npages=0;
	struct stat		st;
	off_t			off=0;
	size_t			size;
	
	u_64			ratio;
	
	kidpid = 0;
	infile = NULL;
	stdbug = NULL;
	opterr = 0;

	while( (c = getopt(argc, argv, OPTIONS)) != EOF )
		switch( c )
		{
		case 's':
			statspid = strtol(optarg, &ptr, 0);
			if( strspn(ptr,	WHITE_SPACE) != strlen(ptr) )
			{
				fprintf(stderr, "illegal pid parameter %s\n", optarg);
				exit(EXIT_FAILURE);
			}
			break;
		case 'p':
			printpid = strtol(optarg, &ptr, 0);
			if( strspn(ptr,	WHITE_SPACE) != strlen(ptr) )
			{
				fprintf(stderr, "illegal pid parameter %s\n", optarg);
				exit(EXIT_FAILURE);
			}
			printonlypid = optarg[0]=='+';
			break;
		case 'd':
			stdbug = stdout;
			break;
		case 'f':
			infile = optarg;
			break;
		case 'k':
			kernel_only=1;
			break;
		case 'b':
			pagebufstats = optarg;
			break;
		case 't':
			pagetimestats = optarg;
			break;
		case 'h':
			usage(argv[0]);
			exit(EXIT_SUCCESS);
		case ':':
			fprintf(stderr, "missing parameter to switch %c\n", optopt);
			usage(argv[0]);
			exit(EXIT_FAILURE);
		case '?':
			fprintf(stderr, "illegal switch %c\n", optopt);
			usage(argv[0]);
			exit(EXIT_FAILURE);
		}	/* switch */
	
	if( optind < argc )
		fprintf(stderr, "extra command line arguments ignored\n");
	
	if( stdbug == NULL )
	{
		if( (stdbug = fopen("/dev/null", "w")) == NULL )
		{
			perror("/dev/null");
			stdbug = stdout;
		}
	}
	
	if( infile == NULL )
	{
		if( (infile = getenv("TRACE_FILE")) == NULL )
			infile = DEFAULT_TRACE_FILE;
	}
	
	while( (fd = open(infile, O_RDONLY)) < 0 )
	{/* could not open the indicated file name for reading */
		perror(infile);
		if( strcmp(infile, DEFAULT_TRACE_FILE) == 0 )
			/* can't open the default file, have to give up */
			exit(EXIT_FAILURE);
		infile = DEFAULT_TRACE_FILE;
		fprintf(stderr, "input defaulting to %s\n", infile);
	}
	
	if (!(fxt = fxt_fdopen(fd)))
		{
		perror("fxt_fdopen");
		exit(EXIT_FAILURE);
		}
	info = fxt_infos(fxt);

	printf("%14s = %i\n", "ncpus", info->ncpus);
	
#ifdef FKT
	CALLOC(basehigh,info->ncpus);
#endif
	CALLOC(lasttime,info->ncpus);
	CALLOC(lastreltime,info->ncpus);
	CALLOC(start_time_cpu,info->ncpus);
	CALLOC(end_time_cpu,info->ncpus);
	CALLOC(lastpid,info->ncpus);
	CALLOC(already_saw,info->ncpus);
	CALLOC(fun_time_cpu,info->ncpus);
	CALLOC(cpu_stack,info->ncpus);
	
	pid = info->record_pid;
	kidpid = info->traced_pid;
	
	get_the_time(info->start_time, "start time");
	get_the_time(info->stop_time, "stop time");

	printf("%14s = %lu\n", "stop jiffies", (unsigned long)info->stop_jiffies);
	printf("%14s = %lu\n", "total jiffies", (unsigned long)info->stop_jiffies -
	       (unsigned long)info->start_jiffies);
	
	page_size = info->page_size;
	printf("%14s = %u\n", "page size", page_size);
	trace_format = info->format;
	printf("%14s = %u (%s)\n", "trace format", trace_format, trace_format_name[trace_format]);
	printf(info->uname);
	
	if (fstat(fd, &st)) {
		perror("fstat");
		exit(EXIT_FAILURE);
	}
	
	if( S_ISBLK(st.st_mode) )
	{
		int sectors;
		if( ioctl(fd,BLKGETSIZE,&sectors) )
		{
			perror("getting device size\n");
			exit(EXIT_FAILURE);
		}
		if( sectors >= 0xffffffffUL >> 9 )
			size=0xffffffffUL;
		else
			size = sectors << 9;
	}
	else
		size=st.st_size;
	
	printf("%14s = %lu\n", "bytes", size-off);
	
	CALLOC(page_status,(int)((size-off+page_size-1)/page_size + 1));
	current_page_status=page_status;
	memset(current_page_status,0,sizeof(*current_page_status));
	
	if( size > off )
	{
		npages = (size-off+page_size-1)/page_size;
		dumpit(fd, &npages);
	}
	
	ratio = 0LL;
	if (stop_jiffies)
	{/* fkt_record could write the stop time, good */
		ratio = (u_64)(stop_jiffies - start_jiffies);
		if( ratio != 0LL )
			ratio = lastreltime[lastcpu] / ratio;
		printf("%llu clock cycles in %u jiffies = %llu cycles per jiffy\n",
		       lastreltime[lastcpu], (unsigned int)(stop_jiffies - start_jiffies), ratio);
		printf("\n");
	}
	
	print_stats(ratio);
	
	/* print final statistics about storage usage */
	printf("\n\n");
	printf("internal storage usage statistics\n\n");
	
	printf("%21s %3u, last possible %3d, percentage used %7.2f%%\n",
	       "last used pos",
			actual_max_pos, MAX_FUN_RECUR-1,
	       ((double)actual_max_pos)*100.0/((double)(MAX_FUN_RECUR-1)));
	printf("%21s %3u, last possible %3d, percentage used %7.2f%%\n",
	       "last used cycles_pos",
	       actual_max_cycles_pos, MAX_EACH_FUN_RECUR-1,
	       ((double)actual_max_cycles_pos)*100.0/
	       ((double)(MAX_EACH_FUN_RECUR-1)));
	printf("%21s %3u, last possible %3d, percentage used %7.2f%%\n",
	       "last used newfun_pos",
	       actual_max_funs_pos, MAX_FUNS-1,
	       ((double)actual_max_funs_pos)*100.0/((double)(MAX_FUNS-1)));

	if (npages)
		page_stats_print(npages);
	return EXIT_SUCCESS;
}

/* vim: ts=4
 */
