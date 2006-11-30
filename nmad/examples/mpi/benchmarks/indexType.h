#ifndef INDEX_TYPE_H
#define INDEX_TYPE_H

/**
 * This function sends from the process with rank 0 data with the index type to all the process with rank > 0
 */
int sendIndexType(int argc, char *argv[], int rank, int numtasks);

void sendIndexTypeFromSrcToDest(int numberOfElements, int blocks, int rank, int source, int dest, int numtasks, int use_hindex, int display);

#endif // INDEX_TYPE_H
