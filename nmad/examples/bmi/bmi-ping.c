
/* -*- Mode: C; c-basic-offset:2 ; -*- */
/*
 * NewMadeleine
 * Copyright (C) 2009 (see AUTHORS file)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

/* BMI is only available for 'huge tags' (ie. at least 64 bits) */
#ifdef NM_TAGS_AS_INDIRECT_HASH

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include <bmi.h>
#include <bmi-types.h>

#include "tbx.h"

//#define DEBUG 1

#define SERVER          1
#define CLIENT          2
#define MIN_BYTES       1
#define MAX_BYTES      (4<<20)
#define MAX_LEN        (8*1024*1024)
#define RECV            0
#define SEND            1
#define EXPECTED        0
#define UNEXPECTED      1
#define MAX_CONNEXION   4
#define NB_SERV         4

struct host{
    char *hostid;           /* host identifier */
    int  which;
    int  test;
};

struct hosts{
    struct host* host;
    int nb_hosts; 
};

static struct hosts* parse_args(int argc, char *argv[]);
static int check_uri(char *uri);

static void 
print_usage(void){
    fprintf(stderr, "usage: bmi-ping -h HOST_URI -s|-c [-u]\n");
    fprintf(stderr, "       where:\n");
    fprintf(stderr, "       HOST_URI is nmad://host:port\n");
    fprintf(stderr, "       -s is server and -c is client\n");
    fprintf(stderr, "       -u will use unexpected messages (pass to client only)\n");
    return;

}

static void 
fill_buffer(char *buffer, int len) {
    int i = 0;
    for (i = 0; i < len; i++) {
	buffer[i] = 'a'+(i%26);
    }
}

static void
print_buffer(char *buffer, int len) {
    int i;
    printf("buffer %p :\t", buffer);
    for (i=0; i<len; i++)
	printf("%c", buffer[i]);
    printf("\n");
}

static void 
clear_buffer(char *buffer, int len) {
    memset(buffer, 0, len);
}


