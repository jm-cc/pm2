/*	fut_print.c	*/

/*	Usage: fut_print [-f input-file] [-c] [-d] [-u] [-[Mm] cpu_mhz]

			if -f is not given, look for TRACE_FILE environment variable, and
				if that is not given, read from "fut_trace_file".

			if -c is given, time printed is cummulative time
				if that is not given, time is since previous item printed

			if -d is given, turn on debug printout to stdout for stack nesting
				if that is not given, no debug printout is printed

			if -u is given, times are printed in microseconds
				if that is not given, times are printed in cycles

			if -M is given, use that value as the cpu_mhz and do NOT read it
				from the trace file; otherwise use value read from trace file
				The cpu_mhz value is a floating point number.

			if -m is given, use that value as the cpu_mhz and ignore value read
				from the trace file; otherwise use value read from trace file
				The cpu_mhz value is a floating point number.

		If more than 1 -M and/or -m switches are given,
		the last one given is the one that takes effect.
*/

#define _POSIX_C_SOURCE 199309L

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/times.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>
#include "fut.h"

#define WHITE_SPACE " \t\n\f\v\r"
#define OPTIONS ":f:m:M:cdu"
#define DEFAULT_TRACE_FILE "fut_trace_file"

#define HISTO_SIZE	128
#define MAX_HISTO_BAR_LENGTH	20
#define HISTO_CHAR	'*'

static int print_delta_time = 1;	/* 1 to print delta, 0 for cummulative */
static int print_usecs = 0;			/* 1 to print times in usecs, 0 in cycles */
static int bypass_cpu_mhz = 0;		/* 1 to NOT read cpu_mhz from trace file */
static FILE *stdbug = NULL;			/* fd for debug file (/dev/null normally) */


/*	use 64-bit arithmetic for all times */
typedef unsigned long long int	u_64;


/*
Document regarding printing cycles for each function 

MAX_FUNS is to be set to the number of functions that contain probes.
MAX_EACH_FUN_RECUR is the number of times that the same function
could be calling itself recursively.
MAX_NESTING is the depth of call nesting allowed.

"fun_cycles_time" is the structure that holds a counter ( for the number
of times the "entry" to the function is seen), the code ( for the
"entry" probes to the functions), and the total cycles for each function
is maintained in the field total.

Exit codes for each function are 0x100 more than the entry probe codes.
Therefore, the way that the time spent on executing a function is calculated
is as follows.  When a function is entered for the first time, store
it in the array "v", with its entry code.
when the exit code is seen for that function, index into the "v" array
and subtract the entry cycles from the exit cycle.  The difference is stored
in the field "total" cumulatively.
*/

#define MAX_FUNS 200
#define MAX_EACH_FUN_RECUR 100
#define MAX_NESTING 100


/* one of these items in v[] array for each function called */
struct fun_cycles_item
	{
	unsigned int	code;    			/* code for this function */
	unsigned int	counter; 			/* number of times it was called */
	u_64			total;				/* total cycles spent in this function*/
	u_64			called_funs_cycles[MAX_FUNS];	/* ith item = no of cycles
													   in function i when
													   called from this funct */
	unsigned int	called_funs_ctr[MAX_FUNS];	/* ith item = no of calls from
												   this funct to function i */
	};

/* This array is capable of storing information for MAX_FUNS unique functions */
struct fun_cycles_item v[MAX_FUNS];


/*	keep nesting statistics in these arrays */
/*	every cycle is accounted for exactly once */

/*	stacks to keep track of start cycle and code for each nesting */
/*	one stack per id, kept in linked-list headed by v3_stack_head */
/*	new items are added to front of list, so v3_stack_first is the tail too */
/*	current stack is pointed to by v3_stack_ptr */
struct v3_stack_item
	{
	unsigned int			v3_pid;						/* id for this item */
	u_64					v3_empty_time;				/* empty stack time */
	int						v3_pos;						/* top of nest stacks */
	u_64					v3_start_rel[MAX_NESTING];	/* rel-time */
	u_64					v3_start_abs[MAX_NESTING];	/* abs-time */
	unsigned int			v3_code[MAX_NESTING];		/* code */
	unsigned int			v3_index[MAX_NESTING];		/* index into v[] */
	unsigned int			v3_switch_pid[MAX_NESTING];
	struct v3_stack_item	*v3_next;					/* link for v3 list */
	};

static struct v3_stack_item	*v3_stack_head = NULL;	/* head of v3 list */
static struct v3_stack_item	*v3_stack_first = NULL;	/* tail of v3 list */
static struct v3_stack_item	*v3_stack_ptr = NULL;	/* active v3 item */

/*	arrays to keep number of occurrences and time accumulated in each function*/
static u_64			v3_function_rel[MAX_FUNS] = {0};	/* rel no of cycles */
static u_64			v3_function_abs[MAX_FUNS] = {0};	/* abs no of cycles */
static unsigned int	v3_function_count[MAX_FUNS] = {0};	/* no of calls */
static unsigned int	v3_function_code[MAX_FUNS] = {0};	/* entry code */

static int v_pos = 0;
    
u_64 start_time_glob=0L, end_time_glob=0L;
int a, b;


struct pid_cycles_item	{
						unsigned int			pid;
						u_64					total_cycles;
						struct pid_cycles_item	*link;
						};

struct code_list_item	{
						unsigned int			code;
						char					*name;
						struct code_list_item	*link;
						};


struct code_name	{
					unsigned int		code;
					char				*name;
					};

