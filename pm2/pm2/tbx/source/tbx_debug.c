/*! \file tbx_debug.c
 *  \brief TBX debug support routines
 *
 *  This file contains the support functions for the
 *  TBX debugging facilities.
 * 
 */

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

/* On a besoin de partie de marcel non exportées */
#define MARCEL_INTERNAL_INCLUDE

#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include "tbx_debug.h"

#ifdef MARCEL
#include "marcel.h"
#endif

#define MAX_DEBUG_REGISTER 50
#define MAX_DEBUG_CMDLINE_OPTIONS 50
/* Timeout before aborting taking the lock */
#define TIMEOUT  10000000
/* Buffer for the activations ... */
#define DEBUG_SIZE_BUFFER 1600000
#define SIZE_FILE_NAME 35

#define STRING(s) tbx_str(s)
#define tbx_str(s) #s

static int nb_debug_type_registered=0;
static debug_type_t* debug_type_registered[MAX_DEBUG_REGISTER];
static int nb_options=0;
static char* options[MAX_DEBUG_CMDLINE_OPTIONS];

debug_type_t debug_pm2debug=NEW_DEBUG_TYPE(1, "pm2debug: ", "pm2debug");
debug_type_t debug_pm2fulldebug=NEW_DEBUG_TYPE(1, "pm2fulldebug: ", "pm2fulldebug");
debug_type_t debug_type_register=NEW_DEBUG_TYPE(0, "register: ", "register");
static debug_type_t debug_type_flush=NEW_DEBUG_TYPE(0, "unused", "unused");
DEBUG_DECLARE(init)
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
		pm2debug("Warning: unknown action %s\n", chaine);
		return 1;
	}
	if (addlen) {
		option+=size;
	}
	if (option[0]==':') {
		option++;
	} else if (option[0]) {
		pm2debug("Warning: unknown action %s\n", chaine);
		return 1;
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
		pm2debug("Warning: unknown value %s\n", chaine);
		return 1;
	}
	if (addlen) {
		option+=size;
	}
	if (option[0]==':') {
		option++;
	} else if (option[0]) {
		pm2debug("Warning: unknown value %s\n", chaine);
		return 1;
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
	nb_options--;
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

void pm2debug_init_ext(int *argc, char **argv, int debug_flags)
{
	static int called=0;
	static int arg=0;
	int type;
	int opt=1;
	int i;

	if (!called) {
		pm2debug_register(&debug_pm2debug);
		pm2debug_register(&debug_pm2fulldebug);
		pm2debug_setup(&debug_pm2fulldebug, PM2DEBUG_SHOW_FILE, 1);
		pm2debug_setup(&debug_pm2fulldebug, PM2DEBUG_CRITICAL, 1);
		debug_type_register.setup=register_setup;
		pm2debug_register(&debug_type_register);
		DEBUG_INIT(init);
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
			//pm2debug("registering option %s\n",
			//	 options[nb_options-1]);
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
	
void pm2debug_flush()
{
	pm2debug_printf(&debug_type_flush,0,__FILE__, "%s", "");
}

#if !defined(MARCEL) || !defined(MA__PTHREAD_FUNCTIONS)
int __pm2debug_printf_state=PM2DEBUG_PRINTF_UNPROTECT_ALLOWED;
#else
int __pm2debug_printf_state=PM2DEBUG_PRINTF_DENY;
#endif

void pm2debug_printf_state(int state)
{
	__pm2debug_printf_state=state;
}

volatile int inprint=0;

inline static int pm2debug_tryprint()
{
#ifndef MARCEL
	return 1;
#else
	switch (__pm2debug_printf_state) {
	case PM2DEBUG_PRINTF_UNPROTECT_ALLOWED:
		if (inprint) {
			return 0;
		}
		inprint=1;
		return 1;
	case PM2DEBUG_PRINTF_DENY:
		return 0;
	case PM2DEBUG_PRINTF_ALLOWED:
		ma_local_bh_disable();
		if(tbx_unlikely(ma_test_and_set_thread_flag(TIF_DEBUG_IN_PROGRESS))) {
			/* Déjà en cours de pm2debug */
			ma_local_bh_enable();
			return 0;
		}
		return 2;
	}
	RAISE(PROGRAM_ERROR);
	return 0;
#endif
}

inline static void pm2debug_endprint(int entry)
{
#ifdef MARCEL
	switch(entry) {
	case 1:
		inprint=0;
		break;
	case 2:
		ma_smp_mb__before_clear_bit();
		ma_clear_thread_flag(TIF_DEBUG_IN_PROGRESS);
		ma_local_bh_enable();
		break;
	case 0:
		break;
	}
#endif
}

#define my_print(fmt, args...) \
	do { \
		size=tbx_snprintf(cur, remaining, fmt, ##args); \
		remaining-=size; \
		if (remaining<=0) \
			goto do_write; \
		eaten+=size; \
		cur+=size; \
	} while (0);

int pm2debug_printf(debug_type_t *type, int line, const char* file, 
		    const char *format, ...)
{
	va_list ap;
	int can_print;

	if (!type) { 
		return 0;
	}
	
	if(0 == (can_print=pm2debug_tryprint())) {
		/* un pm2debug dans lui même ? */
		return 0;
	}

	if (type->show) {
		char buffer[PM2DEBUG_MAXLINELEN],*cur=buffer;
		int eaten=0,remaining=PM2DEBUG_MAXLINELEN,size;
		if (type->show_prefix) {
			my_print("%s", type->prefix);
		}
		if (type->show_file) {
			int len=strlen(file)+15;
			char location[len];
			if (line > 10000000) {
				line=0;
			}
			if (tbx_snprintf(location, len, "%s:%i", file, line)>=len) {
				strcpy(location,"BUG");
			}
			my_print("%-" STRING(SIZE_FILE_NAME)
				 "s ", location);
		}
#ifdef MARCEL
#ifdef MA__LWPS
		if (!type->do_not_show_lwp) {
			my_print("[P%02d] ", 
				 (((can_print==2) 
				   && GET_LWP(marcel_self())) 
				  ? GET_LWP_NUMBER(marcel_self()) : -1));
		}
#endif
		if (!type->do_not_show_thread) {
			my_print("(%8p) ", (can_print==2) ?
			marcel_self():(void*)-1);
		}
#endif /* MARCEL */
		va_start(ap, format);
		eaten+=tbx_vsnprintf(cur, remaining, format, ap);
		va_end(ap);
do_write:
		if (eaten>PM2DEBUG_MAXLINELEN)
			eaten=PM2DEBUG_MAXLINELEN;
		write(STDERR_FILENO,buffer,eaten);
	}

	pm2debug_endprint(can_print);
	return 0;
}




