/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2009 "the PM2 team" (see AUTHORS file)
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


#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>
#include <search.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <fxt/fxt.h>
#include <fxt/fut.h>
#include <fut_entries.h>

#define MAX_THREADS 1024

static char **thread_name;
static unsigned int nthreads = 0;
static unsigned int user_tasks = 0;

static fxt_t fut;
struct fxt_ev_64 ev;

static unsigned first_event = 1;
static uint64_t start_time = 0;

/*
 * Paje trace file tools
 */

static char *out_paje_path = "paje.trace";
static char *events_file_path = "events.tmp";
static FILE *out_paje_file = NULL, *events_file;

static void write_paje_header (FILE *);

static void
perror_and_exit (const char *message, int exit_code)
{
  perror (message);
  exit (exit_code);
}

static void
paje_output_file_init (void)
{
  if((out_paje_file = fopen (out_paje_path, "w+")) == NULL)
    perror_and_exit("fopen on out_paje_file failed", -1);

  if((events_file = fopen (events_file_path, "w+")) == NULL)
    {
      fclose(out_paje_file);
      perror_and_exit("fopen on events_file failed", -1);
    }

  write_paje_header (out_paje_file);

  fprintf (out_paje_file, "1       CT_M           0               \"Machine\"	\n");
  fprintf (out_paje_file, "1       CT_VP          CT_M            \"Virtual Processor\"\n");
  fprintf (out_paje_file, "3       ST_VPState     CT_VP           \"VP State\"\n");
  fprintf (out_paje_file, "6       i              ST_VPState      Idle           \"1.0 1.0 1.0\"\n");
  fprintf (out_paje_file, "6       k              ST_VPState      KSoftIRQ       \"0.1 1.0 1.0\"\n");
  fprintf (out_paje_file, "6       r              ST_VPState      Run            \"0.9 0.9 0.0\"\n");
  fprintf (out_paje_file, "6       u              ST_VPState      User           \"1.0 0.0 0.0\"\n");
  fprintf (out_paje_file, "6       f              ST_VPState      ForestGOMP     \".9 0.0 0.0\"\n");
}

static int
paje_output_file_append (FILE *dest_file, FILE *src_file)
{
  int n;
  char buffer[1024];

  dest_file = fopen (out_paje_path, "a+");
  src_file = fopen (events_file_path, "r");

  fseek (dest_file, 0, SEEK_END);

  while ((n = fread (buffer, sizeof (char), 1024, src_file)) > 0)
    fwrite (buffer, sizeof (char), n, dest_file);

  fclose (dest_file);
  fclose (src_file);

  return 0;
}


static void
paje_output_file_terminate (void)
{
  fclose (events_file);
  fclose (out_paje_file);

  paje_output_file_append (out_paje_file, events_file);
}

static int
find_thread_id (unsigned long tid)
{
  char tidstr[16];
  int id;
  ENTRY item;
  ENTRY *res;

  sprintf (tidstr, "%ld", tid);

  item.key = tidstr;
  item.data = NULL;
  res = hsearch (item, FIND);
  assert (res);

  id = (uintptr_t)(res->data);

  return id;
}

/*
 * Generic tools
 */

static int
handle_thread_birth (void)
{
  ENTRY item;
  ENTRY *res;
  uint64_t workerid;
  char *tidstr = malloc (16 * sizeof(char));

  sprintf (tidstr, "%ld", ev.param[0]);

  workerid = nthreads++;
  item.key = tidstr;
  item.data = (void *)workerid;

  res = hsearch (item, FIND);

  if (res != NULL)
    perror_and_exit ("ANALYSING PROF_FILE ERROR : thread_birth", -1);

  res = hsearch(item, ENTER);

  if (res == NULL)
    perror_and_exit ("ANALYSING PROF_FILE ERROR : thread_birth", -1);

  return 0;
}

