#include "mpi.h"
#include <stdio.h>

/* Test the routines to control error handlers on windows */
static int calls = 0;
static int errs = 0;
static MPI_Win mywin;
static int expected_err_class = MPI_ERR_OTHER;

void weh( MPI_Win *win, int *err, ... )
{
  int errclass;
  MPI_Error_class( *err, &errclass );
  if (errclass != expected_err_class) {
    errs++;
    printf( "Unexpected error code (class = %d)\n", errclass );fflush(stdout);
  }
  if (*win != mywin) {
    errs++;
    printf( "Unexpected window (got %x expected %x)\n", (int)*win, (int)mywin );fflush(stdout);
  }
  calls++;
  return;
}

int main( int argc, char *argv[] )
{
  int err;
  int buf[2];
  MPI_Win       win;
  MPI_Comm      comm;
  MPI_Errhandler newerr, olderr;


  MPI_Init( &argc, &argv );
  comm = MPI_COMM_WORLD;
  MPI_Win_create_errhandler( weh, &newerr );

  MPI_Win_create( buf, 2*sizeof(int), sizeof(int), MPI_INFO_NULL, comm, &win );

  mywin = win;
  MPI_Win_get_errhandler( win, &olderr );
  if (olderr != MPI_ERRORS_ARE_FATAL) {
    errs++;
    printf( "Expected errors are fatal\n" );fflush(stdout);
  }

  MPI_Win_set_errhandler( win, newerr );
  expected_err_class = MPI_ERR_RANK;
  err = MPI_Put( buf, 1, MPI_INT, -5, 0, 1, MPI_INT, win );
  if (calls != 1) {
    errs ++;
    printf( "newerr not called\n" );fflush(stdout);
    calls = 1;
  }
  expected_err_class = MPI_ERR_OTHER;
  MPI_Win_call_errhandler( win, MPI_ERR_OTHER );
  if (calls != 2) {
    errs ++;
    printf( "newerr not called (2)\n" );fflush(stdout);
  }

  MPI_Win_free( &win );
  MPI_Errhandler_free( &newerr );

  MPI_Finalize();
  return errs;
}