struct code_name	code_table[] =
			{
			{FUT_SETUP_CODE,				"fut_setup"},
			{FUT_KEYCHANGE_CODE,			"fut_keychange"},
			{FUT_RESET_CODE,				"fut_reset"},
			{FUT_CALIBRATE0_CODE,			"fut_calibrate0"},
			{FUT_CALIBRATE1_CODE,			"fut_calibrate1"},
			{FUT_CALIBRATE2_CODE,			"fut_calibrate2"},
			{FUT_SWITCH_TO_CODE,			"fut_switch_to"},
			{FUT_MAIN_ENTRY_CODE,			"main_entry"},
			{FUT_MAIN_EXIT_CODE,			"main_exit"},


			/*	insert new codes here (ifdefs for Raymond) */
#if !defined(PREPROC) && !defined(DEPEND)
#include "fut_print.h"
#endif

			{0, NULL}
			};


/*	user area to hold copy of system trace buffer */
static u_64			basetime = 0, lastreltime = 0;
static time_t		start_time = 0, stop_time = 0;
static int fd;
static unsigned long pid = 0;
static double cpu_mhz = 0.0;				/* processor speed in MHZ */
static unsigned long current_id = 0;


/*	returns	1 if code is "normal" entry/exit code or switch_to.
			0 if code is "singleton" code */
int is_entry_exit( unsigned int code )
	{
	return	code != FUT_SETUP_CODE  &&
			code != FUT_KEYCHANGE_CODE &&
			code != FUT_RESET_CODE &&
			code != FUT_CALIBRATE0_CODE  &&
			code != FUT_CALIBRATE1_CODE  &&
			code != FUT_CALIBRATE2_CODE;
	}

char *find_name( unsigned int code, int keep_entry )
	{
	struct code_name	*ptr;
	static char			local_buf[128];
	int					len, elen;

	for( ptr = code_table;  ptr->code != 0;  ptr++ )
		if( ptr->code == code )
			{
			if( keep_entry )
				return ptr->name;
			else
				{/* caller wants _entry stripped off end of name */
				len = strlen(ptr->name);
				elen = strlen("_entry");
				if( len > elen  && strcmp(&ptr->name[len-elen], "_entry") == 0 )
					{
					strcpy(local_buf, ptr->name);
					local_buf[len-elen] = '\0';
					return local_buf;
					}
				else
					return ptr->name;
				}
			}
	return "unknown code";
	}

static struct pid_cycles_item	*pid_cycles_list = NULL;

/* keeps track of the total number of cycles attributed to each id */
void update_cycles( u_64 cycles, unsigned int pid )
	{
	struct pid_cycles_item	*ptr;

	for( ptr = pid_cycles_list;  ptr != NULL;  ptr = ptr->link )
		{
		if( ptr->pid == pid )
			{/* found item for this pid, just increment its cycles */
			ptr->total_cycles += cycles;
			return;
			}
		}
	/* loop finishes when item for this pid not found, make a new one */
	ptr = (struct pid_cycles_item *)malloc(sizeof(struct pid_cycles_item));
	ptr->pid = pid;
	ptr->total_cycles = cycles;
	ptr->link = pid_cycles_list;		/* add to front of list */
	pid_cycles_list = ptr;
	}


/*	prints the elapsed cycles and the breakdown into per-thread cycles */
/*	also breaks them into "accounted for" within traced functions and
	"unaccounted for" (i.e., outside traced functions */
double print_cycles( unsigned int ratio )
	{
	struct pid_cycles_item	*ptr;
	double					t, ptot, total, utotal;
	u_64					TimeDiff;
	struct v3_stack_item	*v3_ptr;

	printf("\n\n");
	printf("%32s %12llu\n","cycle clock at start",start_time_glob);
	printf("%32s %12llu\n", "cycle clock at end", end_time_glob);
	TimeDiff = end_time_glob - start_time_glob;
	t = ((double)TimeDiff)/cpu_mhz;
	printf("%32s %12llu (%10.3f usecs)\n", "total elapsed clock cycles",
																TimeDiff, t);

	printf("\n\n");

	total = 0.0;
	ptot = 0.0;
	printf("%32s\n", "elapsed clock cycles by id");
	printf("\n");
	printf("%32s %12s %12s %10s\n", "id", "cycles", "usecs", "percent");
	printf("\n");
	for( ptr = pid_cycles_list;  ptr != NULL;  ptr = ptr->link )
		total += (double)ptr->total_cycles;
	for( ptr = pid_cycles_list;  ptr != NULL;  ptr = ptr->link )
		{
		t = (double)ptr->total_cycles;
		printf("%32u %12.0f %12.3f %9.2f%%\n",
								ptr->pid, t, t/cpu_mhz, t*100.0/total);
		ptot += t*100.0/total;
		}
	printf("%32s %12.0f %12.3f %9.2f%%\n",
	"total elapsed clock cycles", total, total/cpu_mhz, ptot);
	printf("\n\n");

	/*	print all the stack base times */
	printf("%32s\n", "cycles outside traced functions");
	printf("\n");
	printf("%32s %12s %12s %10s\n", "id", "cycles", "usecs", "percent");
	printf("\n");
	utotal = 0.0;
	ptot = 0.0;
	for( v3_ptr = v3_stack_head;  v3_ptr != NULL;  v3_ptr = v3_ptr->v3_next )
		utotal += (double)v3_ptr->v3_empty_time;
	for( v3_ptr = v3_stack_head;  v3_ptr != NULL;  v3_ptr = v3_ptr->v3_next )
		{
		t = (double)v3_ptr->v3_empty_time;
		printf("%32u %12llu %12.3f %9.2f%%\n",
							v3_ptr->v3_pid, v3_ptr->v3_empty_time,
							t/cpu_mhz, t*100.0/utotal);
		ptot += t*100.0/utotal;
		}
	printf("%32s %12.0f %12.3f %9.2f%%\n", "total outside traced functions",
												utotal, utotal/cpu_mhz, ptot);
	printf("%32s %12.0f %12.3f\n", "elapsed total", total, total/cpu_mhz);
	printf("%32s %12.0f %12.3f\n", "total inside traced functions",
									total - utotal, (total-utotal)/cpu_mhz);
	return total - utotal;
	}


