/*
                      PM2 HIGH-PERF/ISOMALLOC
           High Performance Parallel Multithreaded Machine
                           version 3.0

     Gabriel Antoniu, Olivier Aumage, Luc Bouge, Vincent Danjean,
       Christian Perez, Jean-Francois Mehaut, Raymond Namyst

            Laboratoire de l'Informatique du Parallelisme
                        UMR 5668 CNRS-INRIA
                 Ecole Normale Superieure de Lyon

                      External Contributors:
                 Yves Denneulin (LMC - Grenoble),
                 Benoit Planquelle (LIFL - Lille)

                    1998 All Rights Reserved


                             NOTICE

 Permission to use, copy, modify, and distribute this software and
 its documentation for any purpose and without fee is hereby granted
 provided that the above copyright notice appear in all copies and
 that both the copyright notice and this permission notice appear in
 supporting documentation.

 Neither the institutions (Ecole Normale Superieure de Lyon,
 Laboratoire de L'informatique du Parallelisme, Universite des
 Sciences et Technologies de Lille, Laboratoire d'Informatique
 Fondamentale de Lille), nor the Authors make any representations
 about the suitability of this software for any purpose. This
 software is provided ``as is'' without express or implied warranty.
*/

#define USE_MARCEL_POLL
#define USE_DYN_ARRAYS

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>

#include <mpi.h>

#include "sys/netinterf.h"

#include "tbx.h"

#ifdef PM2
#include "marcel.h"
#else
#define mpi_lock()
#define mpi_unlock()
static char *PROGRAM_ERROR = "PROGRAM_ERROR";
#define RAISE(e) fprintf(stderr, "%s\n", e), exit(1)
#endif

enum { HEADER_TAG, DATA_TAG };

static long header[MAX_HEADER/sizeof(long)];

#ifdef PM2
static marcel_mutex_t mpi_mutex, mutex[MAX_MODULES];

static __inline__ void mpi_lock()
{
  marcel_mutex_lock(&mpi_mutex);
}

static __inline__ void mpi_unlock()
{
  marcel_mutex_unlock(&mpi_mutex);
}

static __inline__ int mpi_trylock()
{
  return marcel_mutex_trylock(&mpi_mutex);
}

/* Polling */
#ifdef USE_MARCEL_POLL

#define MAX_MPI_REQ 64

static marcel_pollid_t mpi_io_pollid;

typedef struct {
  MPI_Request req;
  marcel_pollinst_t pollinst;
  MPI_Status status;
} mpi_io_arg_t;

static unsigned nb_mpi_req;
static MPI_Request mpi_req_array[MAX_MPI_REQ];
static mpi_io_arg_t *mpi_arg_array[MAX_MPI_REQ];

static void mpi_io_group(marcel_pollid_t id)
{
  mpi_io_arg_t *myarg;

  nb_mpi_req = 0;

  FOREACH_POLL(id, myarg) {
    myarg->pollinst = GET_CURRENT_POLLINST(id);
    mpi_req_array[nb_mpi_req] = myarg->req;
    mpi_arg_array[nb_mpi_req] = myarg;
    nb_mpi_req++;
  }
  mdebug("Nb of MPI polling requests = %d\n", nb_mpi_req);
}

static void *mpi_io_fast_poll(marcel_pollid_t id,
			      any_t arg,
			      boolean first_call)
{
  mpi_io_arg_t *myarg = (mpi_io_arg_t *)arg;
  int flag;

  if(mpi_trylock()) {
    MPI_Test(&myarg->req, &flag, &myarg->status);
    mpi_unlock();

    if(flag != 0) {
      mdebug("Fast Poll Success !! (self = %p)\n", marcel_self());
      return (void *)1;
    }
  }
  return MARCEL_POLL_FAILED;
}

static void *mpi_io_poll(marcel_pollid_t id,
			 unsigned active, unsigned sleeping, unsigned blocked)
{
  int flag, index;
  MPI_Status status;
  
  mdebug("MPI Polling function called on LWP %d (%2d A, %2d S, %2d B)\n",
	 marcel_current_vp(), active, sleeping, blocked);

  if(mpi_trylock()) {
    /* Ok, nobody is currently using MPI */

    mdebug("MPI Polling function successfuly acquired the mpi lock\n");

    MPI_Testany(nb_mpi_req, mpi_req_array, &index, &flag, &status);

    mpi_unlock();

    if(flag == 0) {
      mdebug("MPI Polling function failed\n");
      return MARCEL_POLL_FAILED;
    }

    mpi_arg_array[index]->status = status;

    mdebug("MPI Polling function succeded\n");
    return MARCEL_POLL_SUCCESS_FOR(mpi_arg_array[index]->pollinst);
  } else {
    mdebug("MPI Polling function could not access MPI -> failed\n");
    return MARCEL_POLL_FAILED;
  }
}

