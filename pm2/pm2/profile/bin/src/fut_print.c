/*	fut_print.c	*/

/*	Usage: fut_print [-f input-file] [-c] [-d]

			if -f is not given, look for TRACE_FILE environment variable, and
				if that is not given, read from "fut_trace_file".

			if -c is given, time printed is cummulative time
				if that is not given, time is since previous item printed

			if -d is given, turn on debug printout to stdout for stack nesting
				if that is not given, no debug printout is printed
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
#define OPTIONS ":f:pcda"
#define DEFAULT_TRACE_FILE "fut_trace_file"

#define HISTO_SIZE	128
#define MAX_HISTO_BAR_LENGTH	29
#define HISTO_CHAR	'*'

static int print_delta_time = 1;	/* 1 to print delta, 0 for cummulative */
static FILE *stdbug = NULL;			/* fd for debug file (/dev/null normally) */

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


struct fun_cycles_item {
					int				counter; 
					int				code ;    
					unsigned int	total;
					unsigned int	called_funs_cycles[MAX_FUNS];
					int				called_funs_ctr[MAX_FUNS] ;
                       };

struct prev_cycle {
					int				code;
					unsigned int	cycle_value;
					int				first_param;
                  };

/* This array is capable of storing information for 100 functions */

struct fun_cycles_item v[MAX_FUNS];
struct prev_cycle previous;
struct prev_cycle previous_switchto;


/*	keep nesting statistics in these arrays */
/*	every cycle is accounted for exactly once */

/*	stacks to keep track of start cycle and code for each nesting */
/*	one stack per pid, kept in linked-list headed by v3_stack_head */
/*	current stack is pointed to by v3_stack_ptr */
struct v3_stack_item
				{
				int						v3_pos;
				unsigned int			v3_pid;
				unsigned int			v3_start_cycle[MAX_NESTING];
				unsigned int			v3_code[MAX_NESTING];
				unsigned int			v3_index[MAX_NESTING];
				unsigned int			v3_switch_pid[MAX_NESTING];
				struct v3_stack_item	*v3_next;
				};

static struct v3_stack_item	*v3_stack_head = NULL;
static struct v3_stack_item	*v3_stack_first = NULL;
static struct v3_stack_item	*v3_stack_ptr = NULL;

/*	arrays to keep track of number of occurrences and time accumulated */
static unsigned int v3_function_cycles[MAX_FUNS] = {0};
static unsigned int v3_function_count[MAX_FUNS] = {0};
static unsigned int v3_function_code[MAX_FUNS] = {0};

static int v_pos = 0;
    
unsigned int start_time_glob=0, end_time_glob=0;
int a, b;