/* first probe for this id, create a new stack */
struct v3_stack_item *create_stack( unsigned int thisid )
	{
	struct v3_stack_item	*v3_ptr;

	fprintf(stdbug, "create_stack for id %u\n", thisid);

	v3_ptr = (struct v3_stack_item *)malloc(sizeof(struct v3_stack_item));
	if( v3_ptr == NULL )
		{
		fprintf(stderr, "=== can't malloc stack for id %d\n", thisid);
		fprintf(stdout, "=== can't malloc stack for id %d\n", thisid);
		exit(EXIT_FAILURE);
		}
	v3_ptr->v3_pid = thisid;
	v3_ptr->v3_empty_time = 0LL;
	v3_ptr->v3_pos = 0;
	v3_ptr->v3_code[0] = 0;
	v3_ptr->v3_index[0] = 0;
	v3_ptr->v3_switch_pid[0] = 0;
	if( v3_stack_first == NULL )
		{/* this is the first stack ever created */
		v3_stack_first = v3_ptr;
		v3_ptr->v3_start_abs[0] = v3_ptr->v3_start_rel[0] = start_time_glob;
		}
	else
		{/* initialize new stack with time of item at top of current stack */
		v3_ptr->v3_start_abs[0] =
							v3_stack_ptr->v3_start_rel[v3_stack_ptr->v3_pos];
		v3_ptr->v3_start_rel[0] =
							v3_stack_ptr->v3_start_rel[v3_stack_ptr->v3_pos];
		}

	v3_ptr->v3_next = v3_stack_head;	/* push on stack list */
	v3_stack_head = v3_ptr;
	return v3_ptr;
	}


void update_stack( u_64 rel_delta, u_64 abs_delta )
	{
	unsigned int	i;

	/* move up start time of all previous in nesting */
	for( i = 0;  i < v3_stack_ptr->v3_pos;  i++ )
		{
		v3_stack_ptr->v3_start_rel[i] += rel_delta;
		v3_stack_ptr->v3_start_abs[i] += abs_delta;
		}

	/* then pop the stack */
	v3_stack_ptr->v3_pos--;
	fprintf(stdbug, "pop off v3 stack for id %u, top %d\n",
							v3_stack_ptr->v3_pid, v3_stack_ptr->v3_pos);
	}


/*	called to update v3_function_xxx entries for function
	when exiting the function with entry code "code".
	relative is the cycles spent in this function ONLY
		(cycles in all functions it called have been subtracted out)
	absolute is the cycles spent in the function
		(including cycles in all functions it called).
*/
void update_function( u_64 relative, u_64 absolute, int code )
	{
	int				j, k;


	/* first search the v3_function_code array for entry with this code */
	j = -1;
	for( k = 0;  k < MAX_FUNS;  k++ )
		{
		if( code == v3_function_code[k] )
			{/* this is the match, update counters for this function */
			break;
			}
		else if( j < 0  &&  v3_function_code[k] == 0 )
			{/* remember this empty slot in case we need to use it */
			j = k;
			}
		}
	if( k >= MAX_FUNS )
		{/* first occurrence of this entry/exit pair */
		if( j < 0 )
			{
			fprintf(stderr,"=== no room for function entry code %04x\n",code);
			fprintf(stdout,"=== no room for function entry code %04x\n",code);
			return;
			}
		else
			{/* store this code here */
			v3_function_code[j] = code;
			k = j;
			}
		}

	v3_function_abs[k] += absolute;
	v3_function_rel[k] += relative;
	v3_function_count[k] += 1;

	fprintf(stdbug,"%6s '%s' by %llu relative cycles, total %llu, count %u\n",
		"update", find_name(code,1), relative,
		v3_function_rel[k], v3_function_count[k]);
	fprintf(stdbug, "%6s by %llu absolute cycles, total %llu\n",
		"", absolute, v3_function_abs[k]);
	}


/*	returns index of code or corresponding entry code in v[] array,
	creating new entry if needed */
