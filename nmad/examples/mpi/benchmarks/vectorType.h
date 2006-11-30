#ifndef VECTOR_TYPE_H
#define VECTOR_TYPE_H

/**
 * This function sends from the process with rank 0 data with the vector type to all the process with rank > 0
 */
int sendVectorType(int argc, char *argv[], int rank, int numtasks);

//void processAndsendVectorType(int size, int blocks, int rank, int numtasks, int use_hvector);

void sendVectorTypeFromSrcToDest(int size, int blocks, int rank, int source, int dest, int numtasks, int use_hvector, int display);

#endif // VECTOR_TYPE_H
