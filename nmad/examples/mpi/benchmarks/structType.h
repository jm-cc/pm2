#ifndef STRUCT_TYPE_H
#define STRUCT_TYPE_H

/**
 * This function sends from the process with rank 0 data with the struct type to all the process with rank > 0
 */
int sendStructType(int argc, char *argv[], int rank, int numtasks);

void sendStructTypeFromSrcToDest(int numberOfElements, int rank, int source, int dest, int numtasks, int display);

#endif // STRUCT_TYPE_H