struct pid_cycles_item	{
						unsigned int			pid;
						unsigned int			hi_cycles;
						unsigned int			lo_cycles;
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
static unsigned int	basetime = 0, lastreltime = 0;
static time_t		start_time = 0, stop_time = 0;
static int fd;
static unsigned long pid = 0;
static unsigned long current_id = 0;


/* initially 1.
 * set to 1 when hit first sys_call entry to times for kidpid task
 *			at which time start_time_glob is set to reltime.
 *			or if all_stats is set, on the first probe of any kind.
 * while 1, allows accumulation of entry/exit statistics
 * set to -1 when hit sys_call entry to times for kidpid task and
 *			it is already set to 1.  end_time_glob is set to reltime then.
 *			of if all_stats is set, on first fut_keychange.
 */
static int fun_time = 0;



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

void update_cycles( unsigned int cycles, unsigned int pid )
	{
	struct pid_cycles_item	*ptr;
	unsigned int			i;

	for( ptr = pid_cycles_list;  ptr != NULL;  ptr = ptr->link )
		{
		if( ptr->pid == pid )
			{/* found item for this pid, just increment its cycles */
			i = ptr->lo_cycles + cycles;
			if( i < ptr->lo_cycles )
				ptr->hi_cycles++;
			ptr->lo_cycles = i;
			return;
			}
		}
	/* loop finishes when item for this pid not found, make a new one */
	ptr = (struct pid_cycles_item *)malloc(sizeof(struct pid_cycles_item));
	ptr->pid = pid;
	ptr->hi_cycles = 0;
	ptr->lo_cycles = cycles;
	ptr->link = pid_cycles_list;		/* add to front of list */
	pid_cycles_list = ptr;
	}


void print_cycles( unsigned int ratio )
	{
	struct pid_cycles_item	*ptr;
	double					t, total, ptot;

	total = 0.0;
	ptot = 0.0;
	printf("%9s %12s %11s\n", "id", "cycles", "percent");
	printf("\n");
	for( ptr = pid_cycles_list;  ptr != NULL;  ptr = ptr->link )
		{
		t = ((double)ptr->hi_cycles) * (((double)UINT_MAX) + 1.0) +
			((double)ptr->lo_cycles);
		total += t;
		}
	for( ptr = pid_cycles_list;  ptr != NULL;  ptr = ptr->link )
		{
		t = ((double)ptr->hi_cycles) * (((double)UINT_MAX) + 1.0) +
			((double)ptr->lo_cycles);
		printf("%9u %12.0f %10.2f%%\n", ptr->pid, t, t*100.0/total);
		ptot += t*100.0/total;
		}
	printf("%9s %12.0f %10.2f%%\n", "TOTAL", total, ptot);
	}


/* first probe for this pid, create a new stack */
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
	v3_ptr->v3_pos = 0;
	v3_ptr->v3_code[0] = 0;
	if( v3_stack_first == NULL )
		{/* this is the first stack ever created */
		v3_stack_first = v3_ptr;
		v3_ptr->v3_start_cycle[0] = start_time_glob;
		}
	else
		{/* initialize new stack with time of item at top of current stack */
		v3_ptr->v3_start_cycle[0] =
							v3_stack_ptr->v3_start_cycle[v3_stack_ptr->v3_pos];
		}

	v3_ptr->v3_next = v3_stack_head;	/* push on stack list */
	v3_stack_head = v3_ptr;
	return v3_ptr;
	}


void update_stack( unsigned int delta )
	{
	unsigned int	i;

	/* move up start time of all previous in nesting */
	for( i = 0;  i < v3_stack_ptr->v3_pos;  i++ )
		v3_stack_ptr->v3_start_cycle[i] += delta;

	/* then pop the stack */
	v3_stack_ptr->v3_pos--;
	fprintf(stdbug, "pop off v3 stack for id %u, top %d\n",
							v3_stack_ptr->v3_pid, v3_stack_ptr->v3_pos);
	}


void update_function( unsigned int delta, int i, int index )
	{
	int				j, k;


	j = -1;
	for( k = 0;  k < MAX_FUNS;  k++ )
		{
		if( i == v3_function_code[k] )
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
			fprintf(stderr,"=== no room for function entry code %04x\n",i);
			fprintf(stdout,"=== no room for function entry code %04x\n",i);
			return;
			}
		else
			{/* store this code here */
			v3_function_code[j] = i;
			k = j;
			}
		}

	v[index].total += delta;
	v3_function_cycles[k] += delta;
	v3_function_count[k] += 1;

	fprintf(stdbug, "update function %s by %u, total now %u\n",
								find_name(i,1), delta, v3_function_cycles[k]);
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

	/* initialize new entry to all zeroes except counter = 1 */
	v[v_pos].code = code; 
	v[v_pos].total = 0;
	v[v_pos].counter = 1;
	for( i = 0; i < MAX_FUNS; i++ )
		{
		v[v_pos].called_funs_cycles[i] = 0;
		v[v_pos].called_funs_ctr[i] = 0;
		}

	v_pos++;
	return k;
	}