#endif /* USE_MARCEL_POLL */

#endif /* PM2 */

static void mad_mpi_send(void *buf, int cnt, MPI_Datatype type, int dest,
			 int tag, MPI_Comm comm, MPI_Status *status)
{
#if defined(PM2) && defined(USE_MARCEL_POLL)

  mpi_io_arg_t myarg;

  LOG_IN();

  mpi_lock();
  MPI_Isend(buf, cnt, type, dest, tag,
	    comm, &myarg.req);
  mpi_unlock();

  marcel_poll(mpi_io_pollid, (any_t)&myarg);

  *status = myarg.status;

#else

  int flag;
  MPI_Request request;

  LOG_IN();

  mpi_lock();
  MPI_Isend(buf, cnt, type, dest, tag, comm, &request);
  MPI_Test(&request, &flag, status);
  mpi_unlock();

  while(flag == 0) {
#ifdef PM2
    marcel_givehandback();
#endif
    mpi_lock();
    MPI_Test(&request, &flag, status);
    mpi_unlock();
  }

#endif

  LOG_OUT();
}

static void mad_mpi_recv(void *buf, int cnt, MPI_Datatype type, int src,
			 int tag, MPI_Comm comm, MPI_Status *status)
{
#if defined(PM2) && defined(USE_MARCEL_POLL)

  mpi_io_arg_t myarg;

  LOG_IN();

  mpi_lock();
  MPI_Irecv(buf, cnt, type, src, tag,
	    comm, &myarg.req);
  mpi_unlock();

  marcel_poll(mpi_io_pollid, (any_t)&myarg);

  *status = myarg.status;

#else

  MPI_Request request;
  int flag;

  LOG_IN();

  mpi_lock();
  MPI_Irecv(buf, cnt, type, src, tag, comm, &request);
  MPI_Test(&request, &flag, status);
  mpi_unlock();

  while(flag == 0) {
#ifdef PM2
    marcel_givehandback();
#endif
    mpi_lock();
    MPI_Test(&request, &flag, status);
    mpi_unlock();
  }

#endif
  LOG_OUT();
}

void mad_mpi_network_init(int *argc, char **argv, int nb_proc, int *tids, int *nb, int *whoami)
{
  int i, f;
  char output[64];

  LOG_IN();

  MPI_Init(argc, &argv);

  MPI_Comm_rank(MPI_COMM_WORLD, whoami);
  MPI_Comm_size(MPI_COMM_WORLD, nb);

  if(*whoami) {

#ifdef PM2
    sprintf(output, "/tmp/%s-pm2log-%d", getenv("USER"), *whoami);
#else
    sprintf(output, "/tmp/%s-madlog-%d", getenv("USER"), *whoami);
#endif
    f = open(output, O_WRONLY | O_CREAT | O_TRUNC, 0600);

    dup2(f, STDOUT_FILENO);
    dup2(STDOUT_FILENO, STDERR_FILENO);
    close(f);
  }

  if(tids != NULL) 
    for(i = 0; i < *nb; i ++)
      tids[i] = i;

#ifdef PM2
  marcel_mutex_init(&mpi_mutex, NULL);
  for(i=0; i<*nb; i++)
    marcel_mutex_init(&mutex[i], NULL);
#ifdef USE_MARCEL_POLL
  mpi_io_pollid = marcel_pollid_create(mpi_io_group,
				       mpi_io_poll,
				       mpi_io_fast_poll, /* Fast Poll */
				       MARCEL_POLL_AT_TIMER_SIG);
#endif
#endif
  LOG_OUT();
}

