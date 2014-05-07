#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <tbx.h>

#define MAX_LEN (8 * 1024 * 1024)
#define MIN_COMPUTE 0
#define MAX_COMPUTE 10000
//#define MIN_COMPUTE 500
//#define MAX_COMPUTE 501
#define COUNT 1000

volatile double r = 1.0;

void do_compute(int usec)
{
  tbx_tick_t t1, t2;
  double delay = 0.0;
  TBX_GET_TICK(t1);
  while(delay < usec)
    {
      int k;
      for(k = 0; k < 10; k++)
	{
	  r *= 2.213890;
	}
      TBX_GET_TICK(t2);
      delay = TBX_TIMING_DELAY(t1, t2);
    }
}

static int comp_double(const void*_a, const void*_b)
{
  const double*a = _a;
  const double*b = _b;
  if(*a < *b)
    return -1;
  else if(*a > *b)
    return 1;
  else
    return 0;
}

int main(int argc, char**argv)
{
  /*  int rc = MPI_Init(&argc, &argv); */
  const int requested = MPI_THREAD_FUNNELED;
  int provided = requested;
  int rc = MPI_Init_thread(&argc, &argv, requested, &provided);
  if(rc != 0)
    {
      fprintf(stderr, "MPI_Init rc = %d\n", rc);
      abort();
    }
  if(provided != requested)
    {
      fprintf(stderr, "MPI_Init_thread- requested = %d; provided = %d\n", MPI_THREAD_MULTIPLE, provided);
      abort();
    }
  int self = -1;
  MPI_Comm_rank(MPI_COMM_WORLD, &self);
  const int peer = 1 - self;
  const int tag = 0;
  char small = 1;

  double*lats_bidir = malloc(sizeof(double) * COUNT);
  double*lats_send = malloc(sizeof(double) * COUNT);
  double*lats_recv = malloc(sizeof(double) * COUNT);
  char*buffer = malloc(MAX_LEN);

  tbx_init(&argc, &argv);

  int len;
  for(len = 1; len <= MAX_LEN; len *= 2)
    {
      int c;
      if(self == 0)
	{
	  printf("# len = %d\n", len);
	  printf("# size  \t|  compute\t|  best (usec.)\t|  median \t|   max \t|  send ov. \t| recv ov. \t| roundtrips\n");
	}
      for(c = MIN_COMPUTE; c < MAX_COMPUTE; c = 1 + c * 1.2)
	{

	  const double decay = 1.0 - (double)c / (double)MAX_COMPUTE;
	  const int iterations = 3 + decay * decay * decay * decay * (COUNT - 3);

	  if(self == 0)
	    {
	      int k;
	      for(k = 0; k < iterations; k++)
		{
		  MPI_Request sreq, rreq;
		  MPI_Status status;
		  tbx_tick_t t1, t2;
		  /* bi-directionnal overlap */
		  MPI_Barrier(MPI_COMM_WORLD);
		  TBX_GET_TICK(t1);
		  MPI_Isend(buffer, len, MPI_CHAR, peer, tag, MPI_COMM_WORLD, &sreq);
		  do_compute(c);
		  MPI_Wait(&sreq, &status);
		  MPI_Irecv(buffer, len, MPI_CHAR, peer, tag, MPI_COMM_WORLD, &rreq);
		  do_compute(c);
		  MPI_Wait(&rreq, &status);
		  TBX_GET_TICK(t2);
		  lats_bidir[k] = TBX_TIMING_DELAY(t1, t2) / 2.0;
 
		  /* sender-side overlap */
		  MPI_Barrier(MPI_COMM_WORLD);
		  TBX_GET_TICK(t1);
		  MPI_Isend(buffer, len, MPI_CHAR, peer, tag, MPI_COMM_WORLD, &sreq);
		  do_compute(c);
		  MPI_Wait(&sreq, &status);
		  MPI_Recv(&small, 1, MPI_CHAR, peer, tag, MPI_COMM_WORLD, &status);
		  TBX_GET_TICK(t2);
		  lats_send[k] = TBX_TIMING_DELAY(t1, t2);

		  /* recv-side overlap */
		  MPI_Barrier(MPI_COMM_WORLD);
		  TBX_GET_TICK(t1);
		  MPI_Send(&small, 1, MPI_CHAR, peer, tag, MPI_COMM_WORLD);
		  MPI_Irecv(buffer, len, MPI_CHAR, peer, tag, MPI_COMM_WORLD, &rreq);
		  do_compute(c);
		  MPI_Wait(&rreq, &status);
		  TBX_GET_TICK(t2);
		  lats_recv[k] = TBX_TIMING_DELAY(t1, t2);
		}
	      qsort(lats_bidir, iterations, sizeof(double), &comp_double);
	      qsort(lats_send, iterations, sizeof(double), &comp_double);
	      qsort(lats_recv, iterations, sizeof(double), &comp_double);
	      const double min_lat = lats_bidir[0];
	      const double max_lat = lats_bidir[iterations - 1];
	      const double med_lat = lats_bidir[(iterations - 1) / 2];

	      const double med_send = lats_send[(iterations - 1) / 2];
	      const double med_recv = lats_recv[(iterations - 1) / 2];

	      printf("%8d\t%8d \t %8.2f\t%8.2f\t%8.2f\t%8.2f\t%8.2f\t%d\n", len, c, min_lat, med_lat, max_lat, med_send, med_recv, iterations);
	      fflush(stdout);
	    }
	  else
	    {
	      int k;
	      for(k = 0; k < iterations; k++)
		{
		  MPI_Request sreq, rreq;
		  MPI_Status status;

		  /* bi-directionnal overlap */
		  MPI_Barrier(MPI_COMM_WORLD);
		  MPI_Irecv(buffer, len, MPI_CHAR, peer, tag, MPI_COMM_WORLD, &rreq);
		  do_compute(c);
		  MPI_Wait(&rreq, &status);
		  MPI_Isend(buffer, len, MPI_CHAR, peer, tag, MPI_COMM_WORLD, &sreq);
		  do_compute(c);
		  MPI_Wait(&sreq, &status);

		  /* sender-side overlap */
		  MPI_Barrier(MPI_COMM_WORLD);
		  MPI_Recv(buffer, len, MPI_CHAR, peer, tag, MPI_COMM_WORLD, &status);
		  MPI_Send(&small, 1, MPI_CHAR, peer, tag, MPI_COMM_WORLD);

		  /* recv-side overlap */
		  MPI_Barrier(MPI_COMM_WORLD);
		  MPI_Recv(&small, 1, MPI_CHAR, peer, tag, MPI_COMM_WORLD, &status);
		  MPI_Send(buffer, len, MPI_CHAR, peer, tag, MPI_COMM_WORLD);
		}
	    }
	}
    }
  free(lats_bidir);
  free(lats_send);
  free(lats_recv);
  free(buffer);
  MPI_Finalize();
  return 0;
}


