#ifndef TRACEBUFFER
#define TRACEBUFFER

/* Maximal number of arguments for a trace */
#define MAX_NB_ARGS 6

/* 64 bits integer type */
typedef unsigned long long int	u_64;

/* Trace modes : KERNEL or USER */
typedef enum {KERNEL, USER} mode;

/* A trace */
struct trace_st{
  u_64 clock;                      // time
  unsigned thread;                 // thread n°
  unsigned code;                   // code
  pid_t pid;                       // pid
  mode type;                       // KERNEL or USER
  short int cpu;                   // n°proc if KERNEL
  unsigned number;                 // Absolute number
  unsigned nbargs;
  unsigned args[MAX_NB_ARGS];      // Table of arguments
  short int relevant;
};

typedef struct trace_st trace;

//#define FKT_UNSHIFTED_LIMIT_CODE	        0x200

#define TRACE_BUFFER_SIZE 10000
#define EMPTY_LIST (trace_list) NULL

typedef struct trace_item_st * trace_list;

struct trace_item_st {
  trace tr;
  trace_list next;
  trace_list prev;
};

typedef struct {
  trace_list first;
  trace_list last;
} trace_buffer;

extern void init_trace_buffer(char *futin, char *fktin, int relative, int dec);

extern int get_next_trace(trace *tr);

extern void close_trace_buffer(void);

extern void supertrace_end(FILE *f);

#endif
