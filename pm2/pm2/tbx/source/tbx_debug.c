
/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 "the PM2 team" (pm2-dev@listes.ens-lyon.fr)
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

#ifndef PM2DEBUG
#define PM2DEBUG
#endif

#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include "tbx_debug.h"
#include "pm2_testandset.h"
#if defined(MARCEL_SMP) || defined(MARCEL_ACTIVATION)
#include <sched.h>
#endif

#ifdef MARCEL
#include "marcel.h"
#endif

#define MAX_DEBUG_REGISTER 50
#define MAX_DEBUG_CMDLINE_OPTIONS 50
/* Timeout before aborting taking the lock */
#define TIMEOUT  10000000
/* Buffer for the activations ... */
#define DEBUG_SIZE_BUFFER 1600000
#define SIZE_FILE_NAME 25

#define STRING(s) str(s)
#define str(s) #s

static int nb_debug_type_registered=0;
static debug_type_t* debug_type_registered[MAX_DEBUG_REGISTER];
static int nb_options=0;
static char* options[MAX_DEBUG_CMDLINE_OPTIONS];

debug_type_t debug_pm2debug=NEW_DEBUG_TYPE(1, "pm2debug: ", "pm2debug");
debug_type_t debug_pm2fulldebug=NEW_DEBUG_TYPE(1, "pm2fulldebug: ", "pm2fulldebug");
debug_type_t debug_type_register=NEW_DEBUG_TYPE(0, "register: ", "register");
static debug_type_t debug_type_flush=NEW_DEBUG_TYPE(0, "unused", "unused");
DEBUG_DECLARE(common)
#ifdef MARCEL
DEBUG_DECLARE(marcel)
#endif
#ifdef MAD1
DEBUG_DECLARE(mad1)
#endif
#ifdef MAD2
DEBUG_DECLARE(mad2)
#endif
#ifdef MAD3
DEBUG_DECLARE(mad3)
#endif
#ifdef LEONIE
DEBUG_DECLARE(leonie)
#endif
#ifdef LEOPARSE
DEBUG_DECLARE(leoparse)
#endif
#ifdef SWANN
DEBUG_DECLARE(swann)
#endif
#ifdef PM2
DEBUG_DECLARE(pm2)
#endif
#ifdef TBX
DEBUG_DECLARE(tbx)
#endif
#ifdef NTBX
DEBUG_DECLARE(ntbx)
#endif
#ifdef DSM
DEBUG_DECLARE(dsm)
#endif
DEBUG_DECLARE(app)

inline void pm2debug_setup(debug_type_t* type, debug_action_t action, 
			   int value)
{
	if (type->setup) {
		type->setup(type, action, value);
	} else {
		debug_setup_default(type, action, value);
	}
}


void debug_setup_default(debug_type_t* type, debug_action_t action, int value)
{
	if (!type) {
		return;
	}
	switch (action) {
	case PM2DEBUG_SHOW:
		type->show=value;
		break;
	case PM2DEBUG_SHOW_PREFIX:
		type->show_prefix=value;
		break;
	case PM2DEBUG_SHOW_FILE:
		type->show_file=value;
		break;
	case PM2DEBUG_CRITICAL:
		type->critical=value;
		break;
	case PM2DEBUG_DO_NOT_SHOW_THREAD:
		type->do_not_show_thread=value;
		break;
	case PM2DEBUG_DO_NOT_SHOW_LWP:
		type->do_not_show_lwp=value;
		break;
	case PM2DEBUG_TRYONLY:
		type->try_only=value;
		break;
	case PM2DEBUG_REGISTER:
		break;
	}
}

static void print_register(debug_type_t* type)
{
	debug_printf(&debug_type_register, "register debug name: %s (%s)\n", 
		     type->option_name, type->show ? "enabled" : "disabled" );
}

static void register_setup(debug_type_t* type, debug_action_t action, 
			   int value)
{
	int type_ind;
	if (!type) {
		return;
	}
	debug_setup_default(type, action, value);
	switch (action) {
	case PM2DEBUG_SHOW:
		for(type_ind=0; type_ind<nb_debug_type_registered; 
		    type_ind++) {
			print_register(debug_type_registered[type_ind]);
		}
		break;
	default:
		break;
	}
}