static int
handle_new_lwp (void)
{
  fprintf (out_paje_file, "7       %f C_VP%lu      CT_VP      C_Machine       \"VP%lu\" \n",
	   (float)(start_time-start_time), ev.param[0], ev.param[0]);

  return 0;
}

static int
handle_switch_to (void)
{
  int thread_id = find_thread_id (ev.param[0]);

  fprintf (events_file, "10       %f	ST_VPState     VP%lu      %c      \"%s\"\n",
	   (float)((ev.time-start_time)/1000000.0),
	   ev.param[2],
	   thread_name[thread_id][0] != 'm' ? thread_name[thread_id][0] : 'u',
	   thread_name[thread_id]);

  return 0;
}

static int
handle_set_thread_name (void)
{
  int th = find_thread_id (ev.param[0]);
  int len = strlen ((char *)&ev.param[1]) + 1;

  if (strcmp ((char *)&ev.param[1], "user_task"))
    {
      thread_name[th] = malloc (len * sizeof (char));
      sprintf (thread_name[th], "%s", (char *)&ev.param[1]);
    }
  else
    {
      thread_name[th] = malloc ((len + 4) * sizeof (char));
      sprintf (thread_name[th], "%s %d", (char *)&ev.param[1], user_tasks++);
    }

  return 0;
}

/*
 * This program should be used to parse the log generated by FxT
 */
int
main (int argc, char **argv)
{
  char *filename;
  int ret;
  int fd_in;
  fxt_blockev_t block;

  if (argc < 2)
    {
      fprintf (stderr, "Usage : %s input_filename [output_filename]\n", argv[0]);
      exit (-1);
    }

  filename = argv[1];

  fd_in = open (filename, O_RDONLY);
  if (fd_in < 0)
    perror_and_exit ("open failed :", -1);

  if (argc > 2)
    out_paje_path = argv[2];
 
  fut = fxt_fdopen (fd_in);
  if (!fut)
    perror_and_exit ("fxt_fdopen :", -1);

  block = fxt_blockev_enter (fut);

  /* create a htable to identify each worker(tid) */
  hcreate (MAX_THREADS);

  thread_name = malloc (MAX_THREADS * sizeof(char *));

  paje_output_file_init ();
  
  while (true)
    {
      ret = fxt_next_ev (block, FXT_EV_TYPE_64, (struct fxt_ev *)&ev);
      if (ret != FXT_EV_OK)
	break;

      if (ev.code == FUT_SET_THREAD_NAME_CODE)
	handle_set_thread_name ();
      else if (ev.code == FUT_THREAD_BIRTH_CODE)
	handle_thread_birth ();
    }

  fxt_rewind (block);

  while (true)
    {
      ret = fxt_next_ev (block, FXT_EV_TYPE_64, (struct fxt_ev *)&ev);
      if (ret != FXT_EV_OK)
	break;

      __attribute__ ((unused)) int nbparam = ev.nb_params;

      if (first_event)
	{
	  first_event = 0;
	  start_time = ev.time;

	  /* Write the Machine level. */
	  fprintf (out_paje_file, "7       %f C_Machine      CT_M      0       \"Machine\" \n", (float)(start_time-start_time));

	  /* Write the LWP running the main thread. */
	  fprintf (out_paje_file, "7       %f C_VP0      CT_VP      C_Machine       \"VP0\" \n", (float)(start_time-start_time));
	}

      switch (ev.code)
	{
	case FUT_SWITCH_TO_CODE:
	  handle_switch_to ();
	  break;

	case FUT_NEW_LWP_CODE:
	  handle_new_lwp ();
	  break;

	default:
	  //fprintf (stderr, "unknown event.. %x at time %llx\n", (unsigned)ev.code, (long long unsigned)ev.time);
	  break;
	}
    }

  free (thread_name);

  paje_output_file_terminate ();
  close(fd_in);

  return 0;
}

