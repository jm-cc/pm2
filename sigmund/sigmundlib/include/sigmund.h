#ifndef _SIGMUND_H
#define _SIGMUND_H

/* Maximal nulmber of arguments for a trace */
#define MAX_NB_ARGS 6

#include <fkt/names.h>

/* 64 bits integer type */
typedef unsigned long long int	u_64;

/* Trace modes : KERNEL or USER */
typedef enum {KERNEL, USER} mode;

/* Table of codes : fut */
extern struct fkt_code_name code_table[1000];
/* Number of code */
extern unsigned nb_code;

/* A trace */
struct trace_st{
  u_64 clock;                      // time
  unsigned thread;                 // thread n°
  unsigned code;                   // code
  unsigned pid;                    // pid
  mode type;                       // KERNEL or USER
  short int cpu;                   // n°proc if KERNEL
  unsigned int number;                      // Absolute number
  unsigned nbargs;
  unsigned args[MAX_NB_ARGS];           // Table of arguments
};

typedef struct trace_st trace;

#endif
