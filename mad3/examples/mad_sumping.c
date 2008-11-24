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

/*
 * sumping.c
 * ==========
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "pm2_common.h"

#ifdef MARCEL

// Types
//......................
typedef struct s_mad_ping_result
{
  ntbx_process_lrank_t lrank_src;
  ntbx_process_lrank_t lrank_dst;
  int                  size;
  double               latency;
  double               bandwidth_mbit;
  double               bandwidth_mbyte;
} mad_ping_result_t, *p_mad_ping_result_t;

typedef struct {
  p_mad_madeleine_t madeleine;
  char *name;
  marcel_t tid;
  int nb_calculus_thread;
  int nb_channel;
  int sum;
  int repeat_sum;
  volatile int *end;
} thread_infos_t;

typedef struct {
  int inf, sup, res;
  marcel_sem_t sem;
} job;

// Static variables
//......................
#define SIZE_PING 1*8

static const int param_control_receive   = 0;
static const int param_send_mode         = mad_send_CHEAPER;
static const int param_receive_mode      = mad_receive_CHEAPER;
static const int param_nb_samples        = 1;
//static const int param_min_size          = MAD_LENGTH_ALIGNMENT;
//static const int param_max_size          = 1024*1024*2;
//static const int param_min_size          = SIZE_PING;
static int param_size                    = SIZE_PING;
static int param_nb_iteration            = 5;
volatile static int param_sum            = 0;
static const int param_nb_tests          = 1;
static const int param_no_zero           = 1;
static const int param_fill_buffer       = 1;
static const int param_fill_buffer_value = 1;
static const int param_one_way           = 0;

static ntbx_process_grank_t process_grank = -1;
static ntbx_process_lrank_t process_lrank = -1;
static ntbx_process_lrank_t master_lrank  = -1;

static unsigned char *main_buffer = NULL;

// Functions
//......................

static void
mark_buffer(int len) TBX_UNUSED;

static void
mark_buffer(int len)
{
  unsigned int n = 0;
  int          i = 0;

  for (i = 0; i < len; i++)
    {
      unsigned char c = 0;

      n += 7;
      c = (unsigned char)(n % 256);

      main_buffer[i] = c;
    }
}

static void
clear_buffer(void) TBX_UNUSED;

static void
clear_buffer(void)
{
  memset(main_buffer, 0, param_size);
}

static void
fill_buffer(void)
{
  memset(main_buffer, param_fill_buffer_value, param_size);
}

static void
control_buffer(int len) TBX_UNUSED;

static void
control_buffer(int len)
{
  tbx_bool_t   ok = tbx_true;
  unsigned int n  = 0;
  int          i  = 0;

  for (i = 0; i < len; i++)
    {
      unsigned char c = 0;

      n += 7;
      c = (unsigned char)(n % 256);

      if (main_buffer[i] != c)
	{
	  int v1 = 0;
	  int v2 = 0;

	  v1 = c;
	  v2 = main_buffer[i];
	  DISP("Bad data at byte %X: expected %X, received %X", i, v1, v2);
	  ok = tbx_false;
	}
    }

  if (!ok)
    {
      DISP("%d bytes reception failed", len);
    }
}

static
void
process_results(p_mad_channel_t     channel,
		p_mad_ping_result_t results)
{
  LOG_IN();
  if (process_lrank == master_lrank)
    {
      mad_ping_result_t tmp_results = { 0, 0, 0, 0.0, 0.0, 0.0 };

      if (!results)
	{
	  p_mad_connection_t in = NULL;
	  ntbx_pack_buffer_t buffer;

	  in = mad_begin_unpacking(channel);

	  mad_unpack(in, &buffer, sizeof(buffer),
		     mad_send_CHEAPER, mad_receive_EXPRESS);
	  tmp_results.lrank_src       = ntbx_unpack_int(&buffer);

	  mad_unpack(in, &buffer, sizeof(buffer),
		     mad_send_CHEAPER, mad_receive_EXPRESS);
	  tmp_results.lrank_dst       = ntbx_unpack_int(&buffer);

	  mad_unpack(in, &buffer, sizeof(buffer),
		     mad_send_CHEAPER, mad_receive_EXPRESS);
	  tmp_results.size            = ntbx_unpack_int(&buffer);

	  mad_unpack(in, &buffer, sizeof(buffer),
		     mad_send_CHEAPER, mad_receive_EXPRESS);
	  tmp_results.latency         = ntbx_unpack_double(&buffer);

	  mad_unpack(in, &buffer, sizeof(buffer),
		     mad_send_CHEAPER, mad_receive_EXPRESS);
	  tmp_results.bandwidth_mbit  = ntbx_unpack_double(&buffer);

	  mad_unpack(in, &buffer, sizeof(buffer),
		     mad_send_CHEAPER, mad_receive_EXPRESS);
	  tmp_results.bandwidth_mbyte = ntbx_unpack_double(&buffer);

	  mad_end_unpacking(in);

	  results = &tmp_results;
	}

      LDISP("%3d %3d %12d %12.3f %8.3f %8.3f",
	    results->lrank_src,
	    results->lrank_dst,
	    results->size,
	    results->latency,
	    results->bandwidth_mbit,
	    results->bandwidth_mbyte);
    }
  else if (results)
    {
      p_mad_connection_t out = NULL;
      ntbx_pack_buffer_t buffer;

      out = mad_begin_packing(channel, master_lrank);

      ntbx_pack_int(results->lrank_src, &buffer);
      mad_pack(out, &buffer, sizeof(buffer),
	       mad_send_CHEAPER, mad_receive_EXPRESS);

      ntbx_pack_int(results->lrank_dst, &buffer);
      mad_pack(out, &buffer, sizeof(buffer),
	       mad_send_CHEAPER, mad_receive_EXPRESS);

      ntbx_pack_int(results->size, &buffer);
      mad_pack(out, &buffer, sizeof(buffer),
	       mad_send_CHEAPER, mad_receive_EXPRESS);

      ntbx_pack_double(results->latency, &buffer);
      mad_pack(out, &buffer, sizeof(buffer),
	       mad_send_CHEAPER, mad_receive_EXPRESS);

      ntbx_pack_double(results->bandwidth_mbit, &buffer);
      mad_pack(out, &buffer, sizeof(buffer),
	       mad_send_CHEAPER, mad_receive_EXPRESS);

      ntbx_pack_double(results->bandwidth_mbyte, &buffer);
      mad_pack(out, &buffer, sizeof(buffer),
	       mad_send_CHEAPER, mad_receive_EXPRESS);

      mad_end_packing(out);
    }

  LOG_OUT();
}

static
void
ping(p_mad_channel_t      channel,
     ntbx_process_lrank_t lrank_dst)
{
  int size = 0;

  LOG_IN();
  DISP("ping (channel %s) with : %i", channel->name, lrank_dst);

  if (param_fill_buffer)
    {
      fill_buffer();
    }

  marcel_yield();
  marcel_yield();
  marcel_yield();


  size=param_size;
    {
      const int         _size   = (!size && param_no_zero)?1:size;
/*        int               dummy   = 0; */
      mad_ping_result_t results = { 0, 0, 0, 0.0, 0.0, 0.0 };
      double            sum     = 0.0;
      tbx_tick_t        t1;
      tbx_tick_t        t2;
      int nb_tests = param_nb_tests;

      sum = 0;

      while (nb_tests--)
	{
	  int nb_samples = param_nb_samples;

	  TBX_GET_TICK(t1);
	  while (nb_samples--)
	    {
	      p_mad_connection_t connection = NULL;

	      connection = mad_begin_packing(channel, lrank_dst);
	      mad_pack(connection, main_buffer, _size,
		       param_send_mode, param_receive_mode);
	      mad_end_packing(connection);

	      connection = mad_begin_unpacking(channel);
	      mad_unpack(connection, main_buffer, _size,
			 param_send_mode, param_receive_mode);
	      mad_end_unpacking(connection);
	    }
	  TBX_GET_TICK(t2);

	  sum += TBX_TIMING_DELAY(t1, t2);
	}


      results.lrank_src       = process_lrank;
      results.lrank_dst       = lrank_dst;
      results.size            = _size;
      results.bandwidth_mbit  =
	(_size * (double)param_nb_tests * (double)param_nb_samples)
	/ (sum / (2 - param_one_way));

      results.bandwidth_mbyte = results.bandwidth_mbit / 1.048576;
      results.latency         =
	sum / param_nb_tests / param_nb_samples / (2 - param_one_way);

      process_results(channel, &results);
    }
  LOG_OUT();
}