int get_code_index( unsigned int code )
	{
	int		k, i;

    for( k = 0;  k < v_pos;  k++ )
    	{
        if( code == v[k].code  ||  code == v[k].code + FUT_GENERIC_EXIT_OFFSET )
        	{/* yes, this code was seen before in slot k of v array */
			return k;
			}
		}

	/* if loop finishes we know this is the first time we have seen this code */
	/* so we need to create a new entry in the v array for this code */
	if( v_pos >= MAX_FUNS )
		{/* too many unique functions being traced */
		fprintf(stderr, "v_pos = %d hit MAX_FUNS limit %d\n", v_pos, MAX_FUNS );
		printf( "v_pos = %d hit MAX_FUNS limit %d\n", v_pos, MAX_FUNS );
		exit(EXIT_FAILURE);
		}

	fprintf(stdbug, "create v[%d] for code %04x with name %s\n",
											v_pos, code, find_name(code, 1));

	/* initialize new entry to all zeroes */
	v[v_pos].code = code; 
	v[v_pos].total = 0LL;
	v[v_pos].counter = 0;

	for( i = 0; i < MAX_FUNS; i++ )
		{
		v[v_pos].called_funs_cycles[i] = 0LL;
		v[v_pos].called_funs_ctr[i] = 0;
		}

	v_pos++;
	return k;
	}

/* here to use 1 buffer record to update statistics */
void my_print_fun( unsigned int code, u_64 cyc_time,
								unsigned int thisid, int param1, int param2 )
	{
    int			i, j, index;
    u_64		rel_delta, abs_delta, off_rel_time, off_abs_time;

    
	off_abs_time = off_rel_time = cyc_time;

	/*	get index into v[] array for this code or its corresponding entry code*/
	index = get_code_index(code);

	if( v3_stack_ptr == NULL )
		{/* first time through here, we need a stack */
		v3_stack_ptr = create_stack(thisid);
		}

	else if( v3_stack_ptr->v3_pid != thisid )
		{/* next probe not the current id, find a stack for the new id */
		struct v3_stack_item	*v3_ptr;

		if( v3_stack_ptr->v3_code[v3_stack_ptr->v3_pos] == FUT_SWITCH_TO_CODE )
			{
			off_rel_time = v3_stack_ptr->v3_start_rel[v3_stack_ptr->v3_pos];
			off_abs_time = v3_stack_ptr->v3_start_abs[v3_stack_ptr->v3_pos];
			}
		else
			{/* top of current stack is not switch_to, should be */
			fprintf(stderr,
						"=== top of current stack for id %u not switch_to\n",
						v3_stack_ptr->v3_pid);
			fprintf(stdout,
						"=== top of current stack for id %u not switch_to\n",
						v3_stack_ptr->v3_pid);
			}
		for( v3_ptr = v3_stack_head;  v3_ptr != NULL; v3_ptr = v3_ptr->v3_next )
			{
			if( v3_ptr->v3_pid == thisid )
				{/* found an existing stack for the new id */
				fprintf(stdbug, "back to v3 stack for id %u, top %d\n",
													thisid, v3_ptr->v3_pos);
				if( v3_ptr->v3_code[v3_ptr->v3_pos] == FUT_SWITCH_TO_CODE )
					{/* top of previous stack is switch_to, pop it off */
					v3_stack_ptr = v3_ptr;
					rel_delta = off_rel_time -
										v3_ptr->v3_start_rel[v3_ptr->v3_pos];
					abs_delta = off_abs_time -
										v3_ptr->v3_start_abs[v3_ptr->v3_pos];
					/* do we need this next update_function?? */
					update_function(rel_delta, abs_delta, FUT_SWITCH_TO_CODE);
					/* move up start of all previous in nesting and pop stack */
					update_stack(rel_delta, abs_delta);
					}
				else if( v3_ptr->v3_pos != 0 )
					{
					fprintf(stderr,
							"=== previous stack does not have switch_to on top,"
							" pos = %d\n", v3_ptr->v3_pos);
					fprintf(stdout,
							"=== previous stack does not have switch_to on top,"
							" pos = %d\n", v3_ptr->v3_pos);
					}
				else
					{/* reinit previous stack with time of item at top of
						current stack */
					v3_ptr->v3_start_abs[0] =
							v3_stack_ptr->v3_start_rel[v3_stack_ptr->v3_pos];
					v3_ptr->v3_start_rel[0] =
							v3_stack_ptr->v3_start_rel[v3_stack_ptr->v3_pos];
					}
				break;
				}
			}
		if( v3_ptr == NULL )
			{/* first probe for this id, create a new stack */
			v3_ptr = create_stack(thisid);
			}
		v3_stack_ptr = v3_ptr;
		}


	/*	now guaranteed that v3_stack_ptr points to stack for this id */
	/*	i gets code at top of this stack */
	i = v3_stack_ptr->v3_code[v3_stack_ptr->v3_pos];

	/* code for a function entry/exit, including switch_to */
	if( (code == i + FUT_GENERIC_EXIT_OFFSET) ||
		(code == FUT_SWITCH_TO_CODE  &&  i == FUT_SWITCH_TO_CODE
		&&  param1 == v3_stack_ptr->v3_switch_pid[v3_stack_ptr->v3_pos]) )

		{/* this is the exit corresponding to a previously stacked entry */
		/* or a switch back to previously stacked switch from */
		rel_delta = cyc_time -
						v3_stack_ptr->v3_start_rel[v3_stack_ptr->v3_pos];
		abs_delta = cyc_time -
						v3_stack_ptr->v3_start_abs[v3_stack_ptr->v3_pos];
		update_function(rel_delta, abs_delta, i);
		if( code == i + FUT_GENERIC_EXIT_OFFSET )
			{/* this code is the exit corresponding to code i */
			if (v3_stack_ptr->v3_pos >= 2 )
				{/* have prev entry on this stack, update its cycles */
				j = v3_stack_ptr->v3_index[v3_stack_ptr->v3_pos - 1]; 
				v[j].called_funs_cycles[index] += rel_delta;  
				}
			} 
		/* move up start time of all previous in nesting and pop stack */
		update_stack(rel_delta, 0LL);
		}

	else
		{/* this is an entry or singleton code */
		if( v3_stack_ptr->v3_pos == 0 )
			{/* stack was empty, attribute this time to the thread */
			rel_delta = cyc_time - lastreltime;
			v3_stack_ptr->v3_empty_time += rel_delta;
			fprintf(stdbug, "add %llu to v3_empty_time for id %u, total %llu\n",
								rel_delta, thisid, v3_stack_ptr->v3_empty_time);
			}
		if( is_entry_exit(code) )
			{/* this must be an entry code, just stack it if we can */
			if( v3_stack_ptr->v3_pos >= MAX_NESTING )
				{
				fprintf(stderr, "=== v3 stack overflow, max depth %d\n",
														MAX_NESTING);
				fprintf(stdout, "=== v3 stack overflow, max depth %d\n",
														MAX_NESTING);
				exit(EXIT_FAILURE);
				}
			/* stack will not overflow, ok to push onto it */
			v3_stack_ptr->v3_pos++;
			v3_stack_ptr->v3_start_abs[v3_stack_ptr->v3_pos] =
					v3_stack_ptr->v3_start_rel[v3_stack_ptr->v3_pos] = cyc_time;
			v3_stack_ptr->v3_code[v3_stack_ptr->v3_pos] = code;
			v3_stack_ptr->v3_switch_pid[v3_stack_ptr->v3_pos] = thisid;
			v3_stack_ptr->v3_index[v3_stack_ptr->v3_pos] = index;
				
			if ( v3_stack_ptr->v3_pos >= 2 )
				{/*have prev entry on this stack, update its calls to this fun*/
				j = v3_stack_ptr->v3_index[v3_stack_ptr->v3_pos - 1];
				v[j].called_funs_ctr[index]++;
				}

			/* count number of times this function was called */
			v[index].counter++;

			fprintf(stdbug,
						"push on v3 stack for id %u, top %d, code %04x, "
						"index %d, start rel %llu\n",
						thisid, v3_stack_ptr->v3_pos, code, index,
						v3_stack_ptr->v3_start_rel[v3_stack_ptr->v3_pos]);
			}
		/*	else this is a singleton code, just ignore it!
			(certainly do NOT push it on the stack or count it) */
		}

	fprintf(stdbug, "\n");
	}


