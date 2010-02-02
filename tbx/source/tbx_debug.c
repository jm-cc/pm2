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

/* On a besoin de partie de marcel non exportées */
#define MARCEL_INTERNAL_INCLUDE

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include "tbx.h"

#ifdef MARCEL
#include "marcel.h"
#endif

#define MAX_DEBUG_REGISTER 100
#define MAX_DEBUG_CMDLINE_OPTIONS 50
/* Timeout before aborting taking the lock */
#define TIMEOUT  10000000
/* Buffer for the activations ... */
#define SIZE_FILE_NAME 35

/* Dans l'ordre de debug_action_t */
static const char*options_name[] = {
	"show",
	"prefix",
	"file",
	"thread",
	"lwp",
	NULL,
};

#define STRING(s) tbx_str(s)
#define tbx_str(s) #s

static int nb_debug_type_registered=0;
static debug_type_t* debug_type_registered[MAX_DEBUG_REGISTER];
static int nb_options=0;
static char* options[MAX_DEBUG_CMDLINE_OPTIONS];

debug_type_t debug_pm2debug=NEW_DEBUG_TYPE("pm2debug: ", "pm2debug");
debug_type_t debug_type_register=NEW_DEBUG_TYPE("register: ", "register");

debug_type_t debug_default={
	.show=0,
	.actions={PM2DEBUG_DEFAULTLEVEL,0,0,1,1},
	.prefix="default: ",
	.option_name="default",
	.depend=NULL,
	.register_shown=0,
};

debug_type_t debug_disp = NEW_DEBUG_TYPE("disp: ", "disp");
debug_type_t debug_log = NEW_DEBUG_TYPE("log: ", "log");
debug_type_t debug_trace = NEW_DEBUG_TYPE("trace: ", "trace");
debug_type_t debug_warn = NEW_DEBUG_TYPE("warn: ", "warn");

DEBUG_DECLARE(init)
#ifdef MARCEL
DEBUG_DECLARE(marcel)
#endif
#ifdef PIOMAN
DEBUG_DECLARE(pioman)
#endif
#ifdef MAD3
DEBUG_DECLARE(mad3)
#endif
#ifdef NMAD
DEBUG_DECLARE(nmad)
#endif
#ifdef LEONIE
DEBUG_DECLARE(leonie)
#endif
#ifdef LEOPARSE
DEBUG_DECLARE(leoparse)
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
#ifdef PROFILE
DEBUG_DECLARE(profile)
#endif
DEBUG_DECLARE(app)

inline static int get_action_value(debug_type_t* type, debug_action_t action)
{
	int local;

	if (!type) {
		type=&debug_default;
	}
	local=type->actions[action];
	if (local == PM2DEBUG_DEFAULT) {		
		if (type==&debug_default) {
			pm2debug("Default action for %i is default!\n",
				 action);
			return 0;
		}
		return get_action_value(type->depend, action);
	}
	return local;
}

inline static void update_show(void)
{
	int type_ind;

	for(type_ind=0; type_ind<nb_debug_type_registered; type_ind++) {
		debug_type_t *type=debug_type_registered[type_ind];
		if (type->show != PM2DEBUG_AUTO_REGISTER) {
			type->show=get_action_value(type, PM2DEBUG_SHOW);
		}
	}
}

static void print_register(debug_type_t* type)
{
	int reg_level=get_action_value(&debug_type_register, PM2DEBUG_SHOW);

	if ((reg_level > PM2DEBUG_STDLEVEL && !type->register_shown)
	    ||(reg_level > PM2DEBUG_ON && type->register_shown<2)) {
		/* Si on doit afficher quelque chose */
		if (type->show != PM2DEBUG_AUTO_REGISTER) {
			const char *s1="", *s2="";
			type->register_shown=2;
			if (type->actions[PM2DEBUG_SHOW] == PM2DEBUG_DEFAULT) {
				s1="DEFAULT (";
				s2=")";
			}
			debug_printf(&debug_type_register, 
				     "register debug name: %s [%s] (show=%s%i%s)\n", 
				     type->option_name, 
				     (type->depend?:&debug_default)->option_name,
				     s1, get_action_value(type, PM2DEBUG_SHOW),
				     s2);
		} else if (reg_level>PM2DEBUG_ON 
			   && type->show == PM2DEBUG_AUTO_REGISTER) {
			type->register_shown=1;
			debug_printf(&debug_type_register, 
				     "register debug name: %s [%s] not yet used\n",
				     type->option_name,
				     (type->depend?:&debug_default)->option_name);
		}
	}
}

