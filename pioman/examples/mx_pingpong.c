/*************************************************************************
 * The contents of this file are subject to the MYRICOM MYRINET          *
 * EXPRESS (MX) NETWORKING SOFTWARE AND DOCUMENTATION LICENSE (the       *
 * "License"); User may not use this file except in compliance with the  *
 * License.  The full text of the License can found in LICENSE.TXT       *
 *                                                                       *
 * Software distributed under the License is distributed on an "AS IS"   *
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See  *
 * the License for the specific language governing rights and            *
 * limitations under the License.                                        *
 *                                                                       *
 * Copyright 2003 - 2004 by Myricom, Inc.  All rights reserved.          *
 *************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "/opt-x86_64/src/Networks/Myrinet/mx10g-1.2.0i/common/mx_auto_config.h"
#include "/opt-x86_64/src/Networks/Myrinet/mx10g-1.2.0i/common/myriexpress.h"
#if !MX_OS_WINNT
#include <unistd.h>
#include <sys/time.h>
#include <errno.h>
#else
#include "/opt-x86_64/src/Networks/Myrinet/mx10g-1.2.0i/tests/mx_uni.h"
#endif
#include "/opt-x86_64/src/Networks/Myrinet/mx10g-1.2.0i/common/mx_byteswap.h"
#include "/opt-x86_64/src/Networks/Myrinet/mx10g-1.2.0i/tests/test_common.h"

#include "pm2_common.h"

#define FILTER     0x12345
#define MATCH_VAL  0xabcdef
#define DFLT_EID   1
#define DFLT_INC   1
#define DFLT_START 1
#define DFLT_END   1280
#define MAX_LEN    (1024*1024*1024) 
#define DFLT_ITER  1000
#define DFLT_MULT  1.0
#define DFLT_WARMUP 10

struct datapoint
{
  int len;
  struct datapoint *next;
};
#define PIOM

/*********************************************/
typedef struct piom_io_server {
	struct piom_server server;
} *piom_io_serverid_t;

typedef struct piom_mx_ev {
	struct piom_req inst;
	mx_endpoint_t ep;
        mx_request_t *request;
        mx_status_t *status;
        uint32_t *result;
} *piom_mx_ev_t;

static struct piom_io_server piom_io_server = {
	.server =
	PIOM_SERVER_INIT(piom_io_server.server, "MX EVServer"),

};
mx_endpoint_addr_t prout;
static mx_status_t status_global;
static char remote_host[100];
static int nb_polled=0;
/* PIOM_FUNCTYPE_POLL_POLLONE */
static int piom_test_pollone(piom_server_t server,
			  piom_op_t _op,
			  piom_req_t req, int nb_ev, int option)
{
	
	piom_mx_ev_t ev = struct_up(req, struct piom_mx_ev, inst);
//	fprintf(stderr, "polling   %p, %p, %p, %p\n", ev->ep, ev->request, ev->status, ev->result);
	memcpy(&ev->status->source, &status_global.source, sizeof(status_global.source));
	//fprintf(stderr, "polling %p\n",status_global.source);
	if(mx_test(ev->ep, ev->request, ev->status, ev->result) == MX_SUCCESS){

//		fprintf(stderr, "ev->status\n");
		if(*ev->result){
			//fprintf(stderr, "polling success\n");
			nb_polled++;
				
			piom_req_success(req);

		}
	} /*
else 
		fprintf(stderr, "poll failed\n");
	  */

}

static int piom_null(piom_server_t server,
			  piom_op_t _op,
			  piom_req_t req, int nb_ev, int option)
{
}