/*	draw a horizontal line of 80 instances of character c onto stdout */
void my_print_line( int c )
	{
	int		i;

	for( i = 0;  i < 80;  i++ )
		putchar(c);
	putchar('\n');
	}


void draw_histogram( int n, double histo_max, char **histo_name,
				u_64 *histo_cycles, double *histo_percent, double total_cycles)
	{
	int		i, k, length;
	u_64	sum;

	printf("\n");
	printf("%37s %12s %8s\n\n","Name", print_usecs?"Usecs":"Cycles", "Percent");
	sum = 0LL;
	for( k = 0;  k < n;  k++ )
		{
		if( print_usecs )
			printf("%37s %12.3f %7.2f%% ",
			histo_name[k], ((double)histo_cycles[k])/cpu_mhz, histo_percent[k]);
		else
			printf("%37s %12llu %7.2f%% ",
							histo_name[k], histo_cycles[k], histo_percent[k]);
		length = (int)((histo_percent[k]*MAX_HISTO_BAR_LENGTH)/histo_max + 0.5);
		for( i = 0;  i < length;  i++ )
			putchar(HISTO_CHAR);
		putchar('\n');
		sum += histo_cycles[k];
		}
	if( print_usecs )
		printf("%37s %12.3f %7.2f%%\n\n", "Total usecs in traced functions",
					((double)sum)/cpu_mhz, 100.0*((double)sum)/total_cycles);
	else
		printf("%37s %12llu %7.2f%%\n\n", "Total cycles in traced functions",
								sum, 100.0*((double)sum)/total_cycles);
	}


