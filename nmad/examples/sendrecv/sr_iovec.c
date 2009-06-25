#include <err.h>
#include <stdio.h>
#include <nm_public.h>
#include <nm_launcher.h>
#include <nm_sendrecv_interface.h>

static int rank, peer;
static nm_core_t p_core = NULL;
static nm_gate_t gate_id = NULL;


#define IOV_COUNT 10

int main(int argc, char ** argv)
{
	int i;
	int sbuf[IOV_COUNT];
	int rbuf[IOV_COUNT];
	struct iovec riov[IOV_COUNT], siov[IOV_COUNT];
	nm_sr_request_t sreq, rreq;


	nm_launcher_init(&argc, argv);
	nm_launcher_get_core(&p_core);
	nm_launcher_get_rank(&rank);
	peer = 1 - rank;
	nm_launcher_get_gate(peer, &gate_id);
	
	
	for(i=0; i < IOV_COUNT; ++i){
		sbuf[i] = IOV_COUNT - i - 1;
		siov[i].iov_base = &sbuf[i];
		siov[i].iov_len = sizeof(int);

		rbuf[i] = -1;
		riov[i].iov_base = &rbuf[IOV_COUNT - i - 1];
		riov[i].iov_len = sizeof(int);
	}

	if(rank == 0){
	        sleep(1);
		if(nm_sr_isend_iov(p_core, gate_id, 42, 
				   siov, IOV_COUNT, 
				   &sreq) != NM_ESUCCESS)
			errx(1, "NM Isend failed \n");
		while(nm_sr_stest(p_core, &sreq) != NM_ESUCCESS);
		if(nm_sr_irecv_iov(p_core, gate_id, 42, 
				   riov, IOV_COUNT, 
				   &rreq) != NM_ESUCCESS)
			errx(1, "NM Irecv failed \n");
		

	}else{
		if(nm_sr_isend_iov(p_core, gate_id, 42, 
				   siov, IOV_COUNT, 
				   &sreq) != NM_ESUCCESS)
			errx(1, "NM Isend failed \n");
		if(nm_sr_irecv_iov(p_core, gate_id, 42, 
				   riov, IOV_COUNT, 
				   &rreq) != NM_ESUCCESS)
			errx(1, "NM Irecv failed \n");
		while(nm_sr_stest(p_core, &sreq) != NM_ESUCCESS);
	}

	nm_sr_rwait(p_core, &rreq);
	printf("rank = %d\n", rank);
	for(i=0; i < IOV_COUNT; ++i){
	  printf("[%i] %d\n", i, rbuf[i]);
		if(rbuf[i] != i)
			printf("BUG !!!! index %d value %d\n", i, rbuf[i]);
	}

	return 0;
}
