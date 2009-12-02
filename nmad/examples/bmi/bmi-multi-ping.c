
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

#include <nm_public.h>

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

struct host{
    char *hostid;           /* host identifier */
    int  which;
    int clientid;
};

struct hosts{
    struct host* host;
    int nb_hosts; 
};

static struct hosts    *hosts      = NULL;
static bmi_context_id   context;
static char           **send_buf   = NULL;
static char           **recv_buf   = NULL;

static int              end_len    = MAX_LEN;
static int              iter       = 1000;
static int              warmup     = 200;
static int              loop       = 0;
static int              nb_peers    = 1;

static struct hosts* parse_args(int argc, char *argv[]);
static int check_uri(char *uri);

static void 
print_usage(void){
    fprintf(stderr, "usage: bmi-ping -s -n <nb_client>|-c <client_id> -h HOST_URI\n");
    fprintf(stderr, "       where:\n");
    fprintf(stderr, "       -s is server and -c is client\n");
    fprintf(stderr, "       HOST_URI is nmad://url\n");
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

void do_client(int id)
{
    /*
     *  CLIENT
     */
    tbx_tick_t t1, t2;
    double sum, lat, bw_million_byte, bw_mbyte;
    int              i       = 0; 
    struct host*     host    = NULL;
    int              nb_h    = hosts->nb_hosts;
    BMI_addr_t      *peer    = NULL;
    bmi_op_id_t     *op_id      = NULL;
    bmi_msg_tag_t   *tag        = 0;

    uint64_t         len        = 0;
    int              ret        = -1;
    bmi_error_code_t error_code = 0;
    int              outcount   = 0;

   
    peer     = malloc(sizeof(BMI_addr_t*)   * hosts->nb_hosts);
    op_id    = malloc(sizeof(bmi_op_id_t*)  * hosts->nb_hosts);
    tag      = malloc(sizeof(bmi_msg_tag_t*)* hosts->nb_hosts);
    send_buf = malloc(sizeof(char*)         * hosts->nb_hosts);
    recv_buf = malloc(sizeof(char*)         * hosts->nb_hosts);

    for(i=0 ;i < hosts->nb_hosts; i++){
	host = (hosts->host+i);
      
	/* get a bmi_addr for the server */
	BMI_addr_lookup(&peer[i], host->hostid);
	send_buf[i] = BMI_memalloc(peer[i], end_len, BMI_SEND);
	recv_buf[i] = BMI_memalloc(peer[i], end_len, BMI_RECV);
	/* create a buffer to recv into */
	fill_buffer(send_buf[i], end_len);
	clear_buffer(recv_buf[i], end_len);

        /* connecting to the server */
	BMI_post_sendunexpected(&op_id[i], peer[i], send_buf[i],
				8, BMI_PRE_ALLOC, 0, NULL, context, NULL);
	{
	    bmi_size_t actual_size=0;
	    bmi_size_t msg_len=8;
	    ret = BMI_wait(op_id[i], &outcount, &error_code, &actual_size, NULL, 10, context);
	    
	    if (ret < 0 || error_code != 0) {
		fprintf(stderr, "data send failed.\n");
		abort();
	    }
	}
	
	BMI_post_recv(&op_id[i], peer[i], &tag[i], sizeof(tag[i]), NULL, BMI_EXT_ALLOC, 0, NULL, context, NULL);
	BMI_wait(op_id[i], NULL, NULL, NULL, NULL, 0, context);
#ifdef DEBUG
	fprintf(stderr, "client #%d: using tag %lx\n", id, tag[i]);
#endif
    }

    for(len=1; len<end_len; len*=2) {
      
	for(loop=0; loop<warmup; loop++) {
	    for(i = 0; i < nb_h; i++)
		BMI_post_send(&op_id[i], peer[i], send_buf[i], len, BMI_PRE_ALLOC, tag[i], NULL, context, NULL);
	    for(i = 0; i < nb_h; i++)
		BMI_wait(op_id[i], &outcount, NULL, NULL, NULL, 0, context);
		
	    for(i = 0; i < nb_h; i++)
		BMI_post_recv(&op_id[i], peer[i], recv_buf[i], len, &len, BMI_PRE_ALLOC, tag[i], NULL, context, NULL);
	    for(i = 0; i < nb_h; i++)
		ret = BMI_wait(op_id[i], &outcount, NULL, NULL, NULL, 0, context);
	}
     
	for(loop=0; loop<iter; loop++) {
	
	    for(i = 0; i < nb_h; i++)
		BMI_post_send(&op_id[i], peer[i], send_buf[i], len, BMI_PRE_ALLOC, tag[i], NULL, context, NULL);
	    for(i = 0; i < nb_h; i++)
		BMI_wait(op_id[i], &outcount, NULL, NULL, NULL, 0, context);

	    for(i = 0; i < nb_h; i++)
		BMI_post_recv(&op_id[i], peer[i], recv_buf[i], len, &len, BMI_PRE_ALLOC, tag[i], NULL, context, NULL);

	    for(i = 0; i < nb_h; i++)
		BMI_wait(op_id[i], &outcount, NULL, NULL, NULL, 0, context);
	}
    }

    for(i = 0; i < nb_h; i++) {
	BMI_memfree(peer[i], send_buf[i], end_len, BMI_SEND);
	BMI_memfree(peer[i], recv_buf[i], end_len, BMI_RECV);
    }
}

void do_server(int id)
{
    /*
     *  SERVER  just one connexion at time
     */
    tbx_tick_t t1, t2;
    double sum, lat, bw_million_byte, bw_mbyte;
    BMI_addr_t      *peer       = NULL;
    bmi_op_id_t     *op_id      = NULL;
    bmi_msg_tag_t   *tag        = NULL;

    uint64_t         len        = 0;
    int              ret        = -1;
    bmi_error_code_t error_code = 0;
    int              outcount   = 0;

    peer     = malloc(sizeof(BMI_addr_t)   * nb_peers);
    op_id    = malloc(sizeof(bmi_op_id_t)  * nb_peers);
    tag      = malloc(sizeof(bmi_msg_tag_t)* nb_peers);
    send_buf = malloc(sizeof(char*)        * nb_peers);
    recv_buf = malloc(sizeof(char*)        * nb_peers);

    struct BMI_unexpected_info request_info;
    int i;
    for(i= 0; i < nb_peers; i++) {
	/* wait for a client to connect */
	outcount = 0;
	do {
	    ret = BMI_testunexpected(1, &outcount, &request_info, 0);
	} while (ret == 0 && outcount == 0);
	
	{
	    if (ret < 0 || request_info.error_code != 0) {
		fprintf(stderr, "Request recv failure (bad state).\n");
		abort();
	    }
	}
	
	/* retrieve the client address */
	peer[i] = request_info.addr;
	BMI_unexpected_free(peer[i], request_info.buffer);

	tag[i] = 1<<i;
	/* send the tag to the client */
#ifdef DEBUG
	printf("server: tag for client %d is %lx\n", i, tag[i]);
#endif
	BMI_post_send(&op_id[i], peer[i], &tag[i], sizeof(tag[i]), BMI_EXT_ALLOC, 0, NULL, context, NULL);
	BMI_wait(op_id[i], NULL, NULL, NULL, NULL, 0, context);

	send_buf[i] = BMI_memalloc(peer[i], end_len, BMI_SEND);
	recv_buf[i] = BMI_memalloc(peer[i], end_len, BMI_RECV);
	
	/* create a buffer to recv into */
	fill_buffer(send_buf[i], end_len);
	clear_buffer(recv_buf[i], end_len);
    }

    printf(" size\t\t|  latency\t|   10^6 B/s\t|   MB/s\t|\n");	    

    for(len=1; len<end_len; len*=2) {

	for( loop=0; loop<warmup; loop++) {
	   
	    for(i=0; i < nb_peers; i++)
		BMI_post_recv(&op_id[i], peer[i], recv_buf[i], len, NULL, BMI_PRE_ALLOC,
			      tag[i], NULL, context, NULL);
	    for(i=0; i < nb_peers; i++)
		BMI_wait(op_id[i], &outcount, NULL, NULL, NULL, 0, context);

	    for(i=0; i < nb_peers; i++)
		BMI_post_send(&op_id[i], peer[i], send_buf[i], len, BMI_PRE_ALLOC, tag[i], 
			      NULL, context, NULL);
	    for(i=0; i < nb_peers; i++)
		BMI_wait(op_id[i], &outcount, NULL, NULL, NULL, 0, context);
	}

	TBX_GET_TICK(t1);

	for( loop=0; loop<iter; loop++) {	
	    for(i=0; i < nb_peers; i++)
		BMI_post_recv(&op_id[i], peer[i], recv_buf[i], len, NULL, BMI_PRE_ALLOC,
			      tag[i], NULL, context, NULL);
	    for(i=0; i < nb_peers; i++)
		BMI_wait(op_id[i], &outcount, NULL, NULL, NULL, 0, context);

	    for(i=0; i < nb_peers; i++)
		BMI_post_send(&op_id[i], peer[i], send_buf[i], len, BMI_PRE_ALLOC, 
			      tag[i], NULL, context, NULL);
	    for(i=0; i < nb_peers; i++)
		BMI_wait(op_id[i], &outcount, NULL, NULL, NULL, 0, context);
	}

	TBX_GET_TICK(t2);

	sum = TBX_TIMING_DELAY(t1, t2);

	lat	      = sum / (2 * iter);
	bw_million_byte = nb_peers * len*(iter / (sum / 2));
	bw_mbyte        = bw_million_byte / 1.048576;


	printf("%d\t\t%lf\t%8.3f\t%8.3f\n", 
	       nb_peers * len, lat, bw_million_byte, bw_mbyte);
	fflush(stdout);
    }
    for(i=0; i < nb_peers; i++) {
	BMI_memfree(peer[i], send_buf[i], end_len, BMI_SEND);
	BMI_memfree(peer[i], recv_buf[i], end_len, BMI_RECV);
    }

}


int
main(int  argc,
     char **argv) {
    uint64_t         len        = 0;
    int              ret        = -1;
    bmi_error_code_t error_code = 0;
    int              outcount   = 0;

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

    ret = BMI_open_context(&context);
    if (ret < 0) {
	fprintf(stderr,"BMI_open_context() ret = %d\n", ret);
	exit(1);
    }    

    if(hosts->host->which == SERVER){
	do_server(0);
    } else {
	do_client(hosts->host->clientid);
    }

    BMI_close_context(context);

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
    char flags[] = "h:c:sn:";
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
	case ('n'):
	    nb_peers = atoi(optarg);
	    hosts->host->which = SERVER;
	    break;

	case ('c'):

	    if ((hosts->host+i)->which == SERVER) {
		fprintf(stderr, "use -s OR -c, not both\n");
		goto parse_args_error;
	    }
	    hosts->host->which = CLIENT;
	    hosts->host->clientid = atoi(optarg);
	    fprintf(stderr, "client #%d\n", hosts->host->clientid);
	    break;
	default:
	    break;
	}
    }
    /* if we didn't get a host argument, bail: */
    if (hosts->host->which == CLIENT && hosts->host->hostid == NULL) {
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