void piom_test_init(void)
{
	LOG_IN(); 
	piom_io_server.server.stopable=0;
	piom_server_set_poll_settings(&piom_io_server.server,
				       PIOM_POLL_AT_TIMER_SIG
				       | PIOM_POLL_AT_IDLE, 1, -1);

	piom_server_add_callback(&piom_io_server.server,
				  PIOM_FUNCTYPE_POLL_POLLONE,
				  (piom_pcallback_t) {
				  .func = &piom_test_pollone,.speed =
				  PIOM_CALLBACK_SLOWEST});

/*
	piom_server_add_callback(&piom_io_server.server,
				  PIOM_FUNCTYPE_BLOCK_WAITONE,
				  (piom_pcallback_t) {
				  .func = &piom_null,.speed =
				  PIOM_CALLBACK_SLOWEST});
*/
	piom_server_start(&piom_io_server.server);
	fprintf(stderr, "Piomette server initialized\n");
	LOG_OUT();
}
 

void
usage()
{
  fprintf(stderr, "Usage: mx_pingpong [args]\n");
  fprintf(stderr, "-n nic_id - local NIC ID [MX_ANY_NIC]\n");
  fprintf(stderr, "-b board_id - local Board ID [MX_ANY_NIC]\n");
  fprintf(stderr, "-e local_eid - local endpoint ID [%d]\n", DFLT_EID);
  fprintf(stderr, "-s - runs as slave, wait for another connection after this test\n");  
  fprintf(stderr, "---- the following options are only used on the sender side -------\n");
  fprintf(stderr, "-d hostname - destination hostname, required for sender\n");
  fprintf(stderr, "-r remote_eid - remote endpoint ID [%d]\n", DFLT_EID);
  fprintf(stderr, "-f filter - remote filter [%x]\n", FILTER);
  fprintf(stderr, "-S start_len - starting length [%d]\n", DFLT_START);
  fprintf(stderr, "-E end_len - ending length [%d]\n", DFLT_END);
  fprintf(stderr, "-I incr - increment packet length [%d]\n", DFLT_INC);
  fprintf(stderr, "-M mult - length multiplier, overrides -I\n");
  fprintf(stderr, "-L filename - name of file containing lengths, overrides -I, -M\n");
  fprintf(stderr, "-N iterations - iterations per length [%d]\n", DFLT_ITER);
  fprintf(stderr, "-V - verify msg content [OFF]\n");
  fprintf(stderr, "-w - block rather than poll\n");  
  fprintf(stderr, "-W warmup - number of warmup iterations\n");  
  fprintf(stderr, "-h - help\n");
  fprintf(stderr, "\tIf -M specified, length progression is geometric, "
	  "else arithmetic\n"); 

}

int Verify;		/* If true, verify packet contents */

