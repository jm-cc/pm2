#ifndef SIGMUND
#define SIGMUND

/* Maximal nulmber of arguments for a trace */
#define MAX_NB_ARGS 6

/* 64 bits integer type */
typedef unsigned long long int	u_64;

/* Structure to record codes and their names */
struct code_name {
  unsigned int    code;
  char           *name;
};

/* Trace modes : KERNEL or USER */
typedef enum {KERNEL, USER} mode;

/* Table of codes : fut */
struct code_name code_table[1000];
/* Number of code */
int nb_code;

/* A trace */
struct trace_st{
  u_64 clock;                      // time
  int thread;                      // thread n°
  int code;                        // code
  int pid;                         // pid
  mode type;                       // KERNEL or USER
  short int cpu;                  // n°proc if KERNEL
  unsigned int number;                      // Absolute number
  int args[MAX_NB_ARGS];           // Table of arguments
};

typedef struct trace_st trace;

#define FKT_UNSHIFTED_LIMIT_CODE	        0x200

#endif