void my_print_fun( unsigned int code, unsigned int cyc_time,
								unsigned int thisid, int param1, int param2 )
	{
    int					i, j, index;
    unsigned int		delta, off_cyc_time;

    
	off_cyc_time = cyc_time;

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
			off_cyc_time = v3_stack_ptr->v3_start_cycle[v3_stack_ptr->v3_pos];
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
					delta = off_cyc_time -
										v3_ptr->v3_start_cycle[v3_ptr->v3_pos];
					update_function(delta, FUT_SWITCH_TO_CODE, index);
					/* move up start of all previous in nesting and pop stack */
					update_stack(delta);
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
					v3_ptr->v3_start_cycle[0] =
							v3_stack_ptr->v3_start_cycle[v3_stack_ptr->v3_pos];
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


	/*	now guaranteed that v3_stack_ptr points to stack for this pid */
	/*	i gets code at top of this stack */
	i = v3_stack_ptr->v3_code[v3_stack_ptr->v3_pos];

	/* code for a function entry/exit, including switch_to */
	if( (code == i + FUT_GENERIC_EXIT_OFFSET) ||
		(code == FUT_SWITCH_TO_CODE  &&  i == FUT_SWITCH_TO_CODE
		&&  param1 == v3_stack_ptr->v3_switch_pid[v3_stack_ptr->v3_pos]) )
		{/* this is the exit corresponding to a previously stacked entry */
		/* or a switch back to previously stacked switch from */
		delta = cyc_time -
						v3_stack_ptr->v3_start_cycle[v3_stack_ptr->v3_pos];
		update_function(delta, i, index);
		if( code == i + FUT_GENERIC_EXIT_OFFSET )
			{/* this code is the exit corresponding to code i */
			if (v3_stack_ptr->v3_pos >= 1 )
				{
				j = v3_stack_ptr->v3_index[v3_stack_ptr->v3_pos - 1]; 
				v[j].called_funs_cycles[index] += delta;  
				}
			} 
		/* move up start time of all previous in nesting and pop stack */
		update_stack(delta);
		}
	else if( code != FUT_SETUP_CODE  &&
			code != FUT_KEYCHANGE_CODE &&
			code != FUT_RESET_CODE &&
			code != FUT_CALIBRATE0_CODE  &&
			code != FUT_CALIBRATE1_CODE  &&
			code != FUT_CALIBRATE2_CODE )
		{/* this must be an entry code, just stack it  */
		if( v3_stack_ptr->v3_pos >= MAX_NESTING )
			{
			fprintf(stderr, "=== v3 stack overflow, max depth %d\n",
													MAX_NESTING);
			fprintf(stdout, "=== v3 stack overflow, max depth %d\n",
													MAX_NESTING);
			exit(EXIT_FAILURE);
			}
		else
			{/* stack will not overflow, ok to push onto it */
			v3_stack_ptr->v3_pos++;
			v3_stack_ptr->v3_start_cycle[v3_stack_ptr->v3_pos] = cyc_time;
			v3_stack_ptr->v3_code[v3_stack_ptr->v3_pos] = code;
			v3_stack_ptr->v3_switch_pid[v3_stack_ptr->v3_pos] = thisid;
			v3_stack_ptr->v3_index[v3_stack_ptr->v3_pos] = index;
                
            if ( v3_stack_ptr->v3_pos >= 2 )
				{/* previous entry on this stack, update its calls to this fun*/
				j = v3_stack_ptr->v3_index[v3_stack_ptr->v3_pos - 1];
				v[j].called_funs_ctr[index]++;
				}
			v[index].counter++;
                
			fprintf(stdbug,
						"push on v3 stack for id %u, top %d, code %04x, "
						"index %d\n",
						thisid, v3_stack_ptr->v3_pos, code, index);
			}
		}

	previous.code = code;
	previous.cycle_value = cyc_time;
	previous.first_param = param1;
	}


/*	draw a horizontal line of 80 c characters onto stdout */
void my_print_line( int c )
	{
	int		i;

	for( i = 0;  i < 80;  i++ )
		putchar(c);
	putchar('\n');
	}