static
void
pong(p_mad_channel_t      channel,
     ntbx_process_lrank_t lrank_dst)
{
  int size = 0;

  LOG_IN();
  DISP("pong (channel %s) with : %i", channel->name, lrank_dst);

  size=param_size;
    {
      const int _size = (!size && param_no_zero)?1:size;
      int nb_tests = param_nb_tests;

      while (nb_tests--)
	{
	  int nb_samples = param_nb_samples;

	  while (nb_samples--)
	    {
	      p_mad_connection_t connection = NULL;

	      connection = mad_begin_unpacking(channel);
	      mad_unpack(connection, main_buffer, _size,
			 param_send_mode, param_receive_mode);
	      mad_end_unpacking(connection);

	      connection = mad_begin_packing(channel, lrank_dst);
	      mad_pack(connection, main_buffer, _size,
		       param_send_mode, param_receive_mode);
	      mad_end_packing(connection);
	    }
	}
      process_results(channel, NULL);
    }
  LOG_OUT();
}

static
void
play_with_channel(p_mad_madeleine_t  madeleine,
		  char              *name)
{
  p_mad_channel_t            channel = NULL;
  p_ntbx_process_container_t pc      = NULL;
  tbx_bool_t                 status  = tbx_false;
  ntbx_pack_buffer_t         buffer;

  //DISP_STR("Channel", name);
  channel = tbx_htable_get(madeleine->channel_htable, name);
  if (!channel)
    {
      DISP("I don't belong to the channel %s", name);

      return;
    }

  pc = channel->pc;

  status = ntbx_pc_first_local_rank(pc, &master_lrank);
  if (!status)
    return;

  process_lrank = ntbx_pc_global_to_local(pc, process_grank);
  DISP("Client : My local channel (%s) rank is : %i", name, process_lrank);

  if (process_lrank != master_lrank)
    {
      DISP("nothing to do");
    }
  else
    {
      // Master
      ntbx_process_lrank_t lrank_dst = -1;
      int                  i         =  0;

      LDISP_STR("Channel", name);
      LDISP("src|dst|size        |latency     |10^6 B/s|MB/s    |");

      while(param_sum || (i++ < param_nb_iteration)) {
	ntbx_pc_first_local_rank(pc, &lrank_dst);
	while (ntbx_pc_next_local_rank(pc, &lrank_dst)) {

	  p_mad_connection_t out = NULL;
	  //p_mad_connection_t in  = NULL;

	  out = mad_begin_packing(channel, lrank_dst);
	  ntbx_pack_int(master_lrank, &buffer);
	  mad_pack(out, &buffer, sizeof(buffer),
		   mad_send_CHEAPER, mad_receive_EXPRESS);
	  ntbx_pack_int(0, &buffer);
	  mad_pack(out, &buffer, sizeof(buffer),
		   mad_send_CHEAPER, mad_receive_EXPRESS);
	  mad_end_packing(out);

	  //in = mad_begin_unpacking(channel);
	  //mad_unpack(in, &buffer, sizeof(buffer),
	  //     mad_send_CHEAPER, mad_receive_EXPRESS);
	  //mad_end_unpacking(in);

	  ping(channel, lrank_dst);
	  if (param_sum) {
	    marcel_delay(200);
	  }
	}
      }
      DISP("test series completed");
      ntbx_pc_first_local_rank(pc, &lrank_dst);
      while (ntbx_pc_next_local_rank(pc, &lrank_dst))
	{
	  p_mad_connection_t out = NULL;

	  out = mad_begin_packing(channel, lrank_dst);
	  ntbx_pack_int(master_lrank, &buffer);
	  mad_pack(out, &buffer, sizeof(buffer),
		   mad_send_CHEAPER, mad_receive_EXPRESS);
	  ntbx_pack_int(1, &buffer);
	  mad_pack(out, &buffer, sizeof(buffer),
		   mad_send_CHEAPER, mad_receive_EXPRESS);
	  mad_end_packing(out);
	}
    }
}

