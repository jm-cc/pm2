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

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>


#include <bmi.h>
#include <bmi-types.h>
#include "tbx.h"
#include "test-bmi.h"

//#define llu(i) (unsigned long long)(i)
#define SERVER          1
#define CLIENT          2
#define MIN_BYTES       1
#define MAX_BYTES       (4<<20)

#define RECV            0
#define SEND            1
#define EXPECTED        0
#define UNEXPECTED      1

#define ITERATIONS      10000

//#define DEBUG           0

struct options{
  char *hostid;           /* host identifier */
  char *method;
  int  which;
  int  test;
};

const char *msg	= "hello, world";
static struct options *parse_args(int argc, char *argv[]);
static int check_uri(char *uri);

static void 
print_usage(void){
  fprintf(stderr, "usage: bmi-hello -h HOST_URI -s|-c [-u]\n");
  fprintf(stderr, "       where:\n");
  fprintf(stderr, "       HOST_URI is nmad://host:port\n");
  fprintf(stderr, "       -s is server and -c is client\n");
  fprintf(stderr, "       -u will use unexpected messages (pass to client only)\n");
  return;
}

int
main(int  argc,
     char **argv) {
  char	              *buf = NULL;
  uint64_t             len = 0;
  struct options      *opts = NULL;
  int                  ret = -1;
  BMI_addr_t           peer_addr = NULL;
  bmi_op_id_t          op_id = NULL;
  bmi_error_code_t     error_code = 0;
  int                  outcount = 0;
  bmi_context_id       context = NULL;
  len = 1+strlen(msg);

  opts = parse_args(argc, argv);
  if (!opts) {
    print_usage();
    exit(1);
  } 
  /*
   * initialize local interface (default options)  
   */
  if(opts->which == SERVER) {
	  /* method: le driver a utiliser */
    ret = BMI_initialize(opts->method, NULL, BMI_INIT_SERVER);
    if (ret < 0) {
      fprintf(stderr,"BMI_initialize() ret = %d\n",ret);
      exit (1);
    }
  } else {  
    
    /* cut address protocol://host -> host*/
    char* tmp = (char *) strdup(opts->hostid);
    tmp = strrchr(tmp, '/');
    opts->hostid = (char *) strdup(tmp+1);
    
    ret = BMI_initialize(NULL, tmp+1, 0);
    if (ret < 0) {
       fprintf(stderr,"BMI_initialize() ret = %d\n",ret);
      exit(1);
    }
  }
  context = TBX_MALLOC(sizeof(struct bmi_context));
  ret = BMI_open_context(context);
  if (ret < 0) {
    fprintf(stderr,"BMI_open_context() ret = %d\n", ret);
    exit(1);
  }
#ifdef DEBUG
  fprintf(stdout,"context %d\n", context->context_id); 
#endif

  if(opts->which == SERVER){
    /* create a buffer to recv into */
    buf = TBX_MALLOC(len);

    if (buf == NULL) {
      fprintf(stderr, "BMI_memalloc() error\n");
      exit(1);
    }
    
    memset(buf, 0, len);

    ret = BMI_post_recv(&op_id, peer_addr, buf, len, &len, 
			BMI_PRE_ALLOC, 0, NULL, context, 
			NULL);
    if (ret !=  0) {
      fprintf(stderr," BMI_post_recv() ret = %d\n",ret);
      exit(1);
    }
    do {
      ret = BMI_test(op_id, &outcount, &error_code,
		     &len, NULL, 10, context);
    } while (ret !=  0);
    
    printf("message recue\t"); printf("buffer contents: %s\n", buf);
    TBX_FREE(buf);
  } else {
    /*
     *  CLIENT
     */
        
    /* get a bmi_addr for the server */
#ifdef DEBUG
    fprintf(stdout,"Server address : %s\n", opts->hostid);
#endif
    BMI_addr_lookup(&peer_addr, opts->hostid);
    buf = TBX_MALLOC(len);
    if ( !buf ) {
      fprintf(stderr, "BMI_memalloc() error\n");
      exit(1);
    }
    memset(buf, 0, len);    strcpy(buf, msg);
    
    ret = BMI_post_send(&op_id, peer_addr, buf, len, 
		  BMI_PRE_ALLOC, 0, NULL, context, NULL);
    if (ret !=  0) {
      fprintf(stderr," BMI_post_send() ret = %d\n",ret);
      exit(1);
    }

    printf("envoi du message\n");                   
    do {                  
      ret = BMI_test(op_id, &outcount, &error_code,
		     &len, NULL, 10, context);
      printf("ret = %d\n");
    } while (ret != 0);    
    printf("\nmessage envoye\n");
    TBX_FREE(buf);
  }

  BMI_close_context(context);
  TBX_FREE(context);
  BMI_finalize();
  exit(0);
}