void my_print_fun_helper( unsigned int CycPerJiffy )
	{
	int						i, z, k, index;
	u_64					sum_other_cycles = 0, rel_delta, abs_delta;
	u_64					sum, off_abs_time;
	double					Main_fun_sum, Main_fun_sum_percent;
	double					total_cycles, called_fun_percent, only_fun_percent;
	double					called_fun, only_fun;
	struct v3_stack_item	*v3_ptr;
	double					histo_max, ave;
	char					*name;
	char					*histo_name[HISTO_SIZE];
	u_64					histo_cycles[HISTO_SIZE];
	double					histo_percent[HISTO_SIZE];


	/*	close up all open stacks */
	/*	stacks are typically left open when a thread is blocked */
	for( v3_ptr = v3_stack_head;  v3_ptr != NULL;  v3_ptr = v3_ptr->v3_next )
		{
		if( v3_ptr->v3_pos > 1 )
			{
			fprintf(stderr, "=== final stack for id %u has pos %d\n",
											v3_ptr->v3_pid, v3_ptr->v3_pos);
			fprintf(stdout, "=== final stack for id %u has pos %d\n",
											v3_ptr->v3_pid, v3_ptr->v3_pos);
			}
		if( v3_ptr->v3_pos > 0 )
			{
			if( v3_ptr->v3_code[v3_ptr->v3_pos] != FUT_SWITCH_TO_CODE )
				{/* last thing thread did was not a switch_to */
				fprintf(stderr,
				"=== top of final stack for id %u not switch_to\n",
																v3_ptr->v3_pid);
				fprintf(stdout,
				"=== top of final stack for id %u not switch_to\n",
																v3_ptr->v3_pid);
				off_abs_time = lastreltime;
				}
			else
				{/* last thing thread did was switch_to, use its abs time */
				off_abs_time = v3_ptr->v3_start_abs[v3_ptr->v3_pos];
				}
			for( i = v3_ptr->v3_pos-1;  i > 0;  i-- )
				{/* close up all unterminated function calls on this stack */
				unsigned int	code;

				code = v3_ptr->v3_code[i];
				rel_delta = v3_ptr->v3_start_rel[i+1] - v3_ptr->v3_start_rel[i];
				abs_delta = off_abs_time-v3_ptr->v3_start_abs[i];
				fprintf(stdbug, "%2d: %04x %s, rel %llu, abs %llu\n",
										i, code,  find_name(code, 1), rel_delta,
										abs_delta);
				update_function(rel_delta, abs_delta, code);
				v3_ptr->v3_start_rel[0] += rel_delta;
				}
			}
		}

	total_cycles = print_cycles(0);

/*	fill in the v[].total array by copying the v3_function_abs value */
	for( k = 0;  k < MAX_FUNS;  k++ )
		{
		if( v3_function_code[k] > 0 )
			{/* this slot has a valid function code in it */
			index = get_code_index(v3_function_code[k]);
			if( v3_function_count[k] != v[index].counter &&
				v[index].code != FUT_SWITCH_TO_CODE )
				{
				printf("0x%04x: function count %u not equal to v counter %u\n",
				v3_function_code[k], v3_function_count[k], v[index].counter);
				}
			if( v[index].total > 0 )
				printf("0x%04x: v total %llu not zero\n",
										v3_function_code[k], v[index].total);
			v[index].total = v3_function_abs[k];
			}
		}

/*	print table of all function names */
if( v_pos > 0 )
	{
	printf("\n\n\n");
	if( print_usecs )
		printf("%55s\n\n", "elapsed usecs in traced functions");
	else
		printf("%55s\n\n", "elapsed cycles in traced functions");

	printf("%36s  %4s %14s %8s %12s\n",
			"Name", "Code", print_usecs?"Usecs":"Cycles", "Count", "Avg");
	printf("%36s  %4s %14s %8s %12s\n",
			"----", "----", "------", "-----", "---");
	for ( i = 0 ; i < v_pos; i ++ )
		{
		if( v[i].code > 0  &&  v[i].code != FUT_SWITCH_TO_CODE )
			{
			ave = ((double)v[i].total)/((double)v[i].counter);
			printf("%36s  %04x ", find_name(v[i].code,0), v[i].code);
			if( print_usecs )
				printf("%14.3f %8u %12.1f\n", 
						((double)v[i].total)/cpu_mhz,v[i].counter,ave/cpu_mhz);
			else
				printf("%14llu %8u %12.1f\n", v[i].total, v[i].counter, ave);
			}
		}
	printf("\n");

	printf("\n\n");
	printf("%4s %31s  %4s %14s %8s\n",
				"From", "To", "Code", print_usecs?"Usecs":"Cycles", "Count");
	printf("%4s %31s  %4s %14s %8s\n",
				"----", "--", "----", "------", "-----");
	for ( z = 0; z < v_pos; z++)
		if( v[z].code != FUT_SWITCH_TO_CODE )
			{
			printf("%s\n", find_name(v[z].code,0));
			for ( i = 0; i < v_pos; i++)
				{
				if(v[z].called_funs_ctr[i]!=0 && v[i].code!=FUT_SWITCH_TO_CODE) 
					{
					printf("%36s  %04x ", find_name(v[i].code,0), v[i].code);
					if( print_usecs )
						printf("%14.3f", 
								((double)v[z].called_funs_cycles[i])/cpu_mhz);
					else
						printf("%14llu", v[z].called_funs_cycles[i]);
					printf(" %8u\n", v[z].called_funs_ctr[i]);
					}  
				}
			printf("\n");
			}

	printf("\n\n");
	printf("%50s\n\n", "NESTING SUMMARY");
	my_print_line('*');
	printf("%33s %4s %12s%7s %12s %8s\n",
			"Name", "Code", print_usecs?"Usecs":"Cycles",
			"Count", "Average", "Percent");
	my_print_line('*');
	for ( z = 0 ; z < v_pos; z ++ )
		if( v[z].code != FUT_SWITCH_TO_CODE )
			{
			Main_fun_sum = (double)v[z].total;
			Main_fun_sum_percent = 100.0;
			ave = Main_fun_sum/((double)v[z].counter);
			printf("%33s %04x ", find_name(v[z].code,0), v[z].code);
			if( print_usecs )
				printf("%12.3f%7u %12.1f", 
							Main_fun_sum/cpu_mhz, v[z].counter, ave/cpu_mhz);
			else
				printf("%12llu%7u %12.1f", 
							v[z].total, v[z].counter, ave);
			printf(" %7.2f%%\n", Main_fun_sum_percent);

			for ( i = 0; i < v_pos; i++)
				{

				if(v[z].called_funs_ctr[i]!=0 && v[i].code!=FUT_SWITCH_TO_CODE) 
					{
					called_fun = (double)v[z].called_funs_cycles[i];
					called_fun_percent = called_fun/Main_fun_sum * 100.0;
					ave = called_fun/((double)v[z].called_funs_ctr[i]);
					printf("%33s %04x ", 
							find_name(v[i].code,0), v[i].code);
					if( print_usecs )
						printf("%12.3f%7u %12.1f", 
										called_fun/cpu_mhz,
										v[z].called_funs_ctr[i], ave/cpu_mhz);
					else
						printf("%12llu%7u %12.1f", 
										v[z].called_funs_cycles[i],
										v[z].called_funs_ctr[i], ave);
					printf(" %7.2f%%\n", called_fun_percent );
					sum_other_cycles += v[z].called_funs_cycles[i];
					}
				}
			/* for fsync, account for actual disk waiting time */
			only_fun = (double)(v[z].total - sum_other_cycles);
			only_fun_percent = only_fun/Main_fun_sum * 100.0;
			ave = only_fun/((double)v[z].counter);
			printf("%28s ONLY %04x ", find_name(v[z].code,0), v[z].code);
			if( print_usecs )
				printf("%12.3f%7u %12.1f", 
								only_fun/cpu_mhz, v[z].counter, ave/cpu_mhz);
			else
				printf("%12llu%7u %12.1f", 
							v[z].total - sum_other_cycles, v[z].counter, ave);
			printf(" %7.2f%%\n", only_fun_percent); 
			sum_other_cycles = 0LL;
			my_print_line('-');
			printf("\n");
			}
	}

	/* now print out the precise accounting statistics from the v3 arrays */
	sum = 0LL;
	k = 0;
	histo_max = 0.0;
	printf("\n\n\n");
	printf("%70s\n\n",
			"Accounting for every cycle in a traced function exactly once");
	my_print_line('*');
	printf("%33s %4s %12s %7s %11s %8s\n",
					"Name", "Code", print_usecs?"Usecs":"Cycles",
					"Count", "Average", "Percent");
	my_print_line('*');
	for( i = 0;  i < MAX_FUNS; i++ )
		{
		z = v3_function_code[i];
		if( z > 0  &&  z != FUT_SWITCH_TO_CODE )
			{/* this occurred at least once and was not switch_to, print it */

			name = find_name(z,0);
			if( (histo_name[k] = malloc(strlen(name)+1)) == NULL )
				{
				fprintf(stderr, "no space to malloc histo_name\n");
				exit(EXIT_FAILURE);
				}
			strcpy(histo_name[k], name);
			histo_cycles[k] = v3_function_rel[i];
			sum += histo_cycles[k];
			histo_percent[k] = 
						100.0*((double)histo_cycles[k])/total_cycles;
			if( histo_max < histo_percent[k] )
				histo_max = histo_percent[k];
			ave = ((double)histo_cycles[k])/((double)v3_function_count[i]);
			printf("%33s %04x", name, z);
			if( print_usecs )
				printf(" %12.3f %7u %11.1f %7.2f%%\n",
					((double)histo_cycles[k])/cpu_mhz, v3_function_count[i],
					ave/cpu_mhz, histo_percent[k]);
			else
				printf(" %12llu %7u %11.1f %7.2f%%\n",
						histo_cycles[k], v3_function_count[i],
						ave, histo_percent[k]);
			k++;
			}
		}


	if( print_usecs )
		{
		printf("%33s", "Total usecs in traced functions");
		printf(" %4s %12.3f %7s %11s %7.2f%%\n", "",
			((double)sum)/cpu_mhz, "", "", 100.0*((double)sum)/total_cycles);
		}
	else
		{
		printf("%33s", "Total cycles in traced functions");
		printf(" %4s %12llu %7s %11s %7.2f%%\n", "",
								sum, "", "", 100.0*((double)sum)/total_cycles);
		}

	printf("\n\n\n%75s\n",
	"Histogram accounting for every cycle in a traced function exactly once");
	draw_histogram(k, histo_max, histo_name, histo_cycles, histo_percent,
																total_cycles);
	}