void mad_mpi_network_send(int dest_node, struct iovec *vector, size_t count)
{
  MPI_Status send_status;
  MPI_Datatype iovec_type;
#ifdef USE_DYN_ARRAYS
  int blocklength[count];
  MPI_Aint displacement[count];
#else
  int *blocklength;
  MPI_Aint *displacement;
#endif
  int i;

  LOG_IN();

  if(count == 0)
    RAISE(PROGRAM_ERROR);

#ifdef PM2
  marcel_mutex_lock(&mutex[dest_node]);
#endif

  /* We first have to send the header */
  mad_mpi_send(vector[0].iov_base, vector[0].iov_len, MPI_BYTE, dest_node, HEADER_TAG,
	       MPI_COMM_WORLD, &send_status);

  /* Ok, now let us send the body */

  if(count == 2) {

    mad_mpi_send(vector[1].iov_base, vector[1].iov_len, MPI_BYTE, dest_node, DATA_TAG,
		 MPI_COMM_WORLD, &send_status);

  } else if(count > 1) {

#ifndef USE_DYN_ARRAYS
    blocklength = (int *)TBX_MALLOC(count*sizeof(int));
    displacement = (MPI_Aint *)TBX_MALLOC(count*sizeof(MPI_Aint));
#endif

    mpi_lock();
    for(i=1; i<count; i++) {
      MPI_Address(vector[i].iov_base, displacement+i-1);
      blocklength[i-1] = vector[i].iov_len;
    }
    MPI_Type_hindexed(count-1, blocklength, displacement, MPI_BYTE, &iovec_type);
    MPI_Type_commit(&iovec_type);
    mpi_unlock();

    mad_mpi_send(MPI_BOTTOM, 1, iovec_type, dest_node, DATA_TAG,
		 MPI_COMM_WORLD, &send_status);

    mpi_lock();
    MPI_Type_free(&iovec_type);
    mpi_unlock();

#ifndef USE_DYN_ARRAYS
    TBX_FREE(blocklength);
    TBX_FREE(displacement);
#endif
  }
#ifdef PM2
  marcel_mutex_unlock(&mutex[dest_node]);
#endif
  LOG_OUT();
}

static int current_expeditor;

void mad_mpi_network_receive(char **head)
{
  MPI_Status receive_status;

  LOG_IN();

  mad_mpi_recv(header, MAX_HEADER, MPI_BYTE, MPI_ANY_SOURCE, HEADER_TAG,
	       MPI_COMM_WORLD, &receive_status);

  current_expeditor = receive_status.MPI_SOURCE;

  *head = (char *)header;

  LOG_OUT();
}

void mad_mpi_network_receive_data(struct iovec *vector, size_t count)
{
  int i;
  MPI_Datatype iovec_type;
  MPI_Status receive_status;
#ifdef USE_DYN_ARRAYS
  int blocklength[count];
  MPI_Aint displacement[count];
#else
  int *blocklength;
  MPI_Aint *displacement;
#endif

  LOG_IN();

  if(count == 0) {
    LOG_OUT();
    return;
  }

  if(count == 1) {

    mad_mpi_recv(vector[0].iov_base, vector[0].iov_len, MPI_BYTE, current_expeditor, DATA_TAG,
		 MPI_COMM_WORLD, &receive_status);

  } else {

#ifndef USE_DYN_ARRAYS
    blocklength = (int *)TBX_MALLOC(count*sizeof(int));
    displacement = (MPI_Aint *)TBX_MALLOC(count*sizeof(MPI_Aint));
#endif

    mpi_lock();
    for(i=0; i<count; i++) {
      MPI_Address(vector[i].iov_base, displacement+i);
      blocklength[i] = vector[i].iov_len;
    }

    MPI_Type_hindexed(count, blocklength, displacement, MPI_BYTE, &iovec_type);
    MPI_Type_commit(&iovec_type);
    mpi_unlock();

    mad_mpi_recv(MPI_BOTTOM, 1, iovec_type, current_expeditor, DATA_TAG,
		 MPI_COMM_WORLD, &receive_status);

    mpi_lock();
    MPI_Type_free(&iovec_type);
    mpi_unlock();

#ifndef USE_DYN_ARRAYS
    TBX_FREE(blocklength);
    TBX_FREE(displacement);
#endif
  }

  LOG_OUT();
}

void mad_mpi_network_exit()
{
  LOG_IN();

  MPI_Finalize();

  LOG_OUT();
}

static netinterf_t mad_mpi_netinterf = {
  mad_mpi_network_init,
  mad_mpi_network_send,
  mad_mpi_network_receive,
  mad_mpi_network_receive_data,
  mad_mpi_network_exit
};

netinterf_t *mad_mpi_netinterf_init()
{
  return &mad_mpi_netinterf;
}

