/*
                      PM2 HIGH-PERF/ISOMALLOC
           High Performance Parallel Multithreaded Machine
                           version 3.0

     Gabriel Antoniu, Olivier Aumage, Luc Bouge, Vincent Danjean,
       Christian Perez, Jean-Francois Mehaut, Raymond Namyst

            Laboratoire de l'Informatique du Parallelisme
                        UMR 5668 CNRS-INRIA
                 Ecole Normale Superieure de Lyon

                      External Contributors:
                 Yves Denneulin (LMC - Grenoble),
                 Benoit Planquelle (LIFL - Lille)

                    1998 All Rights Reserved


                             NOTICE

 Permission to use, copy, modify, and distribute this software and
 its documentation for any purpose and without fee is hereby granted
 provided that the above copyright notice appear in all copies and
 that both the copyright notice and this permission notice appear in
 supporting documentation.

 Neither the institutions (Ecole Normale Superieure de Lyon,
 Laboratoire de L'informatique du Parallelisme, Universite des
 Sciences et Technologies de Lille, Laboratoire d'Informatique
 Fondamentale de Lille), nor the Authors make any representations
 about the suitability of this software for any purpose. This
 software is provided ``as is'' without express or implied warranty.

______________________________________________________________________________
$Log: pm2debug.h,v $
Revision 1.1  2000/05/09 10:49:36  vdanjean
pm2debug module

Revision 1.4  2000/04/28 18:33:36  vdanjean
debug actsmp + marcel_key

Revision 1.2  2000/04/11 09:07:17  rnamyst
Merged the "reorganisation" development branch.

Revision 1.1.2.1  2000/03/31 09:04:12  rnamyst
This is the old "debug.h" file.

______________________________________________________________________________
*/

#ifndef PM2DEBUG_EST_DEF
#define PM2DEBUG_EST_DEF

typedef enum {
	DEBUG_SHOW,
	DEBUG_SHOW_PREFIX,
	DEBUG_SHOW_FILE,
	DEBUG_CRITICAL,
	DEBUG_TRYONLY,
	DEBUG_REGISTER,
} debug_action_t;

typedef struct struct_debug_type_t debug_type_t;

typedef void (*debug_action_func_t)(debug_type_t*, debug_action_t, int);

struct struct_debug_type_t {
	int show;
	char* prefix;
	char* option_name;
	int show_prefix;
	int show_file;
	int critical;
	int try_only;
	debug_action_func_t setup;
	void* data;
};

#define NEW_DEBUG_TYPE(show, prefix, name) \
   {show, prefix, name, 0, 0, 0, 0, NULL, NULL}

extern debug_type_t debug_type_default;
void debug_setup_default(debug_type_t*, debug_action_t, int);

#ifdef PM2DEBUG
void pm2debug_init(int *argc, char **argv);
int pm2debug_printf(debug_type_t *type, int line, char* file, 
		     const char *format, ...);
void pm2debug_register(debug_type_t *type);
void pm2debug_setup(debug_type_t* type, debug_action_t action, int value);
#else
#define pm2debug_init(argc, argv)
#define pm2debug_printf(type, line, file, format, ...)
#define pm2debug_register(type)
#define pm2debug_setup(type, action, value)
#endif

#define debug_printf(type, fmt, args...) \
   ((type) && ((type)->show) ? \
    pm2debug_printf(type, __LINE__, __BASE_FILE__, fmt, ##args) \
    : (void)0 )

#define debug(fmt, args...) \
   debug_printf(&debug_type_default, fmt, ##args)

#endif