int
main(int  argc,
     char **argv) {
    char	          *send_buf   = NULL;
    char	          *recv_buf   = NULL;
    uint64_t         len        = 0;
    struct hosts    *hosts      = NULL;
    int              ret        = -1;
    BMI_addr_t      *peer_addr  = NULL;
    BMI_addr_t       peer       = NULL;
    bmi_op_id_t      op_id      = NULL;
    bmi_error_code_t error_code = 0;
    int              outcount   = 0;
    int              end_len    = MAX_LEN;
    int              iter       = 1000;
    int              warmup     = 200;
    int              loop       = 0;

#ifndef TAG_MATCH
    bmi_msg_tag_t    tag        = 12;
#else
    bmi_msg_tag_t    tag        = 5624;
#endif


#ifdef TAG_MATCH
    printf("TAG_MATCHing enabled\n");
#endif


    assert(NB_SERV <= MAX_CONNEXION);

    hosts = parse_args(argc, argv);
    if (!hosts) {
	print_usage();
	exit(1);
    } 

    /*
     * initialize local interface (default options)  
     */ 
    if(hosts->host->which == SERVER) {
	ret = BMI_initialize(NULL,
			     NULL,
			     BMI_INIT_SERVER);
    } else {  
	ret = BMI_initialize(NULL, NULL, 0);
    }
    if (ret < 0) {
	fprintf(stderr,"BMI_initialize() ret = %d\n",ret);
	exit(1);
    }
  
    if(hosts->host->which == SERVER){
	/*
	 *  SERVEUR  just one connexion at time
	 */
	tbx_tick_t t1, t2;
	double sum, lat, bw_million_byte, bw_mbyte;

	bmi_context_id   context    = NULL;

	context = TBX_MALLOC(sizeof(struct bmi_context));
    
	ret = BMI_open_context(&context);
	if (ret < 0) {
	    fprintf(stderr,"BMI_open_context() ret = %d\n", ret);
	    exit(1);
	}
	struct BMI_unexpected_info request_info;
	/* wait for a client to connect */
	do {
	    ret = BMI_testunexpected(1, &outcount, &request_info, 10);
	} while (ret == 0 && outcount == 0);
    
	{
	    if (ret < 0) {
		fprintf(stderr, "Request recv failure (bad state).\n");
		perror("BMI_testunexpected");
		return ret;
	    }
	    if (request_info.error_code != 0) {
		fprintf(stderr, "Request recv failure (bad state).\n");
                return ret;
	    }
	}

	/* retrieve the client address */
	peer = request_info.addr;
	BMI_unexpected_free(peer, request_info.buffer);

	send_buf = BMI_memalloc(peer, end_len, BMI_SEND);
	recv_buf = BMI_memalloc(peer, end_len, BMI_RECV);

	/* create a buffer to recv into */
	fill_buffer(send_buf, end_len);
	clear_buffer(recv_buf, end_len);
    
	printf(" size\t\t|  latency\t|   10^6 B/s\t|   MB/s\t|\n");	    

	for(len=1; len<end_len; len*=2) {

	    for( loop=0; loop<warmup; loop++) {
#ifdef DEBUG
		fprintf(stderr, "\n[%d]\tRecv %d\n", len, loop);
#endif
		ret = BMI_post_recv(&op_id, peer, recv_buf, len, NULL, BMI_PRE_ALLOC,
				    tag, NULL, context, NULL);
		if (ret !=  0) {
		    fprintf(stderr," BMI_post_recv() ret = %d\n",ret); exit(1);
		}
		do {
		    ret = BMI_test(op_id,&outcount,NULL,NULL,NULL,0, context);
		} while (ret ==  0 && outcount == 0);
	
#ifdef DEBUG
		fprintf(stderr, "\n[%d]\tSend %d\n", len, loop);
#endif

		ret = BMI_post_send(&op_id, peer, send_buf, len, BMI_PRE_ALLOC, tag, 
				    NULL, context, NULL);
		if (ret !=  0) {
		    fprintf(stderr," BMI_post_send() ret = %d\n",ret); exit(1);
		}	  
		do {                  
		    ret = BMI_test(op_id, &outcount, NULL, NULL, NULL, 0, context);
		} while (ret == 0 && outcount == 0);    	
	    }

	    TBX_GET_TICK(t1);

	    for( loop=0; loop<iter; loop++) {	

#ifdef DEBUG
		fprintf(stderr, "\n[%d]\tRecv %d\n", len, loop);
#endif

		ret = BMI_post_recv(&op_id, peer, recv_buf, len, NULL, BMI_PRE_ALLOC,
				    tag, NULL, context, NULL);
		if (ret !=  0) {
		    fprintf(stderr," BMI_post_recv() ret = %d\n",ret); exit(1);
		}
		do {
		    ret = BMI_test(op_id, &outcount, NULL, NULL, NULL, 0, context);
		} while (ret ==  0 && outcount == 0);


#ifdef DEBUG
		fprintf(stderr, "\n[%d]\tSend %d\n", len, loop);
#endif

		ret = BMI_post_send(&op_id, peer, send_buf, len, BMI_PRE_ALLOC, 
				    tag, NULL, context, NULL);
		if (ret !=  0) {
		    fprintf(stderr," BMI_post_send() ret = %d\n",ret); exit(1);
		}	  
		do {                  
		    ret = BMI_test(op_id, &outcount, NULL,NULL, NULL, 0, context);
		} while (ret == 0 && outcount == 0);    	
	    }

	    TBX_GET_TICK(t2);

	    sum = TBX_TIMING_DELAY(t1, t2);

	    lat	      = sum / (2 * iter);
	    bw_million_byte = len*(iter / (sum / 2));
	    bw_mbyte        = bw_million_byte / 1.048576;


	    printf("%d\t\t%lf\t%8.3f\t%8.3f\n", 
		   len, lat, bw_million_byte, bw_mbyte);
	    fflush(stdout);
	}

	BMI_memfree(peer, send_buf, end_len, BMI_SEND);
	BMI_memfree(peer, recv_buf, end_len, BMI_RECV);

	BMI_close_context(context);

    } else {
	/*
	 *  CLIENT
	 */
	tbx_tick_t t1, t2;
	double sum, lat, bw_million_byte, bw_mbyte;
	int               i       = 0; 
	struct host*      host    = NULL;
	bmi_context_id*   context = NULL;
	int               nb_h    = hosts->nb_hosts;

	context = TBX_MALLOC(MAX_CONNEXION*sizeof(bmi_context_id*));

	for(i = 0; i < nb_h; i++){
	    ret = BMI_open_context(&context[i]);
	    if (ret < 0) {
		fprintf(stderr,"BMI_open_context() ret = %d\n", ret);
		exit(1);
	    }
	}
    
	peer_addr = TBX_MALLOC(MAX_CONNEXION*sizeof(BMI_addr_t*));

	for(i=0 ;i < hosts->nb_hosts; i++){
	    host = (hosts->host+i);
      
	    /* get a bmi_addr for the server */
	    BMI_addr_lookup(&peer, host->hostid);
	    peer_addr[i] = peer;
	}
	assert(peer);
	assert(peer_addr);

	send_buf = BMI_memalloc(peer_addr[0], end_len, BMI_SEND);
	recv_buf = BMI_memalloc(peer_addr[0], end_len, BMI_RECV);

	/* create a buffer to recv into */
	fill_buffer(send_buf, end_len);
	clear_buffer(recv_buf, end_len);

	/* connecting to the server */
	ret = BMI_post_sendunexpected(&op_id, peer_addr[0], send_buf,
				      8, BMI_PRE_ALLOC, 0, NULL, context[0], NULL);
	{
	    if (ret < 0) {
		fprintf(stderr, "BMI_post_sendunexpected failure.\n");
		return (-1);
	    } else if (ret == 0) {
		bmi_size_t actual_size=0;
		bmi_size_t msg_len=8;

		do {
		    ret = BMI_test(op_id, &outcount, &error_code,
				   &actual_size, NULL, 10, *context);
		} while (ret == 0 && outcount == 0);
	
		if (ret < 0 || error_code != 0) {
		    fprintf(stderr, "data send failed.\n");
		    return (-1);
		}
		if (actual_size != msg_len) {
		    fprintf(stderr, "Expected %d but received %d\n",
			    msg_len, actual_size);
		    return (-1);
		}      
	    }
	}
    
	for(len=1; len<end_len; len*=2) {
	    int i; 
      
	    for(loop=0; loop<warmup; loop++) {
	
		for(i = 0; i < nb_h; i++){

#ifdef DEBUG
		    fprintf(stderr, "\n[%d]\tSend %d\n", len, loop);
#endif
		    ret = BMI_post_send(&op_id, peer_addr[i], send_buf, len, 
					BMI_PRE_ALLOC, tag, 
					NULL, context[i], NULL);
		    if (ret !=  0) {
			fprintf(stderr," BMI_post_send() ret = %d\n",ret);
			exit(1);
		    }	    
		}

		for(i = 0; i < nb_h; i++){
		    do {                  
			ret = BMI_test(op_id, &outcount,NULL,NULL,NULL,0,context[i]);
		    } while (ret == 0 && outcount == 0); 
		}

		for(i = 0; i < nb_h; i++){

#ifdef DEBUG
		    fprintf(stderr, "\n[%d]\tRecv %d\n", len, loop);
#endif
		    ret = BMI_post_recv(&op_id, peer_addr[i], recv_buf, len, 
					&len, BMI_PRE_ALLOC, tag, 
					NULL, context[i], NULL);
		    if (ret !=  0) {
			fprintf(stderr," BMI_post_recv() ret = %d\n",ret);
			exit(1);
		    }
		}

		for(i = 0; i < nb_h; i++){
		    do {
			ret = BMI_test(op_id, &outcount, NULL,NULL,NULL,0,context[i]);
		    } while (ret ==  0 && outcount == 0);
		}
#ifdef DEBUG
		print_buffer(recv_buf, len);
#endif
	    }
     
	    for(loop=0; loop<iter; loop++) {
	
		for(i = 0; i < nb_h; i++){

#ifdef DEBUG
		    fprintf(stderr, "[%d]\tSend %d\n", len, loop);
#endif

		    ret = BMI_post_send(&op_id, peer_addr[i], send_buf, len, 
					BMI_PRE_ALLOC, tag, 
					NULL, context[i], NULL);
		    if (ret !=  0) {
			fprintf(stderr," BMI_post_send() ret = %d\n",ret);
			exit(1);
		    }
		}

		for(i = 0; i < nb_h; i++){

		    do {                  
			ret = BMI_test(op_id, &outcount, NULL,NULL, NULL, 10, context[i]);
		    } while (ret == 0 && outcount == 0);
		}

		for(i = 0; i < nb_h; i++){

#ifdef DEBUG
		    fprintf(stderr, "[%d]\tRecv %d\n", len, loop);
#endif

		    ret = BMI_post_recv(&op_id, peer_addr[i], recv_buf, len, 
					&len, BMI_PRE_ALLOC, tag, 
					NULL, context[i], NULL);
		    if (ret !=  0) {
			fprintf(stderr," BMI_post_recv() ret = %d\n",ret);
			exit(1);
		    }
		}	

		for(i = 0; i < nb_h; i++){
		    do {
			ret = BMI_test(op_id, &outcount, NULL,NULL, NULL, 0, context[i]);
		    } while (ret ==  0 && outcount == 0);
		}
	    }
	}

	BMI_memfree(peer_addr[0], send_buf, end_len, BMI_SEND);
	BMI_memfree(peer_addr[0], recv_buf, end_len, BMI_RECV);

	for(i = 0; i < nb_h; i++)
	    BMI_close_context(context[i]);
    }


    BMI_finalize();
    exit(0);
}