static int apply_option(debug_type_t *type, char* option)
{
	size_t size=strlen(type->option_name);
	const char *chaine;
	int addlen;
	debug_action_t action;
	int value;

	option+=7; /* on saute --debug */
	if (option[0]==':') {
		option++;
	} else if (option[0]) {
		return 0;
	}

	if (!*option) {
		chaine="default";
		addlen=0;
	} else {
		addlen=1;
		chaine=option;
	}
	if (strncmp(chaine, type->option_name, size))
		return 0;
	if (addlen) {
		option+=size;
	}
	if (option[0]==':') {
		option++;
	} else if (option[0]) {
		return 0;
	}
	
	if (!*option) {
		chaine="show";
		addlen=0;
	} else {
		addlen=1;
		chaine=option;
	}
	if (!strncmp(chaine, "show", 4)) {
		action=PM2DEBUG_SHOW;
		size=4;
	} else if (!strncmp(chaine, "prefix", 6)) {
		action=PM2DEBUG_SHOW_PREFIX;
		size=6;
	} else if (!strncmp(chaine, "file", 4)) {
		action=PM2DEBUG_SHOW_FILE;
		size=4;
	} else if (!strncmp(chaine, "critical", 8)) {
		action=PM2DEBUG_CRITICAL;
		size=8;
	} else if (!strncmp(chaine, "lwp", 3)) {
		action=PM2DEBUG_DO_NOT_SHOW_LWP;
		size=3;
	} else if (!strncmp(chaine, "thread", 6)) {
		action=PM2DEBUG_DO_NOT_SHOW_THREAD;
		size=3;
	} else if (!strncmp(chaine, "tryonly", 7)) {
		action=PM2DEBUG_TRYONLY;
		size=7;
	} else {
		return 0;
	}
	if (addlen) {
		option+=size;
	}
	if (option[0]==':') {
		option++;
	} else if (option[0]) {
		return 0;
	}
	
	if (!*option) {
		chaine="on";
		addlen=0;
	} else {
		addlen=1;
		chaine=option;
	}
	if (!strncmp(chaine, "on", 2)) {
		value=1;
		size=2;
	} else if (!strncmp(chaine, "off", 3)) {
		value=0;
		size=3;
	} else {
		return 0;
	}
	if (addlen) {
		option+=size;
	}
	if (option[0]==':') {
		option++;
	} else if (option[0]) {
		return 0;
	}
	pm2debug_setup(type, action, value);
	return 1;
}

static void remove_option(int opt)
{
	int i;
	for (i=opt+1; i<nb_options; i++) {
		options[i-1]=options[i];
	}
}

void pm2debug_register(debug_type_t *type)
{
	int opt;
	int type_ind;
	for (type_ind=0; type_ind<nb_debug_type_registered; type_ind++) {
		if (debug_type_registered[type_ind] ==  type) {
			return ; /* Already registered */
		}
	}
	if (nb_debug_type_registered<MAX_DEBUG_REGISTER-1) {
		debug_type_registered[nb_debug_type_registered++]=type;
	} else {
		pm2fulldebug("Too many debug types registered."
			     " Please, increase\n"
			     "MAX_DEBUG_REGISTER in " __FILE__ "\n");
	}
	pm2debug_setup(type, PM2DEBUG_REGISTER, 0);
	print_register(type);

	for (opt=0; opt<nb_options; opt++) {
		if (apply_option(type, options[opt])) {
			remove_option(opt);
		}
	}

}

#ifdef MA__ACTIVATION
// Please, do not change order! This allows:
// sed -e '/^PM2DEBUG BUFFER 0$/,/^PM2DEBUG BUFFER 2$/p;d' < core
// ;)
static char pm2debug_dummy0[50];
static char base_buffer0[DEBUG_SIZE_BUFFER];
static char pm2debug_dummy1[50];
static char base_buffer1[DEBUG_SIZE_BUFFER];
static char pm2debug_dummy2[50];
#endif