static int 
check_uri(char *uri){
  int ret = 0; /* failure */
  if (uri[0] == ':' && uri[1] == '/' && uri[2] == '/') ret = 1;
  return ret;
}

static void 
get_method(struct options *opts){
  char *id = opts->hostid;
  
  if (id[0] == 'n' && 
      id[1] == 'm' && 
      id[2] == 'a' && 
      id[3] == 'd' && 
      check_uri(&id[4])) {
    opts->method = strdup("bmi_nmad");
  } else if (id[0] == 't' && 
	     id[1] == 'c' && 
	     id[2] == 'p' && 
	     check_uri(&id[3])) {
    opts->method = strdup("bmi_tcp");
  } else if (id[0] == 'g' && 
	     id[1] == 'm' && 
	     check_uri(&id[2])) {
    opts->method = strdup("bmi_gm");
  } else if (id[0] == 'm' && 
	     id[1] == 'x' && 
	     check_uri(&id[2])) {
    opts->method = strdup("bmi_mx");
  } else if (id[0] == 'i' && 
	     id[1] == 'b' && 
	     check_uri(&id[2])) {
    opts->method = strdup("bmi_ib");
  }
  return;
}


static struct options*
parse_args(int argc, char *argv[]){
  /* getopt stuff */
  extern char *optarg;
  char flags[] = "h:scu";
  int one_opt = 0;
  
  struct options *opts = NULL;

  /* create storage for the command line options */
  opts = (struct options *) calloc(1, sizeof(struct options));
  if (!opts) {
    goto parse_args_error;
  }
    
  /* look at command line arguments */
  while ((one_opt = getopt(argc, argv, flags)) != EOF) {
    switch (one_opt) {
    case ('h'):
               opts->hostid = (char *) strdup(optarg);
	       if (opts->hostid == NULL) {
		 goto parse_args_error;
	       }
	       get_method(opts);
	       break;
    case ('s'):
               if (opts->which == CLIENT) {
		 fprintf(stderr, "use -s OR -c, not both\n");
		 goto parse_args_error;
	       }
	       opts->which = SERVER;
	       break;
    case ('c'):
               if (opts->which == SERVER) {
		 fprintf(stderr, "use -s OR -c, not both\n");
		 goto parse_args_error;
	       }
	       opts->which = CLIENT;
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
  if (opts->hostid == NULL) {
    fprintf(stderr, "you must specify -h\n");

    goto parse_args_error;
  }
  if (opts->method == NULL) {
    fprintf(stderr, "you must use a valid HOST_URI\n");
    goto parse_args_error;
  }
  if (opts->which == 0) {
    fprintf(stderr, "you must specify -s OR -c\n");
    goto parse_args_error;
  }
  
  return (opts);

parse_args_error:

        /* if an error occurs, just free everything and return NULL */
  if (opts) {
    if (opts->hostid) {
      TBX_FREE(opts->hostid);
    }
    TBX_FREE(opts);
  }
  return NULL;
}
