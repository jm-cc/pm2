

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

#include <stdio.h>
#include "sigmund.h"
#include "parser.h"
#include <stdlib.h>
#include "tracelib.h"
#include <string.h>
#include <fkt/names.h>

#include "fut.h"

/* deals with the main function,
   creates the filters with the selected parameters
   runs the action */

enum action {NONE, LIST_EVENTS, NB_EVENTS, NTH_EVENT, ACTIVE_TIME,
	     IDLE_TIME, TIME, NB_CALLS, ACTIVE_SLICES, IDLE_SLICES,
	     AVG_ACTIVE_SLICE, FUNCTION_TIME}; // Possible action

// Prints the header when printing events
void print_help()
{
  printf("type  date_tick  pid cpu  thr   code                          name                  args\n");  
}


// Prints an event tr accordingly to the format given above
void print_trace(trace tr)
{
  unsigned j;
  printf("%s",(tr.type == USER)? "USER: " : "KERN: ");
  printf("%9u ",(unsigned) tr.clock);
  printf("%5u  %2u  %2u ", tr.pid, tr.cpu, tr.thread);
  printf("%6x",tr.code);
  if (tr.type == USER) {
    printf("%40s", fut_code2name(tr.code));
  } else {
    if (tr.code >= FKT_UNSHIFTED_LIMIT_CODE)
      printf("%40s", fkt_code2name(tr.code));
    else {
      if (tr.code < FKT_TRAP_BASE) {
	printf("\t\t\t       system call   %3u", tr.code);
	printf("   %s", fkt_syscalls[tr.code]);
      }
      else if (tr.code < FKT_TRAP_LIMIT_CODE) {
	j = tr.code - FKT_TRAP_BASE;
	printf("\t\t\t\t      trap   %3u", j);
	printf("   %s", fkt_traps[j]);
      }
      else {
	j = tr.code -  FKT_IRQ_TIMER;
	printf("\t\t\t\t       IRQ   %3u", j);
	printf("   %s", fkt_find_irq(j));
      }
    }
  }
  for (j = 0; j<tr.nbargs; j++)
    printf("%14u  ",tr.args[j]);
  printf("\n");
}


/* The handlers */

// List events
void list_events()
{
  trace tr;
  print_help();
  for(;;) {
    switch(get_next_filtered_trace(&tr))
      {
      case 0 : { print_trace(tr); break;}    // Not end of file
      case 1 : { return; }                   // End of file and last one not valid
      case 2 : { print_trace(tr); return;}   // End of file and last one is valid
      default: { 
	fprintf(stderr,"Please report bug to cmenier@ens-lyon.fr\n");
	exit(1);
      }
      }
  }
}

// Prints the number of events 
void nb_events()
{
  int n;
  int eof;
  trace tr;
  for(n = 0; (eof = get_next_filtered_trace(&tr)) == 0; n++);
  if (eof == 2) n++;
  printf("%d événements\n",n);
}

// Prints the nth event taking account of the filter
void nth_event(int nth)
{
  int n;
  int eof;
  trace tr;
  print_help();
  for(n = 1; ; n++) {
    if ((eof = get_next_filtered_trace(&tr)) != 0) break;
    if (n == nth) {
      print_trace(tr);
      return;
    }
  }
  if ((eof == 2) && (n+1 == nth)){
    print_trace(tr);
    return;
  }
  printf("L'événement %d n'a pas pu être trouvé.\n", nth);
}

// Prints the total active time taking account of the filter
void active_time()
{
  trace tr;
  int eof = 0;
  while (eof == 0) 
    eof = get_next_filtered_trace(&tr);
  printf("Temps actif total = %u\n",(unsigned) get_active_time());
}

// Prints the total idle timem taking account of the filter
void idle_time()
{
  trace tr;
  int eof = 0;
  while (eof == 0) 
    eof = get_next_filtered_trace(&tr);
  printf("Temps inactif total = %u\n",(unsigned) get_idle_time());
}

