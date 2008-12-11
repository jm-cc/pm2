#include <cblas.h>
#include <stdio.h>
#include <stdlib.h>

#define N	1024

int main(int argc, char **argv) {
  float *matA, *matB, *matC;
  unsigned i,j;

  matA = malloc(N*N*sizeof(float));
  matB = malloc(N*N*sizeof(float));
  matC = malloc(N*N*sizeof(float));

  for (j = 0; j < N; j++) {
    for (i = 0; i < N; i++) {
      matA[i+N*j] = 1.0f;
      matB[i+N*j] = 1.0f;
      matC[i+N*j] = 1.0;
    }
  }

  cblas_sgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans, N, N, N,
              1.0f,
              matA, N,
              matB, N,
              0.0f,
              matC, N);

  printf("matA[0] = %f\n", matA[0+0]);
  printf("matB[0] = %f\n", matB[0+0]);
  printf("matC[0] = %f\n", matC[0+0]);
  
  return 0;
}