static inline void
pingpong(uint32_t sender,
	 mx_endpoint_t ep,
	 mx_endpoint_addr_t dest,
	 int iter,
	 uint32_t start_len,
	 uint32_t end_len,
	 int inc,
	 double mult,
	 struct datapoint *lengths,
	 int wait,
	 int warmup)
{
  mx_status_t stat;
  mx_request_t sreq, rreq[2];
  mx_segment_t seg_send, seg_recv;
  unsigned char *buff_send, *buff_recv;
  struct timeval start_time, end_time;
  unsigned int i, j, cur_len, usec, current_rreq;
  double lat, bw;
  uint64_t sseq, rseq;
  uint32_t result;
  int have_datapoints = (lengths != NULL);
  
  sseq = 0x1000;
  rseq = 0;

  /* allocate buffers for our data */
  buff_send = (unsigned char *) malloc(end_len);
  buff_recv = (unsigned char *) malloc(end_len);
  if ((buff_send == NULL) || (buff_recv == NULL)) {
    fprintf(stderr,"mx_pingpong:malloc(%d) failed!", end_len);
    exit(1);
  }

  if (sender) {
    /* print header */
    printf("Running %d iterations.\n", iter);
    printf("   Length   Latency(us)    Bandwidth(MB/s)\n");
  }

  if (have_datapoints) {
    start_len = lengths->len;
    lengths = lengths->next;
  }
  
  /* set up segment lists */
  seg_send.segment_ptr = buff_send;
  seg_recv.segment_ptr = buff_recv;
  if (!Verify)
    seg_recv.segment_ptr = buff_send;
  
  /* loop through each sample length */
  for (cur_len = start_len; cur_len < end_len; ) {

    /* set up segment lists */
    seg_send.segment_length = cur_len;
    seg_recv.segment_length = cur_len;
    
    if (sender) {
      /* just to avoid unexpected messages between runs (FIXME) */
      usleep(100000);

      /* post receive for sender */
      mx_irecv(ep, &seg_recv, 1, rseq,
	       MX_MATCH_MASK_NONE, 0, &(rreq[0]));
      rseq++;
      current_rreq = 1; 
    } else {
      /* post receives for receiver */
      for (current_rreq = 0; current_rreq < 2; current_rreq++) {
	mx_irecv(ep, &seg_recv, 1, sseq, 
		 MX_MATCH_MASK_NONE, 0, &(rreq[current_rreq]));
	sseq++;
      }
      current_rreq = 0;
    }

    /* if verifying, fill buffer with a value */
    if (sender && Verify) {
      for (j = 0; j < cur_len; j++) {
	buff_send[j] = rand();
	buff_recv[j] = rand();
      }
    }

    /* loop through the iterations */
    for (i=0; i<iter + warmup; i++) {
//	    fprintf(stderr, "Loop %d\n",i);
      /* get starting time */
      if (i == warmup)
	gettimeofday(&start_time, NULL);
      
      if (sender) {
//	      if(i>100)
//		      sleep(5);
	/* post send */
	mx_isend(ep, &seg_send, 1, dest, sseq, NULL, &sreq);
	sseq++;
	
	/* post receive */
	mx_irecv(ep, &seg_recv, 1, rseq, MX_MATCH_MASK_NONE, 0, 
		 &(rreq[current_rreq]));
	current_rreq = ((current_rreq + 1) % 2);
	rseq++;
      
	/* wait for the send to complete */
	if (wait)
	  mx_wait(ep, &sreq, MX_INFINITE, &stat, &result);
	else {
//		fprintf(stderr, "sending1\n");
#ifdef PIOM
		struct piom_wait xp_wait;
		struct piom_mx_ev xp_req;
		xp_req.ep=ep;
		xp_req.request=&sreq;
		xp_req.status=&stat;
		xp_req.result=&result;
		//fprintf(stderr, "piom_wait %p, %p, %p, %p\n", ep, &sreq, &stat, &result);
		piom_wait(&piom_io_server.server, &xp_req.inst, &xp_wait, NULL);
#else
	  do { 
	    mx_test(ep, &sreq, &stat, &result);
	  } while (!result);  
#endif
	}
	
	/* wait for the receive to complete */
	if (wait)
	  mx_wait(ep, &rreq[current_rreq], MX_INFINITE, &stat, &result);
	else {
		//fprintf(stderr, "recving\n");
#ifdef PIOM
		struct piom_wait xp_wait;
		struct piom_mx_ev xp_req;
		xp_req.ep=ep;
		xp_req.request= &rreq[current_rreq];
		xp_req.status=&stat;
		xp_req.result=&result;
		//fprintf(stderr, "wait_recv %p, %p, %p, %p\n", ep, &rreq[current_rreq], &stat, &result);
		piom_wait(&piom_io_server.server, &xp_req.inst, &xp_wait, NULL);
#else
	  do { 
	    mx_test(ep, &rreq[current_rreq], &stat, &result);
	  } while (!result);  
#endif
	}
	
	/* verify recv contents if needed */
	if (Verify) {
//	  exit(2);
	  if (stat.xfer_length != cur_len) {
	    fprintf(stderr, "Bad len from recv, %d should be %d\n",
		    stat.xfer_length, cur_len);
	    exit(1);
	  }
	  
	  if (memcmp(buff_send, buff_recv, cur_len)) {
	    unsigned int erroff = 0, errcnt = 0;
	    for (j = 0; j < cur_len; j++) {
	      if (buff_send[j] != buff_recv[j]) {
		if (errcnt == 0) erroff = j;
		errcnt++;
	      }
	    }
	    fprintf(stderr, "data corruption: offset %d, cnt %d (len %d)\n", 
		    erroff, errcnt, cur_len);
	    exit(2);
	  }
	}
	
      } else {
      
	/* wait for receive to complete */
	if (wait)
	  mx_wait(ep, &rreq[current_rreq], MX_INFINITE, &stat, &result);
	else {
		//fprintf(stderr, "recving\n");
#ifdef PIOM
		struct piom_wait xp_wait;
		struct piom_mx_ev xp_req;
		xp_req.ep=ep;
		xp_req.request= &rreq[current_rreq];
		xp_req.status=&stat;
		xp_req.result=&result;
//		xp_req.inst.func_to_use
		//fprintf(stderr, "piom_wait %p, %p, %p, %p\n", ep, &rreq[current_rreq], &stat, &result);
		piom_wait(&piom_io_server.server, &xp_req.inst, &xp_wait, NULL);
#else
	  do { 
	    mx_test(ep, &rreq[current_rreq], &stat, &result);
	  } while (!result);  
#endif
/*
		{
			uint64_t nic; uint32_t eid;
			mx_decompose_endpoint_addr(stat.source, &nic, &eid);
//			fprintf(stderr, "## %llx nic %llx endpoint %x \n", *(unsigned long long*)(&ev->status->source), nic, eid);
			fprintf(stderr, "@@ nic %llx endpoint %x \n", nic, eid);
//			fprintf(stderr, "## %p nic %llx endpoint %x \n", , nic, eid);

		}
*/
/*
		if(memcmp(&prout, &stat.source, sizeof(prout))){
			abort();

		}
*/
	}
	
	/* post send from recv buffer */
	mx_isend(ep, &seg_recv, 1, stat.source, rseq, NULL, &sreq);
	rseq++;

	/* post receive */
	mx_irecv(ep, &seg_recv, 1, sseq, MX_MATCH_MASK_NONE, 0, 
		 &(rreq[current_rreq]));
	current_rreq = ((current_rreq + 1) % 2);
	sseq++;

	/* wait for send to complete */
	if (wait)
	  mx_wait(ep, &sreq, MX_INFINITE, &stat, &result);
	else {
		//fprintf(stderr, "sending\n");
#ifdef PIOM
		struct piom_wait xp_wait;
		struct piom_mx_ev xp_req;
		xp_req.ep=ep;
		xp_req.request= &sreq;
		xp_req.status=&stat;
		xp_req.result=&result;
		//fprintf(stderr, "piom_wait %p, %p, %p, %p\n", ep, &sreq, &stat, &result);
		piom_wait(&piom_io_server.server, &xp_req.inst, &xp_wait, NULL);
#else
	  do { 
	    mx_test(ep, &sreq, &stat, &result);
	  } while (!result);  
#endif
	}
	
      }
    }
  
    /* get ending time */
    gettimeofday(&end_time, NULL);
  
    /* consume the pre-posted receives */
    if (sender) {
      /* post send */
      mx_isend(ep, &seg_send, 1, dest, sseq, NULL, &sreq);
      sseq++;
      
      /* wait for the send to complete */
      if (wait)
	mx_wait(ep, &sreq, MX_INFINITE, &stat, &result);
      else {
	      //fprintf(stderr, "sending\n");
#ifdef PIOM
		struct piom_wait xp_wait;
		struct piom_mx_ev xp_req;
		xp_req.ep=ep;
		xp_req.request= &sreq;
		xp_req.status=&stat;
		xp_req.result=&result;
		//fprintf(stderr, "piom_wait %p, %p, %p, %p\n", ep, &sreq, &stat, &result);
		piom_wait(&piom_io_server.server, &xp_req.inst, &xp_wait, NULL);
#else
	do { 
	  mx_test(ep, &sreq, &stat, &result);
	} while (!result);  
#endif
      }
	
      /* just to avoid early packets (FIXME) */
      usleep(200000);

      /* post send */
      mx_isend(ep, &seg_send, 1, dest, sseq, NULL, &sreq);
      sseq++;
      
      /* wait for the send to complete */
      if (wait)
	mx_wait(ep, &sreq, MX_INFINITE, &stat, &result);
      else {
	      //fprintf(stderr, "sending\n");
#ifdef PIOM
		struct piom_wait xp_wait;
		struct piom_mx_ev xp_req;
		xp_req.ep=ep;
		xp_req.request= &sreq;
		xp_req.status=&stat;
		xp_req.result=&result;
		//fprintf(stderr, "piom_wait %p, %p, %p, %p\n", ep, &sreq, &stat, &result);
		piom_wait(&piom_io_server.server, &xp_req.inst, &xp_wait, NULL);
#else
	do { 
	  mx_test(ep, &sreq, &stat, &result);
	} while (!result);  
#endif
      }
	
      /* wait for the receive to complete */
      current_rreq = ((current_rreq + 1) % 2);
      if (wait)
	mx_wait(ep, &rreq[current_rreq], MX_INFINITE, &stat, &result);
      else {
	      //fprintf(stderr, "recving\n");
#ifdef PIOM
		struct piom_wait xp_wait;
		struct piom_mx_ev xp_req;
		xp_req.ep=ep;
		xp_req.request=  &rreq[current_rreq];
		xp_req.status=&stat;
		xp_req.result=&result;
		//fprintf(stderr, "piom_wait %p, %p, %p, %p\n",ep, &rreq[current_rreq], &stat, &result);
		piom_wait(&piom_io_server.server, &xp_req.inst, &xp_wait, NULL);
#else
	do { 
	  mx_test(ep, &rreq[current_rreq], &stat, &result);
	} while (!result);  
#endif
      }
   
    } else {
      
      /* wait for receive to complete */
      if (wait)
	mx_wait(ep, &rreq[current_rreq], MX_INFINITE, &stat, &result);
      else {
	      //fprintf(stderr, "recving\n");
#ifdef PIOM
		struct piom_wait xp_wait;
		struct piom_mx_ev xp_req;
		xp_req.ep=ep;
		xp_req.request=  &rreq[current_rreq];
		xp_req.status=&stat;
		xp_req.result=&result;
		//fprintf(stderr, "piom_wait %p, %p, %p, %p\n",ep, &rreq[current_rreq], &stat, &result);
		piom_wait(&piom_io_server.server, &xp_req.inst, &xp_wait, NULL);
#else
	do { 
	  mx_test(ep, &rreq[current_rreq], &stat, &result);
	} while (!result);  
#endif
      }
      current_rreq = ((current_rreq + 1) % 2);
      
      /* wait for receive to complete */
      if (wait)
	mx_wait(ep, &rreq[current_rreq], MX_INFINITE, &stat, &result);
      else {
	      //fprintf(stderr, "recving\n");
#ifdef PIOM
		struct piom_wait xp_wait;
		struct piom_mx_ev xp_req;
		xp_req.ep=ep;
		xp_req.request=  &rreq[current_rreq];
		xp_req.status=&stat;
		xp_req.result=&result;
		//fprintf(stderr, "piom_wait %p, %p, %p, %p\n",ep, &rreq[current_rreq], &stat, &result);
		piom_wait(&piom_io_server.server, &xp_req.inst, &xp_wait, NULL);
#else
	do { 
	  mx_test(ep, &rreq[current_rreq], &stat, &result);
	} while (!result);  
#endif
      }
      /* post send from recv buffer */
      mx_isend(ep, &seg_recv, 1, stat.source, rseq, NULL, &sreq);
      rseq++;
      
      /* wait for send to complete */
      if (wait)
	mx_wait(ep, &sreq, MX_INFINITE, &stat, &result);
      else {
	      //fprintf(stderr, "sending\n");
#ifdef PIOM
		struct piom_wait xp_wait;
		struct piom_mx_ev xp_req;
		xp_req.ep=ep;
		xp_req.request=&sreq;
		xp_req.status=&stat;
		xp_req.result=&result;
		//fprintf(stderr, "piom_wait %p, %p, %p, %p\n",ep, &sreq, &stat, &result);
		piom_wait(&piom_io_server.server, &xp_req.inst, &xp_wait, NULL);
#else
	do { 
	  mx_test(ep, &sreq, &stat, &result);
	} while (!result);  
#endif
      }
    }
      
    usec = end_time.tv_usec - start_time.tv_usec;
    usec += (end_time.tv_sec - start_time.tv_sec) * 1000000;
    lat = (double) usec / iter / 2;
    bw =  (2.0 * iter * cur_len) / (double) usec;

    if (sender) {
      fprintf(stderr, "%9d   %9.3f       %8.3f \t%d polled\n", cur_len, lat, bw, nb_polled);
      nb_polled=0;
    }

    /* update current length */
    if (have_datapoints) {
      if (lengths != NULL) {
	cur_len = lengths->len;
	lengths = lengths->next;
      }
      else {
	break;
      }
    }
    else if (inc > 0) {
      cur_len += inc;
    } else {
      int new_len;

      new_len = cur_len * mult;
      if (new_len > cur_len) {
	cur_len = new_len;
      } else {
	++cur_len;
      }
    }
  }
  free(buff_send);
  free(buff_recv);
}

