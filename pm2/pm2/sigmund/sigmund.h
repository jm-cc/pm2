#ifndef SIGMUND
#define SIGMUND

#define MAX_NB_ARGS 6

typedef unsigned long long int	u_64;

struct code_name {
  unsigned int    code;
  char           *name;
};

typedef enum {KERNEL, USER} mode;

extern struct code_name code_table[1000];


struct trace_st{
  u_64 clock;
  int thread;
  int code;
  int pid;
  mode type;
  short int proc;

  int args[MAX_NB_ARGS];
};

typedef struct trace_st trace;

extern int nb_code;

#endif
