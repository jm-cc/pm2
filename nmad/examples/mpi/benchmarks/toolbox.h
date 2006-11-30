#ifndef TOOLBOX_H
#define TOOLBOX_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef MPICL
#include "pcontrol.h"
#endif // MPICL

#define TAG           1
#define TRUE          1
#define FALSE         0
#define PRINT_SUCCESS 0
#define VERBOSE       0

#define SMALL_SIZE     1000
#define LONG_SIZE      40000
#define DEFAULT_BLOCKS 3

#define ASYNC

typedef enum {
  MPIR_INT,                 // 0
  MPIR_FLOAT,               // 1
  MPIR_DOUBLE,              // 2
  MPIR_COMPLEX,             // 3
  MPIR_LONG,                // 4
  MPIR_SHORT,               // 5
  MPIR_CHAR,                // 6
  MPIR_BYTE,                // 7
  MPIR_UCHAR,               // 8
  MPIR_USHORT,              // 9
  MPIR_ULONG,               // 10
  MPIR_UINT,                // 11
  MPIR_CONTIG,              // 12
  MPIR_VECTOR,              // 13
  MPIR_HVECTOR,             // 14
  MPIR_INDEXED,             // 15
  MPIR_HINDEXED,            // 16
  MPIR_STRUCT,              // 17
  MPIR_DOUBLE_COMPLEX,      // 18
  MPIR_PACKED,              // 19
  MPIR_UB,                  // 20
  MPIR_LB,                  // 21
  MPIR_LONGDOUBLE,          // 22
  MPIR_LONGLONGINT,         // 23
  MPIR_LOGICAL,             // 24
  MPIR_FORT_INT             // 25
} MPIR_NODETYPE;

int checkArguments(int argc, char **argv, int startPos, int *use_hindex, int *short_message, int *minSize, int *maxSize, int *stride, int *blocks, int *size, char *tests);

#ifdef MPICH_PM2
#define PRINT(str, ...)        mad_leonie_print(str, ## __VA_ARGS__)
#define PRINT_NO_NL(str, ...)  mad_leonie_print_without_nl(str, ## __VA_ARGS__)
#else // MPICH_PM2
#define PRINT(str, ...)        fprintf(stderr, str "\n", ## __VA_ARGS__)
#define PRINT_NO_NL(str, ...)  fprintf(stderr, str, ## __VA_ARGS__)
#endif // MPICH_PM2

#endif // TOOLBOX_H
