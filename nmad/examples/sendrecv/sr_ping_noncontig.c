#include <err.h>
#include <stdio.h>
#include <nm_public.h>
#include <nm_launcher.h>
#include <nm_sendrecv_interface.h>
#include <sys/time.h>

#define PAGESIZE 4096

static int rank, peer;
static nm_session_t p_session = NULL;
static nm_gate_t gate_id = NULL;
char * sbuf;
char * rbuf;
struct iovec * riov;
struct iovec * siov;
int size, iter;
int count;

#define SIZE 4 * 1024 *1024
#define ITER 1000

void pingpong(int iter)
{
  int i;
  nm_sr_request_t sreq, rreq;
   
  for(i = 0; i < iter; ++i){
    if(rank == 0){
      if(nm_sr_irecv_iov(p_session, gate_id, 42, 
			 riov, count, 
			 &rreq) != NM_ESUCCESS)
	errx(1, "NM Isend failed \n");
      nm_sr_rwait(p_session, &rreq);
      if(nm_sr_isend_iov(p_session, gate_id, 42, 
			 siov, count, 
			 &sreq) != NM_ESUCCESS)
	errx(1, "NM Irecv failed \n");
      nm_sr_swait(p_session, &sreq);
    }else{
      if(nm_sr_isend_iov(p_session, gate_id, 42, 
			 siov, count, 
			 &sreq) != NM_ESUCCESS)
	errx(1, "NM Irecv failed \n");
      nm_sr_swait(p_session, &sreq);
      if(nm_sr_irecv_iov(p_session, gate_id, 42, 
			 riov, count, 
			 &rreq) != NM_ESUCCESS)
	errx(1, "NM Isend failed \n");
      nm_sr_rwait(p_session, &rreq);
    }
  }
}


int main(int argc, char ** argv)
{
  int i;
  struct timeval tv1, tv2;
  
  nm_launcher_init(&argc, argv);
  nm_launcher_get_session(&p_session);
  nm_launcher_get_rank(&rank);
  peer = 1 - rank;
  nm_launcher_get_gate(peer, &gate_id);
    
  size = SIZE;
  iter = ITER;
  
  rbuf = calloc(1, size);
  sbuf = calloc(1, size);
  
  count = 1;
  siov = malloc(count * sizeof(struct iovec));
  riov = malloc(count * sizeof(struct iovec));
  
  siov->iov_base = sbuf;
  siov->iov_len = size;
  
  riov->iov_base = rbuf;
  riov->iov_len = size;
  
  /*barrier*/
  pingpong(1);
     
  gettimeofday(&tv1, NULL);
  pingpong(iter);
  gettimeofday(&tv2, NULL);
  
  if(rank == 0)
    printf("Contiguous: %lf MB/s \n", 2 * iter * ((double)size / (1024.0*1024)) / 
	   (tv2.tv_sec - tv1.tv_sec + (tv2.tv_usec - tv1.tv_usec)/1000000.0));
  
  free(siov);
  free(riov);
  
  count = size / PAGESIZE;
  siov = malloc(count * sizeof(struct iovec));
  riov = malloc(count * sizeof(struct iovec));
  
  for(i=0; i < count; ++i){
    siov[i].iov_base = sbuf + (i/2)*PAGESIZE + ((size/2) * (i%2));
    siov[i].iov_len = PAGESIZE;
     
    riov[i].iov_base = rbuf + (i/2)*PAGESIZE + ((size/2) * (i%2));
    riov[i].iov_len = PAGESIZE;
  }
  
  /*barrier*/
  pingpong(1);
  
  gettimeofday(&tv1, NULL);
  pingpong(iter);
  gettimeofday(&tv2, NULL);
  
  
  if(rank == 0)
    printf("Non-contiguous: %lf MB/s \n", 2 * iter * (size / (1024.0*1024)) / 
	   (tv2.tv_sec - tv1.tv_sec + (tv2.tv_usec - tv1.tv_usec) / 1000000.0));

  return 0;
}
