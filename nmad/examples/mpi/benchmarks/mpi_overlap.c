#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <tbx.h>

#define LEN (512 * 1024)
#define COUNT 50

static char buffer[LEN];

void calcul(int usec)
{
  tbx_tick_t t1, t2;
  double delay = 0.0;
  TBX_GET_TICK(t1);
  while(delay < usec)
    {
      TBX_GET_TICK(t2);
      delay = TBX_TIMING_DELAY(t1, t2);
    }
}

int main(int argc, char**argv)
{
  int self = -1;
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &self);
  int peer = 1 - self;
  const int tag = 0;
  char small = 1;

  int c;
  printf("# computation (usec.) | total time (usec.)\n");
  for(c = 0; c < 10000; c = 1 + c*1.2)
    {
      MPI_Barrier(MPI_COMM_WORLD);
      if(self == 0)
	{
	  tbx_tick_t t1, t2;
	  TBX_GET_TICK(t1);
	  int k;
	  for(k = 0; k < COUNT; k++)
	    {
	      MPI_Request sreq, rreq;
	      MPI_Status status;
	      MPI_Irecv(&small, 1, MPI_CHAR, peer, tag, MPI_COMM_WORLD, &rreq);
	      MPI_Isend(buffer, LEN, MPI_CHAR, peer, tag, MPI_COMM_WORLD, &sreq);
	      calcul(c);
	      MPI_Wait(&sreq, &status);
	      MPI_Wait(&rreq, &status);
	    }
	  TBX_GET_TICK(t2);
	  double t = TBX_TIMING_DELAY(t1, t2) / COUNT;
	  printf("%d \t %f\n", c, t);
	}
      else
	{
	  int k;
	  for(k = 0; k < COUNT; k++)
	    {
	      MPI_Status status;
	      MPI_Recv(buffer, LEN, MPI_CHAR, peer, tag, MPI_COMM_WORLD, &status);
	      MPI_Send(&small, 1, MPI_CHAR, peer, tag, MPI_COMM_WORLD);
	    }
	}
    }
  MPI_Finalize();
  return 0;
}