// Prints the total time taking account of the filter
void time()
{
  trace tr;
  int eof = 0;
  while (eof == 0) 
    eof = get_next_filtered_trace(&tr);
  printf("Temps total = %u\n",
	 (unsigned) get_idle_time() + (unsigned) get_active_time());
}

// Idem to nb_events
void nb_calls()
{
  nb_events();
}

// Prints the number of active slices taking account of the filter
void active_slices()
{
  trace tr;
  int eof = 0;
  while (eof == 0) 
    eof = get_next_filtered_trace(&tr);
  printf("Nombre de tranches actives = %u\n", (unsigned) get_active_slices());
}

// Prints the number of idle slices taking account of the filter
void idle_slices()
{
  trace tr;
  int eof = 0;
  while (eof == 0) 
    eof = get_next_filtered_trace(&tr);
  printf("Nombre de tranches inactives = %u\n", (unsigned) get_idle_slices);
}

// Prints the average time spent in an active slice
void avg_active_slice()
{
  trace tr;
  int eof = 0;
  while (eof == 0) 
    eof = get_next_filtered_trace(&tr);
  printf("Temps moyen d'une tranche active = %u\n",
	 (unsigned) (get_active_time() / get_active_slices()));
}

// Gives the time spent in each function
void function_time()
{
  unsigned code;
  mode type;
  int thread;
  char *name;
  u_64 begin, end, time; 
  trace tr;
  int eof = 0;
  while (eof == 0) 
    eof = get_next_filtered_trace(&tr);
  while(1) {
    if (get_function_time(&code, &type, &thread, &begin, &end, &time) == -1) 
      break;
    if (type == USER)
      name = fut_code2name(code);
    else name = fkt_code2name(code);
    printf("%s sur thread %d de %u à %u en %u\n", name, thread,
	   (unsigned) begin, (unsigned) end, (unsigned) time);
  }
}

// Error message or help
void error_usage()
{
  fprintf(stderr,"Usage: sigmund [options] [filters] action\n");
  fprintf(stderr,"Options:\n");
  fprintf(stderr,"   --trace-file <nom_supertrace>        Indique la supertrace à utiliser\n");
  fprintf(stderr,"                                          par défaut: prof_file\n");
  fprintf(stderr,"   --help                               Affiche ce message\n");
  fprintf(stderr,"Filters:\n");
  fprintf(stderr,"   --cpu <num_cpu>                      Restreint les traces considérées à celles concernant ce cpu\n");
  fprintf(stderr,"   --process <pid>                      Restreint les traces considérées à celles concernant ce processus\n");
  fprintf(stderr,"   --logic <num>                        Restreint les traces considérées à celles concernant ce numéro logique de lwp\n");
  fprintf(stderr,"   --thread <num>                       Restreint les traces considérées à celles concernant ce thread\n");
  fprintf(stderr,"   --time-slice <begin_tick> <end_tick> Restreint les traces considérées à celles concernant cette zone de temps\n");
  fprintf(stderr,"   --function <function_name>           Restreint les traces considérées à celles concernant cette fonction\n");
  fprintf(stderr,"   --event <event_name>                 Ne considére que cet événement là.\n");
  fprintf(stderr,"                                          Attention ce filtre est inutilisée lors des calculs de temps\n");
  fprintf(stderr,"   --event-slice <begin_num> <end_num>  Restreint les traces considérées à celles entre le <begin_num> ème événement\n");
  fprintf(stderr,"                                          et le <end_num> ème\n");
  fprintf(stderr,"   --sys-call                           Identique à --event mais pour les appels systèmes\n");
  fprintf(stderr,"   --begin <func_name[:param]> --end <func_name[:param]>\n");
  fprintf(stderr,"                                        Indique des zones à considérées comprises entre ces 2 événements avec la possibilité\n");
  fprintf(stderr,"                                           de préciser un argument (le 1er)\n");
  fprintf(stderr,"Actions:\n");
  fprintf(stderr,"   --list-events                        Affiche la liste des événements demandés\n");
  fprintf(stderr,"   --nb-events                          Affiche le nombre d'événements\n");
  fprintf(stderr,"   --nth-event <n>                      Affiche le <n> ème événement\n");
  fprintf(stderr,"   --active-time                        Temps total actif\n");
  fprintf(stderr,"   --idle-time                          Temps total inactif\n");
  fprintf(stderr,"   --time                               Temps total actif+inactif\n");
  fprintf(stderr,"   --active-slices                      Nombre de tranches actives\n");
  fprintf(stderr,"   --idle-slices                        Nombre de tranches inactives\n");
  fprintf(stderr,"   --avg-active-slices                  Temps moyen d'une tranche active\n");
  fprintf(stderr,"   --nb-calls                           Identique à --nb-events\n");
  fprintf(stderr,"   --function-time                      Temps pour chaque appel de foncion\n");
  exit(1);
}