void* server_thread(thread_infos_t * info)
{
  p_mad_madeleine_t  madeleine = info->madeleine;
  char              *name = info->name;
  p_mad_channel_t            channel = NULL;
  p_ntbx_process_container_t pc      = NULL;
  tbx_bool_t                 status  = tbx_false;
  ntbx_pack_buffer_t         buffer;

  DISP_STR("Channel starting", name);
  channel = tbx_htable_get(madeleine->channel_htable, name);
  if (!channel)
    {
      DISP("I don't belong to the channel %s", name);

      return NULL;
    }

  pc = channel->pc;

  status = ntbx_pc_first_local_rank(pc, &master_lrank);
  if (!status)
    return NULL;

  process_lrank = ntbx_pc_global_to_local(pc, process_grank);
  DISP("Server : My local channel (%s) rank is : %i", name, process_lrank);

  if (process_lrank == master_lrank)
    {
      DISP("Master server exciting");
      return NULL;
    }

  /* Slaves */
  while (1)
    {
      p_mad_connection_t   in             = NULL;
      ntbx_process_lrank_t its_local_rank =   -1;
      int                  fin            =    0;

      in = mad_begin_unpacking(channel);
      mad_unpack(in, &buffer, sizeof(buffer),
		 mad_send_CHEAPER, mad_receive_EXPRESS);
      its_local_rank = ntbx_unpack_int(&buffer);
      mad_unpack(in, &buffer, sizeof(buffer),
		 mad_send_CHEAPER, mad_receive_EXPRESS);
      fin = ntbx_unpack_int(&buffer);
      mad_end_unpacking(in);

      if (its_local_rank == process_lrank) {
	DISP("Ping with ourself !");
	return NULL;
      }

      if (fin)
	{
	  DISP_STR("Channel stoping", name);
	  break;
	}
      else
	{
	  /* Pong */
	  pong(channel, its_local_rank);
	}
    }
  DISP_STR("Channel stopped", name);
  return NULL;
}

