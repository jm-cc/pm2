#include <stdio.h>
#include <stdlib.h>

#define __USE_GNU
#include <dlfcn.h>

extern int toto;
extern int __attribute__ ((weak)) go_marcel_main();
int __attribute__ ((weak)) marcel_pthread_initialize();
//{
//  printf("toto\n");
//}
static int (*global_main) (int, char **, char **);
#if 1
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
inline static void __attribute__ ((noreturn)) launch_libc_start_main() {
  char* err;
  global_libc_start_main=dlsym(RTLD_NEXT, "__libc_start_main");
  err=dlerror();
  //global_libc_start_main=__libc_start_main;
#ifdef PM2DEBUG
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

int marcel_main(int argc, char** argv)
{
#ifdef PM2DEBUG
  printf("on the good stack\n");
#endif
  if(marcel_pthread_initialize) {
    marcel_pthread_initialize(&argc, argv);
  }
  launch_libc_start_main();
}


extern __libc_start_main_t __libc_start_main __attribute__ ((noreturn));
int BP_SYM (__libc_start_main) (int (*main) (int, char **, char **),
				      int argc,
				      char *__unbounded *__unbounded ubp_av,
				      void (*init) (void),
				      void (*fini) (void),
				      void (*rtld_fini) (void),
				      void *__unbounded stack_end)
{
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
    go_marcel_main(argc, ubp_av);
#ifdef PM2DEBUG
    printf("__libc_start_main error: comming back from marcel\n");
#endif
    exit(1);    
  } else {
#ifdef PM2DEBUG
    printf("starting direct. go_marcel_main=%p, %p\n", go_marcel_main);
#endif
    launch_libc_start_main();
  }
}

typeof(__libc_start_main) local__libc_start_main 
	__attribute__((alias("__libc_start_main")));

#else
static char **global_env;
extern int jump_to_go_marcel_main (int argc, char **argv, char**env);

int marcel_main(int argc, char** argv)
{
  static int (*fp) (int *argc, char** argv);
  char*err;
  printf("on the good stack\n");
  //printf("before marcel_main\n");
  //if (!fp) fp=dlsym(RTLD_DEFAULT, "marcel_pthread_initialize");
  //err=dlerror();
  //printf("pthread_initialize at %p error: %s\n", fp, err);
  //if (fp) {
  //  (*fp)(&argc, argv);
  //}
  if(marcel_pthread_initialize) {
    marcel_pthread_initialize(&argc, argv);
  }
  return (*global_main)(argc, argv, global_env);
}

int jump_to_go_marcel_main (int argc, char **argv, char **env)
{
  static int (*fp) (int, char**);
  int result;
  char*err;
  global_env=env;
  //printf("before go_marcel_main\n");
  //if (!fp) fp=dlsym(RTLD_DEFAULT, "go_marcel_main");
  //err=dlerror();
  //printf("go_marcel_main at %p error: %s\n", fp, err);
  if (go_marcel_main) {
    result=go_marcel_main(argc, argv);
  } else {
    result=(*global_main)(argc, argv, env);
  }
  //printf("result of go_marcel_main: %i\n", result);  
  return result;
}

void* change_main(void* ret, int (*main) (int, char **, char **))
{
  static void* fp;
  char* err;
  printf("changing main\n");
  global_main=main;
  main=jump_to_go_marcel_main;
  if (!fp) fp=dlsym(RTLD_NEXT, "__libc_start_main");
  err=dlerror();
  if (err || !fp) {
    printf("__libc_start_main at %p error: %s\n", fp, err);
    exit(1);
  }  

  return fp;
}

extern int __libc_start_main (int main)
     asm ("__libc_start_main");

__asm__ (".align 16 \n\
.globl __libc_start_main \n\
        .type    __libc_start_main,@function \n\
__libc_start_main: \n\
        call change_main \n\
        jmp  *%eax\n\
.Lmye: \n\
        .size    __libc_start_main,.Lmye-__libc_start_main \n\
") ;


int sched_yield2(void)
{
	static int (* fp)(void);
	static int (* fp2)(void);
	if (!fp) {
		fp=dlsym(RTLD_DEFAULT, "marcel_yield");
		if (!fp) {
			fp=dlsym(RTLD_NEXT, "sched_yield");
		}
	}
	return (*fp)();
}
#endif