void pm2debug_init_ext(int *argc, char **argv, int debug_flags)
{
	static int called=0;
	static int arg=0;
	int type;
	int opt=1;
	int i;

	if (!called) {
#ifdef MA__ACTIVATION
	  snprintf(pm2debug_dummy0, 49, "\nPM2DEBUG BUFFER 0\n");
	  snprintf(pm2debug_dummy1, 49, "\nPM2DEBUG BUFFER 1\n");
	  snprintf(pm2debug_dummy2, 49, "\nPM2DEBUG BUFFER 2\n");
#endif
	  pm2debug_register(&debug_pm2debug);
	  pm2debug_register(&debug_pm2fulldebug);
	  pm2debug_setup(&debug_pm2fulldebug, PM2DEBUG_SHOW_FILE, 1);
	  pm2debug_setup(&debug_pm2fulldebug, PM2DEBUG_CRITICAL, 1);
	  debug_type_register.setup=register_setup;
	  pm2debug_register(&debug_type_register);
	  DEBUG_INIT(common);
#ifdef MARCEL
	  DEBUG_INIT(marcel);
#endif
#ifdef MAD1
	  DEBUG_INIT(mad1);
#endif
#ifdef MAD2
	  DEBUG_INIT(mad2);
#endif
#ifdef MAD3
	  DEBUG_INIT(mad3);
#endif
#ifdef LEONIE
	  DEBUG_INIT(leonie);
#endif
#ifdef LEOPARSE
	  DEBUG_INIT(leoparse);
#endif
#ifdef SWANN
	  DEBUG_INIT(swann);
#endif
#ifdef PM2
	  DEBUG_INIT(pm2);
#endif
#ifdef TBX
	  DEBUG_INIT(tbx);
#endif
#ifdef NTBX
	  DEBUG_INIT(ntbx);
#endif
#ifdef DSM
	  DEBUG_INIT(dsm);
#endif
	  DEBUG_INIT(app);
	}
	called=1;

	if ((argc==NULL) || (argv==NULL)) {
	  return;
	}
	if (arg) {
	  debug_flags &= (~PM2DEBUG_DO_OPT);
	}

	if (debug_flags & PM2DEBUG_DO_OPT) {
	  arg=1;
	}

	while ((opt<=(*argc)) && (argv[opt])) {
	  
#ifdef LEONIE
	  // If the TBX lib is included as part of Leonie 
	  // only the args up to the configuration file name should
	  // be extracted.
	  // Args appeating after the cfg file name on the command line
	  // are dedicated to the application
	  if ((argv[opt])[0] != '-')
	    {
	      break;
	    }

	  if (!strcmp(argv[opt], "--"))
	    {
	      break;
	    }
#endif // LEONIE

		if (strncmp(argv[opt], "--debug", 7)) {
			opt++;
			continue;
		}

		if (debug_flags & PM2DEBUG_DO_OPT) {
		  for (type=0; type<nb_debug_type_registered; type++) {
		    if (apply_option(debug_type_registered[type], 
				     argv[opt])) {
		      goto option_done;
		    }
		  }
		  options[nb_options++]=argv[opt];
		}
	option_done:
		if (debug_flags & PM2DEBUG_CLEAROPT) {
		  for (i=opt; i<(*argc); i++) {
		    argv[i]=argv[i+1];
		  }
		  argv[(*argc)--]=NULL;
		} else {
		  opt++;
		}
	}
}
	
typedef struct {
const char* file_lock;
int file_line;
int file_flush;
} debug_lock_t;

debug_lock_t dl={NULL,0,0};
static volatile int lock_debug=0;

#ifdef MARCEL
int pm2debug_marcel_launched=0;
#endif

#ifdef ACTIVATION


static char* base_buffer[]={base_buffer0, base_buffer1};

static int cur_buffer=0;
static volatile int buf_used[2]={0,0};

static char* pos_buffer=base_buffer0;
volatile int debug_buffer_lenght=0;

static volatile int in_flush=0;

static volatile int debug_overflow[2]={0,0};