void* thread_for_channel(thread_infos_t * info)
{
  play_with_channel(info->madeleine, info->name);
  return NULL;
}

static __inline__ void job_init(job *j, int inf, int sup)
{
  j->inf = inf;
  j->sup = sup;
  marcel_sem_init(&j->sem, 0);
}

marcel_attr_t static sum_attr = MARCEL_ATTR_INITIALIZER;

any_t sum(any_t arg)
{
  job *j = (job *)arg;
  job j1, j2;

  if(j->inf == j->sup) {
    j->res = j->inf;
    marcel_sem_V(&j->sem);
    return NULL;
  }

  job_init(&j1, j->inf, (j->inf+j->sup)/2);
  job_init(&j2, (j->inf+j->sup)/2+1, j->sup);

  marcel_create(NULL, &sum_attr, sum, (any_t)&j1);
  marcel_create(NULL, &sum_attr, sum, (any_t)&j2);

  marcel_sem_P(&j1.sem);
  marcel_sem_P(&j2.sem);

  j->res = j1.res+j2.res;
  marcel_sem_V(&j->sem);
  return NULL;
}

static tbx_tick_t t1, t2;

void* sum_thread(thread_infos_t * info)
{
  job j;
  unsigned long temps;
  int i;

  marcel_attr_setdetachstate(&sum_attr, tbx_true);
  //marcel_attr_setschedpolicy(&attr, MARCEL_SCHED_AFFINITY);

  marcel_sem_init(&j.sem, 0);
  j.inf = 1;
  j.sup = info->sum;
  TBX_GET_TICK(t1);
  for(i=1; i<info->repeat_sum; i++) {
    marcel_create(NULL, &sum_attr, sum, (any_t)&j);
    marcel_sem_P(&j.sem);
  }
  TBX_GET_TICK(t2);
  DISP("Sum from 1 to %d = %d\n", j.sup, j.res);
  temps = TBX_TIMING_DELAY(t1, t2);
  DISP("time = %ld.%03ldms\n", temps/1000, temps%1000);

  return NULL;
}

