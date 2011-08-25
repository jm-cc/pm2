
/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 "the PM2 team" (see AUTHORS file)
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

#include "pm2.h"

#include <sys/time.h>

#define MAX_LINKS 32
#define MEGA      (1024*1024)
#define BUF_SIZE  (16*MEGA)

static int TRANSFERT, STOP, ACK, INFO;
static volatile tbx_bool_t finished = tbx_false;

static char buffer[BUF_SIZE];
static unsigned msg_size = BUF_SIZE;

static double link_bw[MAX_LINKS] = { 0.0, };
static unsigned nb_links;

static marcel_sem_t sem;

static void info_service(void)
{
  unsigned link;

  pm2_unpack_int(SEND_CHEAPER, RECV_EXPRESS, &link, 1);
  pm2_unpack_double(SEND_CHEAPER, RECV_CHEAPER, &link_bw[link], 1);
  pm2_rawrpc_waitdata();
}

static void ack_service(void)
{
  pm2_rawrpc_waitdata();

  marcel_sem_V(&sem);
}

static void stop_service(void)
{
  pm2_rawrpc_waitdata();

  pm2_rawrpc_begin(0, ACK, NULL);
  pm2_rawrpc_end();

  finished = tbx_true;
}

static long long total_received = -1;
static tbx_tick_t start_time, now;
static double bandwidth;

static void *update_stats(void *arg)
{
  unsigned link = pm2_self()-1;

  pm2_rawrpc_begin(0, INFO, NULL);
  pm2_pack_int(SEND_CHEAPER, RECV_EXPRESS, &link, 1);
  pm2_pack_double(SEND_CHEAPER, RECV_CHEAPER, &bandwidth, 1);
  pm2_rawrpc_end();

  return NULL;
}

static void transfert_service(void)
{
  pm2_unpack_byte(SEND_CHEAPER, RECV_CHEAPER, buffer, msg_size);
  pm2_rawrpc_waitdata();

  if(total_received == -1) {
    total_received=0;
    TBX_GET_TICK(start_time);
  } else {
    total_received += msg_size;
    TBX_GET_TICK(now);

    bandwidth = ((total_received/MEGA)/(TBX_TIMING_DELAY(start_time, now)*1e-6));
    update_stats(NULL);
  }
}

static void display_thread(void *arg)
{
  double cumul;
  int i;

  while(!finished) {
    marcel_delay(1000);

    cumul = 0.0;
    for(i=0; i<nb_links; i++) {
      fprintf(stderr, "\tBandwidth[link %d] = %f\n", i, link_bw[i]);
      cumul += link_bw[i];
    }
    fprintf(stderr, "TOTAL bandwidth = %f\n", cumul);
  }
}

static void wait_keystroke(void)
{
  char c;

  fprintf(stderr, "\t<Press return to terminate>\n");
  marcel_read(STDIN_FILENO, &c, 1);
}

static void the_master_node(int argc, char *argv[])
{
  int i;

  nb_links = pm2_config_size()/2;

  marcel_sem_init(&sem, 0);

  pm2_thread_create(display_thread, NULL);

  wait_keystroke();

  finished = tbx_true;

  for(i=pm2_config_size()/2+1; i<pm2_config_size(); i++) {
    pm2_rawrpc_begin(i, STOP, NULL);
    pm2_rawrpc_end();
  }

  for(i=1; i<pm2_config_size(); i++)
    marcel_sem_P(&sem);

  pm2_halt();
}

static void receiver_node(int argc, char *argv[])
{}

static void sender_node(int argc, char *argv[])
{
  while(!finished) {
    pm2_rawrpc_begin(pm2_self() - pm2_config_size()/2, TRANSFERT, NULL);
    pm2_pack_byte(SEND_CHEAPER, RECV_CHEAPER, buffer, msg_size);
    pm2_rawrpc_end();
  }

  pm2_rawrpc_begin(pm2_self() - pm2_config_size()/2, STOP, NULL);
  pm2_rawrpc_end();
  
  fprintf(stderr, "Process %d signaled process %d...\n",
	  pm2_self(), pm2_self() - pm2_config_size()/2);
}

int pm2_main(int argc, char **argv)
{
  pm2_rawrpc_register(&TRANSFERT, transfert_service);
  pm2_rawrpc_register(&STOP, stop_service);
  pm2_rawrpc_register(&ACK, ack_service);
  pm2_rawrpc_register(&INFO, info_service);

  pm2_init(argc, argv);

  if(pm2_self() == 0) { /* master process */

    if((pm2_config_size() < 3) || ((pm2_config_size() & 1) == 0)) {
      fprintf(stderr,
	      "This program requires an odd (> 3) number of processes.\n"
	      "Please rerun pm2-conf.\n");
      exit(1);
    }

    the_master_node(argc, argv);

  } else if(pm2_self() > pm2_config_size() / 2) {

    sender_node(argc, argv);

  } else {

    receiver_node(argc, argv);

  }

  pm2_exit();

  return 0;
}
