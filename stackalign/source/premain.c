#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>

#include <dlfcn.h>

extern int __attribute__ ((weak)) go_marcel_main(int (*main_func)(int, char*[]), int argc, char *argv[]);
extern int __attribute__ ((weak)) marcel_initialize(int *argc, char *argv[]);

int  __attribute__ ((weak)) marcel_a_demare=0;

#ifdef PM2DEBUG
# warning disabling PM2DEBUG: printf call to early !
#// undef PM2DEBUG
#endif

/****************************************************************
 * Les variables globales
 */
#ifndef __unbounded
#define __unbounded
#endif
static int (*global_main) (int, char **, char **);
static int global_argc;
static char *__unbounded *__unbounded global_ubp_av;
static void (*global_init) (void);
static void (*global_fini) (void);
static void (*global_rtld_fini) (void);
static void *__unbounded global_stack_end;

#define BP_SYM(sym) sym
typedef int BP_SYM (__libc_start_main_t) (int (*main) (int, char **, char **),
					  int argc,
					  char *__unbounded *__unbounded ubp_av,
					  void (*init) (void),
					  void (*fini) (void),
					  void (*rtld_fini) (void),
					  void *__unbounded stack_end);
static __libc_start_main_t *global_libc_start_main __attribute__ ((noreturn));


extern __libc_start_main_t local__libc_start_main;
extern __libc_start_main_t __libc_start_main;
/****************************************************************
 * launch_libc_start_main :
 * appelle la vraie fonction __libc_start_main
 * en utilisant les variables globales.
 */
inline static void __attribute__ ((noreturn)) launch_libc_start_main() {
	char* err;
	global_libc_start_main=dlsym(RTLD_NEXT, "__libc_start_main");
	err=dlerror();
	//global_libc_start_main=__libc_start_main;
#ifdef PM2DEBUG
#warning PM2DEBUG enabled
	printf("have old start_main\n");
#endif
	if (err || !global_libc_start_main) {
		printf("__libc_start_main at %p error: %s\n", global_libc_start_main, err);
		exit(1);
	}

#ifdef PM2DEBUG
	printf("old = %p, our = %p \n", global_libc_start_main, local__libc_start_main);
#endif

	global_libc_start_main(global_main,
			       global_argc,
			       global_ubp_av,
			       global_init,
			       global_fini,
			       global_rtld_fini,
			       global_stack_end);
}

/****************************************************************
 * marcel_main : appelle la vraie fonction __libc_start_main
 */
int marcel_main(int argc, char** argv)
{
#ifdef PM2DEBUG
	printf("on the good stack\n");
#endif
	if(marcel_initialize) {
#ifdef PM2DEBUG
		printf("initialising marcel\n");
#endif
		marcel_initialize(&argc, argv);
		marcel_a_demare=1;
	}
#ifdef PM2DEBUG
	printf("launching libc_start_main\n");
#endif
	launch_libc_start_main();
}


/****************************************************************
 * notre fonction __libc_start_main
 * 1) remplit les variables globales
 * 2) Si marcel présent (go_marcel_main non nul)
 *      appelle go_marcel_main
 *        (qui change de pile, appelle marcel_main qui appelle __libc_start_main)
 *    Sinon 
 *      appelle la vraie fonction __libc_start_main
 */
extern __libc_start_main_t __libc_start_main __attribute__ ((noreturn));
int BP_SYM (__libc_start_main) (int (*main) (int, char **, char **),
				      int argc,
				      char *__unbounded *__unbounded ubp_av,
				      void (*init) (void),
				      void (*fini) (void),
				      void (*rtld_fini) (void),
				      void *__unbounded stack_end)
{
	int ret;
#ifdef PM2DEBUG
	printf("changing main\n");
#endif
	global_main=main;
	global_argc=argc;
	global_ubp_av=ubp_av;
	global_init=init;
	global_fini=fini;
	global_rtld_fini=rtld_fini;
	global_stack_end=stack_end;
	
	//main=jump_to_go_marcel_main;
	if (go_marcel_main) {
#ifdef PM2DEBUG
		printf("running go_marcel_main at %p\n", go_marcel_main);
#endif
		ret = go_marcel_main(marcel_main, argc, ubp_av);
#ifdef PM2DEBUG
		printf("coming back from marcel\n");
#endif
		exit(ret);
	} else {
#ifdef PM2DEBUG
		printf("starting direct. go_marcel_main=%p, marcel_initialize=%p\n", go_marcel_main, marcel_initialize);
#endif
		if (marcel_initialize)
			marcel_initialize(&global_argc, global_ubp_av);
		launch_libc_start_main();
	}
}

typeof(__libc_start_main) local__libc_start_main 
	__attribute__((alias("__libc_start_main")));