static int 
check_uri(char *uri){
    int ret = 0; /* failure */
    if (uri[0] == ':' && 
	uri[1] == '/' && 
	uri[2] == '/') ret = 1;
    return ret;
}

static struct hosts*
parse_args(int argc, 
	   char *argv[]){
    /* getopt stuff */
    extern char *optarg;
    char flags[] = "h:scu";
    int one_opt = 0;
    struct hosts* hosts = NULL;

    /* create storage for the command line options */
    hosts = TBX_MALLOC(sizeof(struct hosts));
    hosts->nb_hosts = 0;
    hosts->host = TBX_MALLOC(sizeof(struct host) * MAX_CONNEXION);

    if (!hosts) {
	goto parse_args_error;
    }

    /* look at command line arguments */
    while ((one_opt = getopt(argc, argv, flags)) != EOF) {
	int i = hosts->nb_hosts;
	switch (one_opt) {
	case ('h'):

	    (hosts->host+i)->hostid = (char *) strdup(optarg);
#ifdef DEBUG
	    fprintf(stdout,"host %d: %s\n",i,(hosts->host+i)->hostid);
#endif
	    hosts->nb_hosts++;
	    if (hosts->host->hostid == NULL)
		goto parse_args_error;
	    break;

	case ('s'):

	    if ((hosts->host+i)->which == CLIENT) {
		fprintf(stderr, "use -s OR -c, not both\n");
		goto parse_args_error;
	    }
	    hosts->host->which = SERVER;
	    break;

	case ('c'):

	    if ((hosts->host+i)->which == SERVER) {
		fprintf(stderr, "use -s OR -c, not both\n");
		goto parse_args_error;
	    }
	    hosts->host->which = CLIENT;
	    break;

#if 0
	case ('u'):
	    opts->test = UNEXPECTED;
	    break;
#endif
	default:
	    break;
	}
    }
    /* if we didn't get a host argument, bail: */
    if (hosts->host->hostid == NULL) {
	fprintf(stderr, "you must specify -h\n");
	goto parse_args_error;
    }
    if (hosts->host->which == 0) {
	fprintf(stderr, "you must specify -s OR -c\n");
	goto parse_args_error;
    }
    return hosts;

 parse_args_error:
    /* if an error occurs, just free everything and return NULL */
    if(hosts){
	if(hosts->host){
	    TBX_FREE(hosts->host);
	}
	TBX_FREE(hosts);
    }
    return NULL;
}

#else /* NM_TAGS_AS_INDIRECT_HASH */

#error BMI is only available for 'huge tags' (ie. at least 64 bits). Please enable the tag_huge option in the NMad module

#endif /* NM_TAGS_AS_INDIRECT_HASH */


/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 *
 * vim: ts=8 sts=4 sw=4 expandtab
 */