inline static void print_all_registered(void)
{
	int type_ind;

	for(type_ind=0; type_ind<nb_debug_type_registered; 
	    type_ind++) {
		print_register(debug_type_registered[type_ind]);
	}
}

inline static void show_registered(void)
{
	static int shown=0;

	if (!shown) {
		//shown=1;
		print_all_registered();
	}
}

static int apply_option(debug_type_t *type, char* option)
{
	size_t size=strlen(type->option_name);
	const char *chaine;
	int addlen;
	debug_action_t action;
	int value;
	const char **opt;

	if (strncmp(option, type->option_name, size))
		return 0;
	option+=size;

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

	for(opt=options_name, action=PM2DEBUG_SHOW;
	    *opt != NULL && action<PM2DEBUG_SHOW_MAX;
	    opt++, action++) {
		size=strlen(*opt);
		if (!strncmp(chaine, *opt, size)) {
			goto option_found;
		}
	}
	pm2debug("Warning: unknown action %s\n", chaine);
	return 1;
 option_found:
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
	if (!strcmp(chaine, "on")) {
		value=PM2DEBUG_ON;
	} else if (!strcmp(chaine, "off")) {
		value=PM2DEBUG_OFF;
	} else if (!strcmp(chaine, "default")) {
		value=PM2DEBUG_DEFAULT;
	} else {
		char *end;
		value=strtol(chaine, &end, 0);
		if (*end) {
			pm2debug("Warning: unknown value %s\n", chaine);
			return 1;
		}
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

#define REG_AUTO    1
#define REG_DEPEND  2
static void pm2debug_register_real(debug_type_t *type, int option)
{
	int opt;
	int type_ind;
	int need_print=0;
	if (type->depend) {
		pm2debug_register_real(type->depend, option|REG_DEPEND);
	}
	if (option & REG_AUTO) {
		if (type->show==PM2DEBUG_AUTO_REGISTER) {
			need_print=1;
			type->show=get_action_value(type, PM2DEBUG_SHOW);
		}
	}
	for (type_ind=0; type_ind<nb_debug_type_registered; type_ind++) {
		if (debug_type_registered[type_ind] ==  type) {
			/* Already registered */
			if (need_print) {
				print_register(type);
			}
			return ;
		}
	}
	if (nb_debug_type_registered<MAX_DEBUG_REGISTER-1) {
		debug_type_registered[nb_debug_type_registered++]=type;
	} else {
		pm2debug("Too many debug types registered."
			 " Please, increase\n"
			 "MAX_DEBUG_REGISTER in " __FILE__ "\n");
	}
	pm2debug_register_real(&debug_default, option|REG_DEPEND);

	for (opt=0; opt<nb_options; opt++) {
		if (apply_option(type, options[opt])) {
			remove_option(opt);
		}
	}
	print_register(type);
}

static
void pm2debug_init_base(void)
{
	static int called=0;

	if (called)
		return;

	pm2debug_register(&debug_default);
	pm2debug_register(&debug_pm2debug);
	pm2debug_register(&debug_type_register);
	DEBUG_INIT(init);
#ifdef MARCEL
	DEBUG_INIT(marcel);
#endif
#ifdef MAD3
	DEBUG_INIT(mad3);
#endif
#ifdef NMAD
	DEBUG_INIT(nmad);
#endif
#ifdef LEONIE
	DEBUG_INIT(leonie);
#endif
#ifdef LEOPARSE
	DEBUG_INIT(leoparse);
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
	DEBUG_INIT(app);

}

#undef pm2debug_register
void pm2debug_register(debug_type_t *type)
{
	pm2debug_register_real(type, 0);
}

inline static void pm2debug_register_auto(debug_type_t *type)
{
	pm2debug_register_real(type, REG_AUTO);
}

#undef pm2debug_setup
void pm2debug_setup(debug_type_t* type, debug_action_t action, 
		    int value)
{
	if (!type) {
		pm2debug("Arghhh: NULL debug type for pm2debug_setup\n");
		return;
	}
	pm2debug_register_real(type, REG_DEPEND);
	type->actions[action]=value;
	if (action==PM2DEBUG_SHOW) {
		update_show();
	}
}

#undef pm2debug_init_ext
void pm2debug_init_ext(int *argc, char **argv, int debug_flags)
{
	static int op_done=0;
	int type;
	int opt=1;
	int i;
	int show_reg=0;

	pm2debug_init_base();

	if ((argc==NULL) || (argv==NULL)) {
		return;
	}

	debug_flags &= (~op_done);
	op_done |= (debug_flags & PM2DEBUG_DO_OPT);

	while ((opt<(*argc)) && (argv[opt] != NULL))
	{
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
		
		if (strncmp(argv[opt], "--debug:", 8)) {
			opt++;
			continue;
		}

		if (debug_flags & PM2DEBUG_DO_OPT) {
			if (!strcmp(argv[opt]+8, "register")) {
				if (debug_type_register.actions[PM2DEBUG_SHOW]
				    == PM2DEBUG_DEFAULT) {
					pm2debug_setup(&debug_type_register, 
						       PM2DEBUG_SHOW, 
						       PM2DEBUG_ON);
				}
				show_reg=1;
				goto option_done;
			}
			for (type=0; type<nb_debug_type_registered; type++) {
				if (apply_option(debug_type_registered[type], 
						 argv[opt]+8)) {
					goto option_done;
				}
			}
			options[nb_options++]=argv[opt]+8;
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
	if (show_reg) {
		show_registered();
	}
}

#if !defined(MARCEL)
int __pm2debug_printf_state=PM2DEBUG_MARCEL_PRINTF_ALLOWED;
#else
int __pm2debug_printf_state=PM2DEBUG_MARCEL_PRINTF_DENY;
#endif

void pm2debug_printf_state(int state)
{
	__pm2debug_printf_state=state;
}

#define my_print(fmt, ...) \
	do { \
		size=tbx_snprintf(cur, remaining, fmt, ##__VA_ARGS__); \
		remaining-=size; \
		if (remaining<=0) \
			goto do_write; \
		eaten+=size; \
		cur+=size; \
	} while (0);

#define marcel_printf_allowed() (__pm2debug_printf_state == PM2DEBUG_MARCEL_PRINTF_ALLOWED)

#undef pm2debug_printf
int pm2debug_printf(debug_type_t *type, int level, int line, const char* file, 
		    const char *format, ...)
{
	va_list ap;

	if (!type) { 
		return 0;
	}
	
	if (type->show==PM2DEBUG_AUTO_REGISTER) {
		pm2debug_register_auto(type);
	}

	if (type->show >= level) {
		char buffer[PM2DEBUG_MAXLINELEN],*cur=buffer;
		int eaten=0,remaining=PM2DEBUG_MAXLINELEN,size;
		if (get_action_value(type, PM2DEBUG_SHOW_PREFIX)) {
			my_print("%s", type->prefix);
		}
		if (get_action_value(type, PM2DEBUG_SHOW_FILE)) {
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
		if (get_action_value(type, PM2DEBUG_SHOW_LWP)) {
			my_print("[P%02d] ", 
				 ((marcel_printf_allowed()
				   && MA_LWP_SELF) 
				  ? ma_get_task_vpnum(marcel_self()) : -1));
		}
#endif
		if (get_action_value(type, PM2DEBUG_SHOW_THREAD)) {
			my_print("(%12p:% 3d:%-15s) ", marcel_printf_allowed() ?
			marcel_self():(void*)-1, marcel_printf_allowed() ?
			marcel_self()->number:-99, marcel_printf_allowed() ?
			marcel_self()->as_entity.name:"");
		}
#endif /* MARCEL */
		va_start(ap, format);
		eaten+=tbx_vsnprintf(cur, remaining, format, ap);
		va_end(ap);
do_write:
		if (eaten>PM2DEBUG_MAXLINELEN)
			eaten=PM2DEBUG_MAXLINELEN;
		/* Write everything in just one go to avoid any interleaving */
		write(STDERR_FILENO,buffer,eaten);
	}

	return 0;
}

#ifdef HAVE_BACKTRACE
int pm2debug_backtrace(void ** array, int size) {
	int ret;
#ifdef MARCEL
	marcel_extlib_protect();
#endif
	ret = backtrace(array,size);
#ifdef MARCEL
	marcel_extlib_unprotect();
#endif
	return ret;
}
#endif /* HAVE_BACKTRACE */
