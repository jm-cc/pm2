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

#ifndef STRUCT_TYPE_H
#define STRUCT_TYPE_H

/**
 * This function sends from the process with rank 0 data with the struct type to all the process with rank > 0
 */
int sendStructType(int argc, char *argv[], int rank, int numtasks);

void sendStructTypeFromSrcToDest(int numberOfElements, int rank, int source, int dest, int display);

typedef struct {
  float x, y;
  int c;
  float z;
} Particle;

void checkStructIsCorrect(Particle *subparticle, int size, int rank);

void processAndSendStructType(int size, int rank, int numtasks, int display);

#endif // STRUCT_TYPE_H