void real_flush(int profondeur_lock)
{
	int nb_write;
	int pos=0;
	int num_buf=cur_buffer;
	
	if (debug_buffer_lenght && !buf_used[1-num_buf]) {
		cur_buffer=1-num_buf;
		buf_used[num_buf]=debug_buffer_lenght;
		debug_buffer_lenght=0;
		pos_buffer=base_buffer[cur_buffer];
		//printf("Write buf %i\n", num_buf);
	}

	in_flush=0;

	dl.file_flush=0;

	lock_debug=0;
	while (profondeur_lock--)
		unlock_task_for_debug();
	if (debug_overflow[num_buf]) {
		write(2, "\n\n\nOVERFLOW\n\n\n", 
		      strlen("\n\n\nOVERFLOW\n\n\n"));
	}
	while (buf_used[num_buf]) {
		//write(2, "£", 1);
		nb_write=write(2, base_buffer[num_buf]+pos, buf_used[num_buf]);
		//write(2, "$", 1);
		if (nb_write<0) {
			perror("Debug error in write");
			exit(1);
		}
		buf_used[num_buf] -= nb_write;
		pos += nb_write;
	}
}

#endif

void pm2debug_flush()
{
	pm2debug_printf(&debug_type_flush,0,"","");
}

#ifdef ACTIVATION
#define my_print(prefix, args...) \
	if (pm2debug_marcel_launched) { \
		new_bytes=prefix##snprintf(pos_buffer, \
				   DEBUG_SIZE_BUFFER-10-debug_buffer_lenght, \
				   ##args); \
			pos_buffer+=new_bytes; \
		debug_buffer_lenght+=new_bytes; \
	} else { \
		prefix##fprintf(stderr, ##args); \
	}
#else
#define my_print(prefix, args...) prefix##fprintf(stderr, ##args)
#endif


	


int pm2debug_printf(debug_type_t *type, int line, const char* file, 
		    const char *format, ...)
{
#ifdef ACTIVATION
	int new_bytes=0;
#endif
	volatile int timeout=TIMEOUT;
	va_list ap;

	if (!type) { 
		return 0;
	}

#ifdef MARCEL
	if (pm2debug_marcel_launched) {
		ma_lock_task(); /* Pas de debug ici ! */
	}
#endif
	if (pm2_spinlock_testandset(&lock_debug)) {
		if (type->try_only) {
#ifdef MARCEL
			unlock_task_for_debug(); /* Pas de debug ici ! */
#endif			
			return 0;
		}
		
		while (timeout--) {
			if (!pm2_spinlock_testandset(&lock_debug)) {
				goto have_lock;
			}
#if defined(MARCEL_SMP) || defined(MARCEL_ACTIVATION)
			sched_yield();
#endif
		}
#ifdef MARCEL
		if (pm2debug_marcel_launched) {
			unlock_task_for_debug(); /* Pas de debug ici ! */
		}
#endif			
		return 0;
	}
have_lock:

	dl.file_lock=file;
	dl.file_line=line;

	if (type->show) {
		if (type->show_prefix) {
			my_print(,"%s", type->prefix);
		}
		if (type->show_file) {
			my_print(,"%-" STRING(SIZE_FILE_NAME)
				 "s{%4i} ", file, line);
		}
#ifdef MARCEL
#ifdef MA__LWPS
		if (pm2debug_marcel_launched && !type->do_not_show_lwp) {
			my_print(,"[P%02d] ", 
				 ((pm2debug_marcel_launched 
				   && (marcel_self())->lwp) 
				  ? (marcel_self())->lwp->number : -1));
		}
#endif
		if (pm2debug_marcel_launched && !type->do_not_show_thread) {
			my_print(,"(%8p) ", pm2debug_marcel_launched ?
			marcel_self():(void*)-1);
		}
#endif /* MARCEL */
		va_start(ap, format);
		my_print(v, format, ap);
		va_end(ap);
	}
#ifdef ACTIVATION
	if (pm2debug_marcel_launched) {
		if (!pm2_spinlock_testandset(&in_flush)) {
			if ((locked()==1) // appel sans lock_task
			    && (MA_GET_TASK_TYPE(marcel_self())==
				MA_TASK_TYPE_NORMAL)) { // tache normale
				dl.file_lock=NULL;
				dl.file_line=0;
				dl.file_flush=1;
				real_flush(1);
				return 0;
			} else {
				in_flush=0;
			}
		}
	}
#endif
	dl.file_lock=NULL;
	dl.file_line=0;
	lock_debug=0;
#ifdef MARCEL	
	if (pm2debug_marcel_launched) {
		unlock_task_for_debug();
	}
#endif
	return 0;
}




