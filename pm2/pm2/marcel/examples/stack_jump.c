
#include "marcel.h"
#include "marcel_alloc.h"

#ifdef ENABLE_STACK_JUMPING

static void *stack;

any_t thread_func(any_t arg)
{
  marcel_detach(marcel_self());

  printf("marcel_self = %p, sp = %p\n", marcel_self(), (void *)get_sp());

  /* Allocation d'une pile annexe, *obligatoirement* de taille SLOT_SIZE.  */
  stack = marcel_slot_alloc();
  printf("New stack goes from %p to %p\n", stack, stack + SLOT_SIZE - 1);

  /* Préparation de la nouvelle pile pour accueillir le thread */
  marcel_prepare_stack_jump(stack);

  /* basculement sur la pile annexe */
  set_sp(stack + SLOT_SIZE - 1024);

  printf("marcel_self = %p, sp = %p\n", marcel_self(), (void *)get_sp());

  printf("Here we are!!\n");

  /* on ne peut pas retourner avec "return", donc : */
  marcel_exit(NULL);

  return NULL;
}

#endif

int marcel_main(int argc, char *argv[])
{
#ifndef ENABLE_STACK_JUMPING

  fprintf(stderr,
	  "Please compile this program (and the Marcel library)\n"
	  "with the -DENABLE_STACK_JUMPING flag.\n");
  exit(1);

#else

  marcel_init(&argc, argv);

  marcel_create(NULL, NULL, thread_func, NULL);

  marcel_end();

#endif
  return 0;
}