/* THE main */
int main(int argc, char **argv)
{
  int argCount;
  char *trace_file_name = NULL;
  enum action ac;                                       // Action asked
  int nth = 0;                                          // If nth-event
  ac = NONE;                                            // No action precised yet
  // Initialise fut names
  init();
  // Initialise the filter to none
  init_filter();
  // Parse the arguments
  for(argc--, argv++; argc > 0; argc -= argCount, argv +=argCount) {
    argCount = 1;
    if (!strcmp(*argv, "--help")) {
      error_usage();
    } else if (!strcmp(*argv, "--trace-file")) {
      if (argc <= 1) error_usage();
      trace_file_name = *(argv + 1);
      argCount = 2;
    } else if (!strcmp(*argv, "--thread")) {
      if (argc <= 1) error_usage();
      filter_add_thread(atoi(*(argv + 1)));
      argCount = 2;
    } else if (!strcmp(*argv, "--process")) {
      if (argc <= 1) error_usage();
      filter_add_proc(atoi(*(argv + 1)));
      argCount = 2;
    } else if (!strcmp(*argv, "--logic")) {
      if (argc <= 1) error_usage();
      filter_add_logic(atoi(*(argv + 1)));
      argCount = 2;
    } else if (!strcmp(*argv, "--cpu")) {
      if (argc <= 1) error_usage();
      filter_add_cpu(atoi(*(argv + 1)));
      argCount = 2;
    } else if (!strcmp(*argv, "--time-slice")) {
      if (argc <= 2) error_usage();
      filter_add_time_slice(atoi(*(argv + 1)), atoi(*(argv + 2)));
      argCount = 3;
    } else if (!strcmp(*argv, "--function")) {
      int begin;
      mode begin_type;
      int end;
      mode end_type;
      char s[400];
      if (argc <= 1) error_usage();
      strcpy(s,*(argv+1));
      strcat(s,"_entry");
      if (name2code(s, &begin_type, &begin) != 0) error_usage();
      strcpy(s,*(argv+1));
      strcat(s,"_exit");
      if (name2code(s, &end_type, &end) != 0) error_usage();
      filter_add_function(begin_type, begin, FALSE, 0, end_type, end, FALSE, 0);
      argCount = 2;
    } else if (!strcmp(*argv, "--event")) {
      int a;
      mode type;
      if (argc <= 1) error_usage();
      if (name2code(*(argv+1), &type, &a) != 0) error_usage();
      filter_add_event(type, a);
      argCount = 2;
    } else if (!strcmp(*argv, "--sys-call")) {
      int a;
      if (argc <= 1) error_usage();
      if (sys2code(*(argv+1), &a) != 0) error_usage();
      filter_add_event(KERNEL, a);
      argCount = 2;
    } else if (!strcmp(*argv, "--event-slice")) {
      if (argc <= 2) error_usage();
      filter_add_evnum_slice(atoi(*(argv + 1)), atoi(*(argv + 2)));
      argCount = 3;
    } else if (!strcmp(*argv, "--begin")) {
      int begin_param_active = 0;
      int end_param_active = 0;
      mode begin_type = 0;
      mode end_type = 0;
      int begin_param = 0;
      int end_param = 0;
      int begin = 0;
      int end = 0;
      if (argc <= 3) error_usage();
      if (strcmp(*(argv+2),"--end")) error_usage();
      if (strchr(*(argv+1),':') == NULL) {
	begin_param_active = FALSE;
	if (name2code(*(argv+1), &begin_type, &begin) != 0) error_usage();
      }
      else {
	char *s;
	begin_param_active = TRUE;
	s = strtok(*(argv+1),":");
	if (name2code(s, &begin_type, &begin) != 0) error_usage();
	s = strtok(NULL,":");
	begin_param = atoi(s);
      }
      if (strchr(*(argv+3),':') == NULL) {
	end_param_active = FALSE;
	if (name2code(*(argv+3), &end_type, &end) != 0) error_usage();
      }
      else {
	char *s;
	end_param_active = TRUE;
	s = strtok(*(argv+3),":");
	if (name2code(s, &end_type, &end) != 0) error_usage();
	s = strtok(NULL, ":");
	end_param = atoi(s);
      }
      filter_add_gen_slice(begin_type, begin, begin_param_active, begin_param,
			   end_type, end, end_param_active, end_param);
      argCount = 4;
    } else if (!strcmp(*argv, "--list-events")) {
      if (ac != NONE) error_usage();
      ac = LIST_EVENTS;
    } else if (!strcmp(*argv, "--nb-events")) {
      if (ac != NONE) error_usage();
      ac = NB_EVENTS;
    } else if (!strcmp(*argv, "--nth-event")) {
      if (argc <= 1) error_usage();
      if (ac != NONE) error_usage();
      ac = NTH_EVENT;
      nth = atoi(*(argv+1));
      argCount = 2;
    } else if (!strcmp(*argv, "--active-time")) {
      if (ac != NONE) error_usage();
      ac = ACTIVE_TIME;
    } else if (!strcmp(*argv, "--idle-time")) {
      if (ac != NONE) error_usage();
      ac = IDLE_TIME;
    } else if (!strcmp(*argv, "--time")) {
      if (ac != NONE) error_usage();
      ac = TIME;
    } else if (!strcmp(*argv, "--nb-calls")) {
      if (ac != NONE) error_usage();
      ac = NB_CALLS;
    } else if (!strcmp(*argv, "--active-slices")) {
      if (ac != NONE) error_usage();
      ac = ACTIVE_SLICES;
    } else if (!strcmp(*argv, "--idle-slices")) {
      if (ac != NONE) error_usage();
      ac = IDLE_SLICES;
    } else if (!strcmp(*argv, "--avg-active-slice")) {
      if (ac != NONE) error_usage();
      ac = AVG_ACTIVE_SLICE;
    } else if (!strcmp(*argv, "--function-time")) {
      if (ac != NONE) error_usage();
      ac = FUNCTION_TIME;
    } else error_usage();
  }
  // Initialise the supertrace library with this supertrace
  tracelib_init(trace_file_name);
  // Switch action
  switch(ac) {
  case NONE : {
    list_events();          // No action -> list_events
    break;
  }
  case LIST_EVENTS : {
    list_events();
    break;
  }
  case NB_EVENTS : {
    nb_events();
    break;
  }
  case NTH_EVENT : {
    nth_event(nth);
    break;
  }
  case ACTIVE_TIME : {
    active_time();
    break;
  }
  case IDLE_TIME : {
    idle_time();
    break;
  }
  case TIME : {
    time();
    break;
  }
  case NB_CALLS : {
    nb_calls();
    break;
  }
  case ACTIVE_SLICES : {
    active_slices();
    break;
  }
  case IDLE_SLICES : {
    idle_slices();
    break;
  }
  case AVG_ACTIVE_SLICE : {
    avg_active_slice();
    break;
  }
  case FUNCTION_TIME : {
    function_time();
    break;
  }
  default: {
    fprintf(stderr,"Please report bug to cmenier@ens-lyon.fr\n");
    exit(1);
  }
  }
  return 0;
}