/*
bufptr[0] is the first item for each line, lo-cycles
bufptr[1] is the hi-cycles, which is currently ignored
bufptr[2] is code
bufptr[3] is P1
bufptr[4] is P2
...
bufptr[n] is P(n-2)
*/
unsigned int *dumpslot( unsigned int *n,  unsigned int *bufptr )
	{
	unsigned int	code, fullcode, params, i;
	int				print_this = 0;
	u_64			r, reltime, ptime, *longbufptr;


	longbufptr = (u_64 *)bufptr;
	reltime =  *longbufptr - basetime;
	r = reltime-lastreltime;
	end_time_glob = reltime;
	if( print_delta_time )
		ptime = r;					/* print time delta from last entry */
	else
		ptime = reltime;			/* print cummulative time on cpu */
	fullcode = bufptr[2];
	code = fullcode >> 8;
	if( code == FUT_SETUP_CODE  ||
		code == FUT_KEYCHANGE_CODE ||
		code == FUT_RESET_CODE )
		{
		print_this = 1;
		current_id = bufptr[4];		/* get new thread id */
		}
	else if( code == FUT_CALIBRATE0_CODE  ||
			code == FUT_CALIBRATE1_CODE  ||
			code == FUT_CALIBRATE2_CODE )
		{
		print_this = 1;
		}
	print_this = 1;
    
    if (print_this)
		{
		if( print_usecs )
			printf( "%14.3f%5lu", ((double)ptime)/cpu_mhz, current_id);
		else
			printf( "%14llu%5lu", ptime, current_id);
		}

	if (print_this)
		printf(" %04x", code);
	params = bufptr[2] & 0xff;
	if( params & 0x03 )
		{
		printf(
			"***** header size not multiple of 4: %08x, %04x, %d *****\n",
												bufptr[2], code, params);
		exit(EXIT_FAILURE);
		}
	params = (params>>2);
	if (print_this)
		{
		printf(" %22s",  find_name(code, 1));

		for( i = 3;  i < params;  i++ )
			{/* hueristic - "small" numbers print in decimal, large in hex */
			if( bufptr[i] <= 0xfffffff )
				printf("%11u", bufptr[i]);
			else
				printf(" %#010x", bufptr[i]);
			}
		printf("\n");
		/***** fflush(stdout); *****/
		}
	*n += params;
	if( v3_stack_ptr == NULL  &&  !is_entry_exit(code) )
		{/* a singleton code before entering any functions (part of setup) */
		start_time_glob += r;			/* just move up the global start time */
		fprintf(stdbug,
		"add %llu cycles to start_time_glob, total %llu, v3_stack_ptr %p\n",
											r, start_time_glob, v3_stack_ptr);
		}
	else
		{/* normal operation -- at least one function has been active */
		update_cycles(r, current_id);
		if( params <= 4 )
			my_print_fun(code, reltime, current_id, bufptr[3], 0);
		else
			my_print_fun(code, reltime, current_id, bufptr[3], bufptr[4]);
		}
	if( code == FUT_SWITCH_TO_CODE )
		current_id = bufptr[3];
	lastreltime = reltime;

	return &bufptr[params];
	}