static void
write_paje_header (FILE *file)
{
  fprintf (file, "\%%EventDef	PajeDefineContainerType	1\n");
  fprintf (file, "\%%	Alias	string\n");
  fprintf (file, "\%%	ContainerType	string\n");
  fprintf (file, "\%%	Name	string\n");
  fprintf (file, "\%%EndEventDef\n");
  fprintf (file, "\%%EventDef	PajeDefineEventType	2\n");
  fprintf (file, "\%%	Alias	string\n");
  fprintf (file, "\%%	ContainerType	string\n");
  fprintf (file, "\%%	Name	string\n");
  fprintf (file, "\%%EndEventDef\n");
  fprintf (file, "\%%EventDef	PajeDefineStateType	3\n");
  fprintf (file, "\%%	Alias	string\n");
  fprintf (file, "\%%	ContainerType	string\n");
  fprintf (file, "\%%	Name	string\n");
  fprintf (file, "\%%EndEventDef\n");
  fprintf (file, "\%%EventDef	PajeDefineVariableType	4\n");
  fprintf (file, "\%%	Alias	string\n");
  fprintf (file, "\%%	ContainerType	string\n");
  fprintf (file, "\%%	Name	string\n");
  fprintf (file, "\%%EndEventDef\n");
  fprintf (file, "\%%EventDef	PajeDefineLinkType	5\n");
  fprintf (file, "\%%	Alias	string\n");
  fprintf (file, "\%%	ContainerType	string\n");
  fprintf (file, "\%%	SourceContainerType	string\n");
  fprintf (file, "\%%	DestContainerType	string\n");
  fprintf (file, "\%%	Name	string\n");
  fprintf (file, "\%%EndEventDef\n");
  fprintf (file, "\%%EventDef	PajeDefineEntityValue	6\n");
  fprintf (file, "\%%	Alias	string\n");
  fprintf (file, "\%%	EntityType	string\n");
  fprintf (file, "\%%	Name	string\n");
  fprintf (file, "\%%	Color	color\n");
  fprintf (file, "\%%EndEventDef\n");
  fprintf (file, "\%%EventDef	PajeCreateContainer	7\n");
  fprintf (file, "\%%	Time	date\n");
  fprintf (file, "\%%	Alias	string\n");
  fprintf (file, "\%%	Type	string\n");
  fprintf (file, "\%%	Container	string\n");
  fprintf (file, "\%%	Name	string\n");
  fprintf (file, "\%%EndEventDef\n");
  fprintf (file, "\%%EventDef	PajeDestroyContainer	8\n");
  fprintf (file, "\%%	Time	date\n");
  fprintf (file, "\%%	Name	string\n");
  fprintf (file, "\%%	Type	string\n");
  fprintf (file, "\%%EndEventDef\n");
  fprintf (file, "\%%EventDef	PajeNewEvent	9\n");
  fprintf (file, "\%%	Time	date\n");
  fprintf (file, "\%%	Type	string\n");
  fprintf (file, "\%%	Container	string\n");
  fprintf (file, "\%%	Value	string\n");
  fprintf (file, "\%%EndEventDef\n");
  fprintf (file, "\%%EventDef PajeSetState 10\n");
  fprintf (file, "\%%	Time	date\n");
  fprintf (file, "\%%	Type	string\n");
  fprintf (file, "\%%	Container	string\n");
  fprintf (file, "\%%	Value	string\n");
  fprintf (file, "\%%       ThreadName      string\n");
  fprintf (file, "\%%EndEventDef\n");
  fprintf (file, "\%%EventDef	PajePushState	11\n");
  fprintf (file, "\%%	Time	date\n");
  fprintf (file, "\%%	Type	string\n");
  fprintf (file, "\%%	Container	string\n");
  fprintf (file, "\%%	Value	string\n");
  fprintf (file, "\%%EndEventDef\n");
  fprintf (file, "\%%EventDef	PajePushState	111\n");
  fprintf (file, "\%%	Time	date\n");
  fprintf (file, "\%%	Type	string\n");
  fprintf (file, "\%%	Container	string\n");
  fprintf (file, "\%%	Value	string\n");
  fprintf (file, "\%%	Object	string\n");
  fprintf (file, "\%%EndEventDef\n");
  fprintf (file, "\%%EventDef	PajePopState	12\n");
  fprintf (file, "\%%	Time	date\n");
  fprintf (file, "\%%	Type	string\n");
  fprintf (file, "\%%	Container	string\n");
  fprintf (file, "\%%EndEventDef\n");
  fprintf (file, "\%%EventDef	PajeSetVariable	13\n");
  fprintf (file, "\%%	Time	date\n");
  fprintf (file, "\%%	Type	string\n");
  fprintf (file, "\%%	Container	string\n");
  fprintf (file, "\%%	Value	double\n");
  fprintf (file, "\%%EndEventDef\n");
  fprintf (file, "\%%EventDef	PajeAddVariable	14\n");
  fprintf (file, "\%%	Time	date\n");
  fprintf (file, "\%%	Type	string\n");
  fprintf (file, "\%%	Container	string\n");
  fprintf (file, "\%%	Value	double\n");
  fprintf (file, "\%%EndEventDef\n");
  fprintf (file, "\%%EventDef	PajeSubVariable	15\n");
  fprintf (file, "\%%	Time	date\n");
  fprintf (file, "\%%	Type	string\n");
  fprintf (file, "\%%	Container	string\n");
  fprintf (file, "\%%	Value	double\n");
  fprintf (file, "\%%EndEventDef\n");
  fprintf (file, "\%%EventDef	PajeStartLink	16\n");
  fprintf (file, "\%%	Time	date\n");
  fprintf (file, "\%%	Type	string\n");
  fprintf (file, "\%%	Container	string\n");
  fprintf (file, "\%%	Value	string\n");
  fprintf (file, "\%%	SourceContainer	string\n");
  fprintf (file, "\%%	Key	string\n");
  fprintf (file, "\%%	Size	int\n");
  fprintf (file, "\%%EndEventDef\n");
  fprintf (file, "\%%EventDef	PajeEndLink	17\n");
  fprintf (file, "\%%	Time	date\n");
  fprintf (file, "\%%	Type	string\n");
  fprintf (file, "\%%	Container	string\n");
  fprintf (file, "\%%	Value	string\n");
  fprintf (file, "\%%	DestContainer	string\n");
  fprintf (file, "\%%	Key	string\n");
  fprintf (file, "\%%	Size	int\n");
  fprintf (file, "\%%EndEventDef\n");
  fprintf (file, "\%%EventDef	PajeStartLink	18\n");
  fprintf (file, "\%%	Time	date\n");
  fprintf (file, "\%%	Type	string\n");
  fprintf (file, "\%%	Container	string\n");
  fprintf (file, "\%%	Value	string\n");
  fprintf (file, "\%%	SourceContainer	string\n");
  fprintf (file, "\%%	Key	string\n");
  fprintf (file, "\%%EndEventDef\n");
  fprintf (file, "\%%EventDef	PajeEndLink	19\n");
  fprintf (file, "\%%	Time	date\n");
  fprintf (file, "\%%	Type	string\n");
  fprintf (file, "\%%	Container	string\n");
  fprintf (file, "\%%	Value	string\n");
  fprintf (file, "\%%	DestContainer	string\n");
  fprintf (file, "\%%	Key	string\n");
  fprintf (file, "\%%EndEventDef\n");
  fprintf (file, "\%%EventDef	PajeNewEvent   112\n");
  fprintf (file, "\%%	Time	date\n");
  fprintf (file, "\%%	Type	string\n");
  fprintf (file, "\%%	Container	string\n");
  fprintf (file, "\%%	Value	string\n");
  fprintf (file, "\%%       ThreadName      string\n");
  fprintf (file, "\%%       ThreadGroup     string\n");
  fprintf (file, "\%%       ThreadParent    string\n");
  fprintf (file, "\%%EndEventDef\n");
}
