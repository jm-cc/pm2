#ifdef MARCEL
#include "pthread_pmarcel.h"
#else
#include <signal.h>
#include <pthread.h>
#endif
#include <stdio.h>
#include <unistd.h>

volatile int fini;

static void traitant(int signo)
{
   fini ++;
   fprintf(stderr,"--- %d fois t1 dans traitant : signal %d ---\n",fini,signo);
   if (fini<3)
   {
      fprintf(stderr,"- raise(%d) -\n",signo);
      raise(SIGUSR2);
   }
   fprintf(stderr,"--- t1 sorti du traitant ---\n");
   return;
}

void *recsig(void * arg)
{   
   fini = 0;
   fprintf(stderr,"entree de t1 dans recsig -> raise(%d)\n",SIGUSR2);
   fprintf(stderr,"- raise(%d) -\n",SIGUSR2);
   raise(SIGUSR2);
   fprintf(stderr,"sortie de t1 de recsig : pthread_exit(1)\n");   
   pthread_exit((void *)1);
}

int main(int argc, char *argv[])
{
   pthread_t t1;
   void * value_ptr1;
   
   struct sigaction act;
   act.sa_handler = traitant;
   sigemptyset(&act.sa_mask);
   act.sa_flags = 0;
   act.sa_flags = SA_RESTART;
   sigaction(SIGUSR2,&act,NULL);
   
   pthread_attr_t attr;

   fprintf(stderr,"structure attr inititialisee\n");
   pthread_attr_init(&attr);
      
   pthread_create(&t1,&attr,recsig,NULL);
   pthread_join(t1,&value_ptr1);
   fprintf(stderr,"main join t1 : value_ptr1 = %d\n",(int)value_ptr1);
 
   return 0;
}