/*	returns 0 if ok, else -1 */
int dumpit( unsigned int *buffer, unsigned int nints )
	{
	unsigned int	n;
	unsigned int	*bufptr, *buflimit;


	printf( "%14s%5s %4s %22s",
						print_usecs?"usecs":"cycles", "id", "code", "name");
	printf("%11s%11s%11s\n", "P1", "P2", "P3");


	buflimit = buffer + nints;
	basetime = *(u_64 *)buffer;
	start_time_glob = lastreltime = 0LL;
	for( bufptr = buffer, n = 0;  n < nints && bufptr < buflimit; )
		{
		bufptr = dumpslot(&n, bufptr);
		}
	my_print_fun_helper(0);
	return 0;
	}


void get_the_time( time_t *the_time, char *message )
	{
	struct tm	breakout;
	char		buffer[128];


	if( read(fd, (void *)the_time, sizeof(time_t)) <= 0 )
		{
		perror(message);
		exit(EXIT_FAILURE);
		}
	if( localtime_r(the_time, &breakout) == NULL )
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


int main( int argc, char *argv[] )
	{
	unsigned int	nints;
	unsigned int	*bufptr;
	char			*infile;
	int				c, i;


	infile = NULL;
	stdbug = NULL;
	opterr = 0;

	while( (c = getopt(argc, argv, OPTIONS)) != EOF )
		switch( c )
			{
		case 'u':
			print_usecs = 1;
			break;
		case 'd':
			stdbug = stdout;
			break;
		case 'c':
			print_delta_time = 0;
			break;
		case 'f':
			infile = optarg;
			break;
		case 'm':
		case 'M':
			if( sscanf(optarg, "%lf", &cpu_mhz) != 1  ||  cpu_mhz <= 0.0 )
				{
				fprintf(stderr, "illegal cpu mhz value: %s\n", optarg);
				exit(EXIT_FAILURE);
				}
			if( c == 'M' )
				bypass_cpu_mhz = 1;		/* do NOT read cpu_mhz from trace file*/
			else
				bypass_cpu_mhz = 0;
			break;
		case ':':
			fprintf(stderr, "missing parameter to switch %c\n", optopt);
			exit(EXIT_FAILURE);
		case '?':
			fprintf(stderr, "illegal switch %c\n", optopt);
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

	/*	the order of reading items from the trace file must obviously
		correspond with the order they were written by fut_record	*/
	if( read(fd, (void *)&pid, sizeof(pid)) <= 0 )
		{
		perror("read pid");
		exit(EXIT_FAILURE);
		}
	printf("%14s = %lu\n", "pid", pid);

	if( !bypass_cpu_mhz )
		{
		if( read(fd, (void *)&cpu_mhz, sizeof(cpu_mhz)) <= 0 )
			{
			perror("read cpu_mhz");
			exit(EXIT_FAILURE);
			}
		}
	printf("%14s = %.2f\n", "cpu_mhz", cpu_mhz);


	get_the_time(&start_time, "start time");
	get_the_time(&stop_time, "stop time");

	if( read(fd, (void *)&nints, sizeof(nints)) <= 0 )
		{
		perror("read nints");
		exit(EXIT_FAILURE);
		}
	printf("%14s = %u\n", "nints", nints);
	printf("\n");

	if( (bufptr = (unsigned int *)malloc(nints*4)) == NULL )
		{
		fprintf(stderr, "unable to malloc %u ints\n", nints);
		exit(EXIT_FAILURE);
		}

	if( read(fd, (void *)bufptr, nints*4) <= 0 )
		{
		perror("read buffer");
		exit(EXIT_FAILURE);
		}

	for( i = 0;  i < MAX_FUNS;  i++ )
		{
		v3_function_rel[i] = 0LL;
		v3_function_abs[i] = 0LL;
		v3_function_count[i] = 0;
		v3_function_code[i] = 0;
		}

	dumpit(bufptr, nints);

	return 0;
	}