void* calculus_thread(thread_infos_t * info)
{
  DISP("Starting calculus thread %p", info->tid);
  while (!*(info->end)) ;
  DISP("Ending calculus thread %p", info->tid);
  return NULL;
}

void *main_thread(thread_infos_t * info)
{
  p_mad_madeleine_t madeleine = info->madeleine;
  p_tbx_slist_t     slist     = NULL;
  thread_infos_t *calculus_threads = NULL;
  thread_infos_t sum_info = {0,};
  volatile int end=0;
  int i;

  DISP("Main thread starting");
  // Démarrage des threads "de calcul"
  if (info->nb_calculus_thread) {
    calculus_threads = tbx_aligned_malloc(info->nb_calculus_thread*
					  sizeof(thread_infos_t),
					  MAD_ALIGNMENT);
    for(i=0;i<info->nb_calculus_thread; i++) {
      calculus_threads[i].end=&end;
      marcel_create(&calculus_threads[i].tid, NULL, (void*) calculus_thread,
		    &calculus_threads[i]);
    }

  }

  slist = madeleine->public_channel_slist;
  if (tbx_slist_is_nil(slist)) {
    DISP("No channels");
  } else {
    int nb_channel = tbx_slist_get_length(slist);
    thread_infos_t *thread_params_tbl = NULL;
    thread_infos_t *thread_params = NULL;
    p_tbx_slist_t     slist_head  = slist;
    marcel_attr_t attr;
    marcel_attr_init(&attr);
    marcel_attr_setrealtime(&attr, tbx_true);

    main_buffer = tbx_aligned_malloc(param_size, MAD_ALIGNMENT);
    thread_params_tbl = tbx_aligned_malloc(nb_channel*2*
					   sizeof(thread_infos_t),
					   MAD_ALIGNMENT);

    // Démarrage des threads "pong" (de réponse)
    thread_params=thread_params_tbl;
    slist = slist_head;
    tbx_slist_ref_to_head(slist);
    i=info->nb_channel;
    do {
      if (! i--) break;
      thread_params->name = tbx_slist_ref_get(slist);
      thread_params->madeleine = madeleine;
      marcel_create(&thread_params->tid, &attr, (void*) server_thread,
		    thread_params);
      thread_params++;
    } while (tbx_slist_ref_forward(slist));

    if (info->sum > 0) {
      param_sum=1;
    }

    // Démarrage des threads "ping"
    slist = slist_head;
    tbx_slist_ref_to_head(slist);
    i=info->nb_channel;
    do {
      if (! i--) break;
      thread_params->name = tbx_slist_ref_get(slist);
      thread_params->madeleine = madeleine;
      marcel_create(&thread_params->tid, &attr,
		    (void*) thread_for_channel,
		    thread_params);
      thread_params++;
    } while (tbx_slist_ref_forward(slist));

    // Démarrage du calcul de la somme
    if (info->sum > 0) {
      sum_info.sum=info->sum;
      DISP("Starting sum thread (%i)", sum_info.sum);
      marcel_create(&sum_info.tid, NULL, (void*) sum_thread,
		    &sum_info);
    }

    // Attente de la fin du thread de somme
    if (info->sum > 0) {
      DISP("Stopping sum thread");
      marcel_join(sum_info.tid, NULL);
      param_sum=0;
      DISP("Sum thread stopped");
    }

    // Attente de la fin des threads ping
    thread_params=thread_params_tbl;
    slist = slist_head;
    tbx_slist_ref_to_head(slist);
    i=info->nb_channel;
    do {
      if (! i--) break;
      marcel_join(thread_params->tid, NULL);
      thread_params++;
    } while (tbx_slist_ref_forward(slist));

    // Attente de la fin des threads pong
    slist = slist_head;
    tbx_slist_ref_to_head(slist);
    i=info->nb_channel;
    do {
      if (! i--) break;
      marcel_join(thread_params->tid, NULL);
      thread_params++;
    } while (tbx_slist_ref_forward(slist));

    tbx_aligned_free(thread_params_tbl, MAD_ALIGNMENT);
    tbx_aligned_free(main_buffer, MAD_ALIGNMENT);
  }

  if (info->nb_calculus_thread) {
    DISP("Stopping calculus threads");
    end=1;

    for(i=0;i<info->nb_calculus_thread; i++) {
      marcel_join(calculus_threads[i].tid, NULL);
    }
  }
  DISP("Main thread ending");
  return NULL;
}

