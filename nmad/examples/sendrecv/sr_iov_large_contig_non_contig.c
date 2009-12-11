#include <err.h>
#include <stdio.h>
#include <nm_public.h>
#include <nm_launcher.h>
#include <nm_sendrecv_interface.h>
#include <sys/time.h>

static int rank, peer;
static nm_session_t p_session = NULL;
static nm_gate_t gate_id = NULL;

#define SIZE (1024*1024)
#define NB_IOV 8

void fill_buffer(char* buffer, int len, int offset)
{
  int i;
  for(i = 0; i < len; i++)
    {
      buffer[i] = 'a' + ((i + offset) % 26);
    }
}

int check_buffer(char*buffer, int len, int offset)
{
  int ret = 0;
  int i;
  int failures = 0;
  for(i = 0; i < len; i++) 
    {
      if(buffer[i] != 'a' + ((i + offset) % 26))
	{
	  failures++;
	  printf("Corrupted data at byte #%d (offset %d): char is %c (%d), expected: %c (%d)\n",
		 i, offset, buffer[i], buffer[i], 'a' + ((i + offset) % 26), 'a' + ((i + offset) % 26));
	  ret = 1;
	}
      if(failures > 100)
	abort();
    }
  return ret;
}

int main(int argc, char ** argv)
{
  int i;
  nm_sr_request_t sreq, rreq;
  
  nm_launcher_init(&argc, argv);
  nm_launcher_get_session(&p_session);
  nm_launcher_get_rank(&rank);
  peer = 1 - rank;
  nm_launcher_get_gate(peer, &gate_id);
    
  if(rank == 0)
    {
      /* server: send/recv non_contiguous messages */
      struct iovec* iov = malloc(NB_IOV * sizeof(struct iovec));
      
      void** buf = malloc(NB_IOV * sizeof(void*));
      for(i = 0; i < NB_IOV; ++i)
	{
	  buf[i] = malloc(SIZE);
	  memset(buf[i], 7, SIZE);
	  
	  iov[i].iov_base = buf[i];
	  iov[i].iov_len = SIZE;
	}
      
      fprintf(stderr, "### Irecv %d x %d bytes ###\n", NB_IOV, SIZE);

      if(nm_sr_irecv_iov(p_session, gate_id, 42, iov, NB_IOV, &rreq) != NM_ESUCCESS)
	{
	  fprintf(stderr, "NM Irecv failed \n");
	  abort();
	}
      nm_sr_rwait(p_session, &rreq);
      
      int cumulated_len = 0;
      for(i = 0; i < NB_IOV; i++)
	{
	  if(check_buffer(iov[i].iov_base, iov[i].iov_len, cumulated_len))
	    fprintf(stderr, "IOV #%d is corrupted\n", i);
	  cumulated_len += iov[i].iov_len;
	}
      fprintf(stderr, "### data received, check ok.\n");

      fprintf(stderr, "### Isend %d x %d bytes ###\n", NB_IOV, SIZE);

      if(nm_sr_isend_iov(p_session, gate_id, 42, iov, NB_IOV, &sreq) != NM_ESUCCESS)
	{
	  fprintf(stderr, "NM Isend failed \n");
	  abort();
	}
      nm_sr_swait(p_session, &sreq);
    }
  else
    {
      /* client: send/recv contiguous messages */
      void *sbuf = malloc(NB_IOV * SIZE);
      void *rbuf = malloc(NB_IOV * SIZE);
      fill_buffer(sbuf, NB_IOV * SIZE, 0);
      memset(rbuf, 3, NB_IOV * SIZE);
      fprintf(stderr, "### Isend %d bytes ###\n", NB_IOV * SIZE);
      if(nm_sr_isend(p_session, gate_id, 42, sbuf, NB_IOV * SIZE, &sreq) != NM_ESUCCESS)
	{
	  fprintf(stderr, "NM Isend failed \n");
	  abort();
	}
      nm_sr_swait(p_session, &sreq);
      
      fprintf(stderr, "### Irecv %d bytes ###\n", NB_IOV * SIZE);
      if(nm_sr_irecv(p_session, gate_id, 42, rbuf, NB_IOV * SIZE, &rreq) != NM_ESUCCESS) 
	{
	  fprintf(stderr, "NM Irecv failed \n");
	  abort();
	}
      nm_sr_rwait(p_session, &rreq);
      
      if(check_buffer(rbuf, NB_IOV * SIZE, 0))
	fprintf(stderr, "contiguous message is corrupted\n");
    }
  
  printf("### End of the test ###\n");
  return 0;
}

/*
 * Local variables:
 *  c-indent-level: 2
 *  c-basic-offset: 2
 * End:
 *
 * vim: ts=8 sts=2 sw=2 expandtab
 */