void draw_histogram( int n, double histo_max, char **histo_name,
							unsigned int *histo_cycles, double *histo_percent)
	{
	int				i, k, length;
	unsigned int	sum;

	printf("\n");
	printf("%22s  %4s %12s %8s\n\n", "Name", "Code", "Cycles", "Percent");
	sum = 0;
	for( k = 0;  k < n;  k++ )
		{
		printf("%28s %12u %7.2f%% ",
							histo_name[k], histo_cycles[k], histo_percent[k]);
		length = (int)((histo_percent[k]*MAX_HISTO_BAR_LENGTH)/histo_max + 0.5);
		for( i = 0;  i < length;  i++ )
			putchar(HISTO_CHAR);
		putchar('\n');
		sum += histo_cycles[k];
		}
	printf("%22s  %4s %12u %7.2f%%\n\n", "Total cycles", "", sum, 100.0);
	}


void my_print_fun_helper(unsigned int CycPerJiffy)
	{
	int						i, z, k;
	unsigned int			sum_other_cycles = 0, delta;
	unsigned int			TimeDiff, sum;
	double					Main_fun_sum, Main_fun_sum_percent;
	double					called_fun_percent, only_fun_percent;
	struct v3_stack_item	*v3_ptr;
	double					histo_max;
	char					*name;
	char					*histo_name[HISTO_SIZE];
	unsigned int			histo_cycles[HISTO_SIZE];
	double					histo_percent[HISTO_SIZE];


	printf("\n");
	printf ("%24s : %10u\n", "Cycle clock at start", start_time_glob);
	printf ("%24s : %10u\n", "Cycle clock at end", end_time_glob);
	TimeDiff = end_time_glob - start_time_glob;
	printf ("%24s : %10u\n", "Number of cycles elapsed", TimeDiff);
	printf("\n\n");

/*	print table of all function names */
if( v_pos > 0 )
	{
	printf("%30s  %4s %12s %10s %12s\n",
			"Name", "Code", "Cycles", "Count", "Avg");
	printf("%30s  %4s %12s %10s %12s\n",
			"----", "----", "------", "-----", "---");
	for ( i = 0 ; i < v_pos; i ++ )
		{
		if( v[i].code > 0  &&  v[i].code != FUT_SWITCH_TO_CODE )
			{
			printf("%30s  %04x %12u %10d %12.1f\n", 
				   find_name(v[i].code,0), v[i].code, v[i].total,
				   v[i].counter, (double)v[i].total/v[i].counter);
			}
		}

	printf("\n\n");
	printf("%4s %25s  %4s %12s %10s\n",
			"From", "To", "Code", "Cycles", "Count");
	printf("%4s %25s  %4s %12s %10s\n",
			"----", "--", "----", "------", "-----");
	for ( z = 0; z < v_pos; z++)
		if( v[z].code != FUT_SWITCH_TO_CODE )
			{
			printf("%s\n", find_name(v[z].code,0));
			for ( i = 0; i < v_pos; i++)
				{
				if(v[z].called_funs_ctr[i]!=0 && v[i].code!=FUT_SWITCH_TO_CODE) 
					{
					printf("%30s  %04x %12u %10d\n", 
							find_name(v[i].code,0), v[i].code,
							v[z].called_funs_cycles[i], 
							 v[z].called_funs_ctr[i]);
					}  
				}
			printf("\n");
			}

	/*	close up all open stacks */
	/*	stacks are typically left open when a process is blocked in the kernel*/
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
				{/* last thing thread did was not a block */
				fprintf(stderr,
				"=== top of final stack for id %u not switch_to\n",
																v3_ptr->v3_pid);
				fprintf(stdout,
				"=== top of final stack for id %u not switch_to\n",
																v3_ptr->v3_pid);
				}
			while( v3_ptr->v3_pos > 0 )
				{/* close up all unterminated function calls */
				unsigned int	code;

				code = v3_ptr->v3_code[v3_ptr->v3_pos-1];
				delta = v3_ptr->v3_start_cycle[v3_ptr->v3_pos] -
						v3_ptr->v3_start_cycle[v3_ptr->v3_pos-1];
				update_function(delta, code, get_code_index(code));
				v3_ptr->v3_pos -= 1;
				}
			}
		}

	printf("\n\n");
	printf("%50s\n\n", "NESTING SUMMARY");
	my_print_line('*');
	printf("%32s  %4s %11s %7s %12s %8s\n",
			"Name", "Code", "Cycles", "Count", "Average", "Percent");
	my_print_line('*');
	for ( z = 0 ; z < v_pos; z ++ )
		if( v[z].code != FUT_SWITCH_TO_CODE )
			{
			Main_fun_sum = v[z].total;
			Main_fun_sum_percent = 100.0;
			printf("%32s  %04x %11u %7d %12.1f %7.2f%%\n", 
				   find_name(v[z].code,0), v[z].code, v[z].total,
				   v[z].counter, (double)v[z].total/v[z].counter, 
							   Main_fun_sum_percent);

			for ( i = 0; i < v_pos; i++)
			{

				if(v[z].called_funs_ctr[i]!=0 && v[i].code!=FUT_SWITCH_TO_CODE) 
				{
				   called_fun_percent = (double)v[z].called_funs_cycles[i]
					   /Main_fun_sum * 100.0;
				   printf("%32s  %04x %11u %7d %12.1f %7.2f%%\n", 
							find_name(v[i].code,0), v[i].code,
							v[z].called_funs_cycles[i], 
							 v[z].called_funs_ctr[i], 
							 (double) v[z].called_funs_cycles[i]/
								v[z].called_funs_ctr[i], called_fun_percent );
				   sum_other_cycles += v[z].called_funs_cycles[i];
				}
			}
			/* for fsync, account for actual disk waiting time */
				only_fun_percent = (double)(v[z].total - sum_other_cycles)
								 /Main_fun_sum * 100.0;
			printf("%27s ONLY  %04x %11u %7d %12.1f %7.2f%%\n", 
					  find_name(v[z].code,0), v[z].code,
					  v[z].total - sum_other_cycles,
					  v[z].counter,
					  (double)(v[z].total - sum_other_cycles)
								/v[z].counter, only_fun_percent); 
				sum_other_cycles = 0;
			my_print_line('-');
			printf("\n");
			}
	}

	/* now print out the precise accounting statistics from the v3 arrays */
	sum = 0;
	k = 0;
	histo_max = 0.0;
	printf("\n\n\n");
	printf("%60s\n\n", "Accounting for every cycle exactly once");
	my_print_line('*');
	printf("%8s %23s  %4s %12s %7s %11s %8s\n",
	"", "Name", "Code", "Cycles", "Count", "Average", "Percent");
	my_print_line('*');
	for( i = 0;  i < MAX_FUNS; i++ )
		{
		z = v3_function_code[i];
		if( z > 0  &&  z != FUT_SWITCH_TO_CODE )
			{/* this occurred at least once and was not switch_to, print it */

			name = find_name(z,0);
			if( (histo_name[k] = malloc(strlen(name)+8)) == NULL )
				{
				fprintf(stderr, "no space to malloc histo_name\n");
				exit(EXIT_FAILURE);
				}
			sprintf(histo_name[k], "%s  %04x", name, z);
			histo_cycles[k] = v3_function_cycles[i];
			sum += histo_cycles[k];
			histo_percent[k] = 
						100.0*((double)histo_cycles[k])/(double)TimeDiff;
			if( histo_max < histo_percent[k] )
				histo_max = histo_percent[k];
			printf("%8s %23s", "function", name);
			printf("  %04x %12u %7u %11.1f %7.2f%%\n",
					z, histo_cycles[k], v3_function_count[i],
					((double)histo_cycles[k])/((double)v3_function_count[i]),
					histo_percent[k]);
			k++;
			}
		}


	printf("%8s %23s", "Total", "Total cycles");
	printf("  %4s %12u %7s %11s %7.2f%%\n", "", sum, "", "",
										100.0*((double)sum)/(double)TimeDiff);

	printf("\n\n%65s\n", "Histogram accounting for every cycle exactly once");
	draw_histogram(k, histo_max, histo_name, histo_cycles, histo_percent);
	}


