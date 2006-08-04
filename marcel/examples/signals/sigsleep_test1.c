#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

void traitant(int sig) 
{  
   fprintf(stderr,"traitant du signal %d\n",sig);   
}


void *dosleep(void * arg) 
{ 
   fprintf(stderr,"sleep :\n");
   sleep(5);
   fprintf(stderr,"apres sleep\n");
   pthread_exit(0);
}

int main(void)
{
   void *retour;
   static pthread_t thr1;
   struct sigaction action;
   
   sigemptyset(&action.sa_mask);
   action.sa_handler = traitant;
   action.sa_flags = 0;
	
   sigaction(SIGINT,&action,NULL);
   sigaction(SIGQUIT,&action,NULL);
   sigaction(SIGWINCH,&action,NULL);
   
   pthread_create(&thr1,NULL,dosleep,NULL);
   pthread_join(thr1,&retour);

   printf("main fini\n");
   return 0;
}