static void
pingpong_blocking(uint32_t sender,
		  mx_endpoint_t ep,
		  mx_endpoint_addr_t dest,
		  int iter,
		  uint32_t start_len,
		  uint32_t end_len,
		  int inc,
		  double mult,
		  struct datapoint *lengths,
		  int warmup)
{
  pingpong(sender, ep, dest, iter, start_len, end_len, inc,
	   mult, lengths, 1, warmup);
}

static void
pingpong_polling(uint32_t sender,
		 mx_endpoint_t ep,
		 mx_endpoint_addr_t dest,
		 int iter,
		 uint32_t start_len,
		 uint32_t end_len,
		 int inc,
		 double mult,
		 struct datapoint *lengths,
		 int warmup)
{
  pingpong(sender, ep, dest, iter, start_len, end_len, inc,
	   mult, lengths, 0, warmup);
}

#if MX_OS_FREEBSD
/* hack around bug in pthreads that gets us into an unkillable state */
#include <signal.h>

void
terminate(int sig)
{

  fprintf(stderr, "Exiting due to signal %d\n", sig);
  _exit(sig == 0 ? 0 : 1);
}
#endif


struct app_param {
  uint32_t start_len;
  uint32_t end_len;
  uint32_t inc;
  uint32_t warmup;
  uint32_t verify;
  uint32_t iter;
  uint32_t do_wait;
  uint32_t eid;
  uint32_t nic_low32;
  uint16_t nic_high16;
  char mult[64];
};