void start_taking_stats( unsigned int thispid, unsigned int reltime )
	{
	fprintf(stdbug, "start taking stats, pid %u, rel time %u\n",
											thispid, reltime);
	fun_time = 1;							/* start the statistics taking */
	if( v3_stack_ptr != NULL )
		{
		fprintf(stderr, "=== starting v3_stack_ptr = %p\n",
														v3_stack_ptr);
		fprintf(stdout, "=== starting v3_stack_ptr = %p\n",
														v3_stack_ptr);
		exit(EXIT_FAILURE);
		}
	if( v3_stack_head != NULL )
		{
		fprintf(stderr, "=== starting v3_stack_head = %p\n",
														v3_stack_head);
		fprintf(stdout, "=== starting v3_stack_head = %p\n",
														v3_stack_head);
		exit(EXIT_FAILURE);
		}
	v3_stack_ptr = create_stack(thispid);
	}


void stop_taking_stats( unsigned int thispid, unsigned int reltime )
	{/* this times() occurred while taking statistics */
	fprintf(stdbug, "stop taking stats, pid %u, rel time %u\n",
											thispid, reltime);
	fun_time = -1;				/* stop the statistics taking */
	end_time_glob = reltime;	/* remember the stopping time */
	if( v3_stack_ptr == NULL )
		{
		fprintf(stderr,"=== stopping v3_stack_ptr = %p\n",v3_stack_ptr);
		fprintf(stdout,"=== stopping v3_stack_ptr = %p\n",v3_stack_ptr);
		exit(EXIT_FAILURE);
		}
	if( v3_stack_ptr->v3_pos != 0 )
		{
		fprintf(stderr, "=== stopping v3_pos = %u\n",
												v3_stack_ptr->v3_pos);
		fprintf(stdout, "=== stopping v3_pos = %u\n",
												v3_stack_ptr->v3_pos);
		}
	v3_stack_ptr->v3_start_cycle[0] = reltime - v3_stack_ptr->v3_start_cycle[0];
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
	unsigned int	code, fullcode, params, i, reltime, r, ptime;
	int				print_this = 0;


	if( bufptr[0] < basetime )
		{
		reltime = bufptr[0] + 0xffffffff - basetime;
		reltime++;
		}
	else
		reltime = bufptr[0] - basetime;
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
	else if( start_time_glob == 0 )
		{
		start_time_glob = reltime;
		fun_time = 1;				/* start the statistics taking */
		}
	lastreltime = reltime;
	print_this = 1;
    
    if (print_this)
		printf( "%10u%9lu", ptime, current_id);
	update_cycles(r, pid);

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
    if( fun_time > 0 )
		{
		if( params <= 4 )
        	my_print_fun(code, reltime, current_id, bufptr[3], 0);
		else
        	my_print_fun(code, reltime, current_id, bufptr[3], bufptr[4]);
		}
	if( code == FUT_SWITCH_TO_CODE )
		current_id = bufptr[3];
	return &bufptr[params];
	}

/*	returns 0 if ok, else -1 */
int dumpit( unsigned int *buffer, unsigned int nints )
	{
	unsigned int	n;
	unsigned int	*bufptr, *buflimit;


	printf( "%10s%9s %4s %22s", "cycles", "id", "code", "name");
	printf("%11s%11s%11s\n", "P1", "P2", "P3");


	buflimit = buffer + nints;
	basetime = buffer[0];
	lastreltime = 0;
	for( bufptr = buffer, n = 0;  n < nints && bufptr < buflimit; )
		{
		bufptr = dumpslot(&n, bufptr);
		}
	printf("\n");
	printf("%10u clock cycles\n", lastreltime);
	printf("\n");
	print_cycles(0);
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
		case 'd':
			stdbug = stdout;
			break;
		case 'c':
			print_delta_time = 0;
			break;
		case 'f':
			infile = optarg;
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
		v3_function_cycles[i] = 0;
		v3_function_count[i] = 0;
		v3_function_code[i] = 0;
		}

	dumpit(bufptr, nints);

	return 0;
	}
