/*
 * NewMadeleine
 * Copyright (C) 2006 (see AUTHORS file)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

#include "toolbox.h"
#include <mpi.h>

int checkArguments(int argc, char **argv, int startPos, int *use_hindex, int *short_message, int *minSize, int *maxSize, int *stride, int *blocks, int *size, char *tests) {
  int i=startPos;
#ifdef MPICL
  char *mpiclFileName;

  mpiclFileName = (char *) malloc(100 * sizeof(char));
  mpiclFileName = getenv("MPICL_FILE");
  if (mpiclFileName != NULL) {
    /* enable tracing */
    MPI_Pcontrol(TRACEFILES, "", mpiclFileName, 0);
    MPI_Pcontrol(TRACELEVEL, 1, 1, 1);
    MPI_Pcontrol(TRACENODE, 1000000, 0, 1);
  }
#endif // MPICL

  while (i<argc) {
    if (strcmp(argv[i], "-hindex") == 0) {
      *use_hindex = TRUE;
    }
    else if (strcmp(argv[i], "-hvector") == 0) {
      *use_hindex = TRUE;
    }
    else if (strcmp(argv[i], "-short") == 0) {
      *short_message = TRUE;
    }
    else if (strcmp(argv[i], "-long") == 0) {
      *short_message = FALSE;
    }
    else if (strcmp(argv[i], "-blocks") == 0) {
      if (i+1 >= argc) {
        printf("Argument missing for -blocks\n");
        return 1;
      }
      *blocks = atoi(argv[i+1]);
      i++;
    }
    else if (strcmp(argv[i], "-min") == 0) {
      if (i+1 >= argc) {
        printf("Argument missing for -min\n");
        return 1;
      }
      *minSize = atoi(argv[i+1]);
      i++;
    } // end else
    else if (strcmp(argv[i], "-max") == 0) {
      if (i+1 >= argc) {
        printf("Argument missing for -max\n");
        return 1;
      }
      *maxSize = atoi(argv[i+1]);
      i++;
    } // end else
    else if (strcmp(argv[i], "-stride") == 0) {
      if (i+1 >= argc) {
        printf("Argument missing for -stride\n");
        return 1;
      }
      *stride = atoi(argv[i+1]);
      i++;
    } // end else
    else if (strcmp(argv[i], "-size") == 0) {
      if (i+1 >= argc) {
        printf("Argument missing for -size\n");
        return 1;
      }
      *size = atoi(argv[i+1]);
      i++;
    } // end else
    else if (strcmp(argv[i], "-tests") == 0) {
      if (i+1 >= argc) {
        printf("Argument missing for -tests\n");
        return 1;
      }
      strcpy(tests, argv[i+1]);
      i++;
    } // end else
    else {
      printf("%s unrecognized argument\n", argv[i]);
    }
    i++;
  }
  return 0;
}

