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

#ifndef VECTOR_TYPE_H
#define VECTOR_TYPE_H

/**
 * This function sends from the process with rank 0 data with the vector type to all the process with rank > 0
 */
int sendVectorType(int argc, char *argv[], int rank, int numtasks);

void processAndsendVectorType(int size, int blocks, int rank, int numtasks, int use_hvector, int display);

void sendVectorTypeFromSrcToDest(int size, int blocks, int rank, int source, int dest, int use_hvector, int display);

float getValue(int x, int y, int size);

void getIndices(int displacement, int size, int initialValue, int *x, int *y);

/** check the data is correct */
void checkVectorIsCorrect(float *b, int rank, int count, int blocklength, int size, int stride);

int getRealSize(int size, int blocks);

#endif // VECTOR_TYPE_H