int main(int argc, char **argv)
{
  mx_endpoint_t ep;
  uint64_t nic_id;
  uint32_t my_eid;
  uint64_t his_nic_id;
  uint32_t board_id;
  uint32_t filter;
  uint16_t his_eid;
  mx_endpoint_addr_t his_addr;
  char *rem_host;
  int inc;
  double mult;
  char *len_filename;
  FILE *len_fp;
  char scratch[80];
  struct datapoint anchor;
  struct datapoint *data1, *data2;
  int start_len;
  int end_len;
  int iter;
  int c;
  int do_wait;
  int slave;
  int warmup;
  int opt_n;
  extern char *optarg;
  
  common_init(&argc, argv, NULL);
  piom_test_init();
#if DEBUG
  extern int mx_debug_mask;
  mx_debug_mask = 0xFFF;
#endif

  setvbuf(stdout, 0, _IOLBF, BUFSIZ);
  /* set up defaults */
  rem_host = NULL;
  filter = FILTER;
  inc = 0;
  my_eid = MX_ANY_ENDPOINT;
  his_eid = DFLT_EID;
  board_id = MX_ANY_NIC;
  inc = DFLT_INC;
  end_len = DFLT_END;
  start_len = DFLT_START;
  iter = DFLT_ITER;
  mult = DFLT_MULT;
  len_filename = NULL;
  warmup = DFLT_WARMUP;
  do_wait = 0;
  slave = 0;
  opt_n = 0;

#if MX_OS_FREEBSD
  (void)signal(SIGINT, terminate);
#endif

  while ((c = getopt(argc, argv, "hsd:e:n:b:r:f:S:E:I:M:L:N:VwW:")) != EOF) switch(c) {
  case 'd':
    rem_host = optarg;
    break;
  case 'e':
    my_eid = atoi(optarg);
    break;
  case 'f':
    filter = atoi(optarg);
    break;
  case 'n':
    sscanf(optarg, "%"SCNx64, &nic_id);
    opt_n = 1;
    break;
  case 'b':
    board_id = atoi(optarg);
    break;
  case 'r':
    his_eid = atoi(optarg);
    break;
  case 'S':
    start_len = atoi(optarg);
    break;
  case 'E':
    end_len = atoi(optarg);
    if (end_len > MAX_LEN) {
      fprintf(stderr, "end_len too large, max is %d\n", MAX_LEN);
      exit(1);
    }
    break;
  case 'M':
    mult = atof(optarg);
    inc = 0;
    break;
  case 'L':
    len_filename = optarg;
    break;
  case 'I':
    inc = atoi(optarg);
    break;
  case 'N':
    iter = atoi(optarg);
    break;
  case 'V':
    Verify = 1;
    break;
  case 'w':
    do_wait = 1;
    break;
  case 'W':
    warmup = atoi(optarg);
    break;
  case 's':
    slave = 1;
    break;
  case 'h':
  default:
    usage();
    exit(1);
  }
  mx_init();
  if (opt_n)
    mx_nic_id_to_board_number(nic_id, &board_id);

  anchor.next = NULL;
  data1 = &anchor;
  if (len_filename != NULL) {
    len_fp = fopen(len_filename, "r");
    if (len_fp == NULL) {
      fprintf(stderr, "opening %s:%s\n", len_filename, strerror(errno));
      exit(1);
    }
    while (fgets(scratch, 80, len_fp) != NULL) {
      if (strlen(scratch) > 1) {
	data2 = malloc(sizeof (*data2));
	if (data2 == NULL) {
	  fprintf(stderr,"mx_pingpong:malloc(%d) failed\n", (int)sizeof(*data2));
	  exit(1);
	}
	data2->len = atoi(scratch);
	if (end_len < data2->len) {
	  end_len = data2->len;
	}
	data2->next = NULL;
	data1->next = data2;
	data1 = data2;
      }
    }
    ++end_len;
    fclose(len_fp);
    data1 = &anchor;
  }
	
  if (my_eid == MX_ANY_ENDPOINT && !rem_host)
    my_eid = DFLT_EID;
  mx_open_endpoint(board_id, my_eid, filter, NULL, 0, &ep);
  
  /* If no host, we are receiver */
  if (rem_host == NULL) {
    char hostname[MX_MAX_HOSTNAME_LEN];
    mx_endpoint_addr_t me;
    mx_get_endpoint_addr(ep, &me);
    mx_decompose_endpoint_addr(me, &nic_id, &my_eid);
    mx_nic_id_to_hostname(nic_id, hostname);
    printf("Starting pingpong receiver on %s, endpoint=%d\n", hostname, my_eid);
    do {
      mx_request_t req;
      mx_status_t status;
      struct app_param param;
      mx_segment_t seg;
      uint32_t result;
      uint64_t nic;

      seg.segment_ptr = &param;
      seg.segment_length = sizeof(param);
      mx_irecv(ep, &seg, 1, 
		    UINT64_C(0x0000222211110000),MX_MATCH_MASK_NONE,
		    0, &req);
      mx_wait(ep, &req, MX_INFINITE, &status, &result);

      inc = ntohl(param.inc);
      end_len = ntohl(param.end_len);
      start_len = ntohl(param.start_len);
      iter = ntohl(param.iter);
      mult = atof(param.mult);
      do_wait = ntohl(param.do_wait);
      Verify = ntohl(param.verify);
      nic = ((uint64_t)ntohs(param.nic_high16) << 32) |
	ntohl(param.nic_low32);
      warmup = ntohl(param.warmup);
      
      if (Verify) printf("Verifying results\n");
      
      mx_connect (ep, nic, ntohl(param.eid), filter, MX_INFINITE, &his_addr);
      memcpy(&status_global.source,&his_addr, sizeof(ep));
      
      fprintf(stderr, "connected to %p\n",status_global.source);
      fprintf(stderr, "%lld %d %d\n", nic, ntohl(param.eid), filter);
      if (do_wait)
	pingpong_blocking(0, ep, his_addr, iter, start_len, end_len, 
			       inc, mult, anchor.next, warmup);
      else
	pingpong_polling(0, ep, his_addr, iter, start_len, end_len, 
			      inc, mult, anchor.next, warmup);
    } while (slave);

  /* remote hostname implies we are sender */
  } else {
    mx_request_t req;
    mx_status_t status;
    struct app_param param;
    mx_segment_t seg;
    uint32_t result;
    mx_endpoint_addr_t me;
    uint32_t eid;
    uint64_t nic;

    param.inc = htonl(inc);
    param.end_len = htonl(end_len);
    param.start_len = htonl(start_len);
    param.iter = htonl(iter);
    snprintf(param.mult, sizeof(param.mult), "%f", (float)mult);
    param.do_wait = htonl(do_wait);
    param.verify = htonl(Verify);
    mx_get_endpoint_addr(ep, &me);
    mx_decompose_endpoint_addr(me, &nic, &eid);
    param.eid = htonl(eid);
    param.nic_low32 = htonl((uint32_t)nic);
    param.nic_high16 = htons((nic) >> 32);
    param.warmup = htonl(warmup);

    /* get address of destination */
    if (strncmp(rem_host,"0x", 2) == 0) {
      sscanf(rem_host, "%"SCNx64, &his_nic_id);
    } else {
      mx_hostname_to_nic_id(rem_host, &his_nic_id);
    }
    mx_connect(ep, his_nic_id, his_eid, filter, MX_INFINITE, &his_addr);
    memcpy(&status_global.source,&his_addr, sizeof(ep));
    fprintf(stderr, "connected to %p\n",status_global.source);
    seg.segment_ptr = &param;
    seg.segment_length = sizeof(param);
    mx_isend(ep, &seg, 1, his_addr, UINT64_C(0x0000222211110000), 0, &req);
    mx_wait(ep, &req,MX_INFINITE, &status, &result);

    printf("Starting pingpong send to host %s\n", rem_host);
    if (Verify) printf("Verifying results\n");

    /* start up the sender */
    if (do_wait)
      pingpong_blocking(1, ep, his_addr, iter, start_len, end_len, inc, mult, anchor.next, warmup);
    else
      pingpong_polling(1, ep, his_addr, iter, start_len, end_len, inc, mult, anchor.next, warmup);
  }

  
  mx_close_endpoint(ep);
  mx_finalize();
  while (anchor.next != NULL) {
    data1 = anchor.next;
    anchor.next = anchor.next->next;
    free(data1);
  }
  return 0;
}