int
marcel_main(int argc, char *argv[])
{
  p_mad_madeleine_t madeleine = NULL;
  p_mad_session_t   session   = NULL;
  thread_infos_t thread_params = {0,};

#ifdef USE_MAD_INIT
  madeleine = mad_init(&argc, argv);
  TRACE("Returned from mad_init");
#else // USE_MAD_INIT

  common_pre_init(&argc, argv, NULL);
  common_post_init(&argc, argv, NULL);
  TRACE("Returned from common_init");
  madeleine = mad_get_madeleine();
#endif // USE_MAD_INIT

  session       = madeleine->session;
  process_grank = session->process_rank;
  DISP_VAL("My global rank is", process_grank);
  DISP_VAL("The configuration size is",
	   tbx_slist_get_length(madeleine->dir->process_slist));

  //  DISP_VAL("argc", argc);
  thread_params.nb_channel=-1;
  thread_params.repeat_sum=1;
  while (argv++, --argc) {
    //DISP_VAL("argc", argc);
    //DISP("%s", argv[0]);
    if (strcmp(argv[0], "-s") == 0) {
      thread_params.sum=atoi(argv[1]);
      argc--; argv++;
    } else if (strcmp(argv[0], "-c") == 0) {
      thread_params.nb_calculus_thread=atoi(argv[1]);
      argc--; argv++;
    } else if (strcmp(argv[0], "-C") == 0) {
      thread_params.nb_channel=atoi(argv[1]);
      argc--; argv++;
    } else if (strcmp(argv[0], "-p") == 0) {
      param_size=atoi(argv[1]);
      argc--; argv++;
    } else if (strcmp(argv[0], "-i") == 0) {
      param_nb_iteration=atoi(argv[1]);
      argc--; argv++;
    } else {
    //    if (strcmp(argv[0], "-h") == 0) {
      DISP("Unkown parametre %s", argv[0]);
      DISP("Usage : [-s sum] [-c nb_calcul] [-C nb_channel]"
	   " [-h] [-p ping_size] [-i ping_iteration]");
      goto end;
    }
  }

  DISP_VAL("Number of calculus threads", thread_params.nb_calculus_thread);
  DISP_VAL("Sum to compute", thread_params.sum);
  DISP_VAL("Number of times to compute the sum", thread_params.repeat_sum);
  DISP_VAL("Number of channels to use", thread_params.nb_channel);
  DISP_VAL("Size of ping", param_size);
  DISP_VAL("Number of ping after the sum computed", param_nb_iteration);


  // Démarrage du thread principal en RT (pour qu'il démarre
  // rapidement tous les threads).
  {
    marcel_attr_t attr;

    thread_params.madeleine = madeleine;

    marcel_attr_init(&attr);
    marcel_attr_setrealtime(&attr, tbx_true);

    marcel_create(&thread_params.tid, &attr, (void*) main_thread,
		  &thread_params);
    marcel_join(thread_params.tid, NULL);
  }

 end:
  DISP("Exiting");

  // mad_exit(madeleine);
  common_exit(NULL);

  return 0;
}
#else  /* MARCEL */

int
main(int    argc,
     char **argv)
{
  common_pre_init(&argc, argv, NULL);
  common_post_init(&argc, argv, NULL);
  DISP("This program requires Marcel to be compiled in. Exiting.");
  common_exit(NULL);

  return 0;
}

#endif /* MARCEL */
