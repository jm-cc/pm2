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

#ifndef INDEX_TYPE_H
#define INDEX_TYPE_H

/**
 * This function sends from the process with rank 0 data with the index type to all the process with rank > 0
 */
int sendIndexType(int argc, char *argv[], int rank, int numtasks);

void sendIndexTypeFromSrcToDest(int numberOfElements, int blocks, int rank, int source, int dest, int use_hindex, int display);

void processAndSendIndexType(int size, int blocks, int rank, int numtasks, int use_hindex, int display);

/** Check the elements have been properly received */
void checkIndexIsCorrect(float *vector, int rank, int numberOfElements);

#endif // INDEX_TYPE_H
