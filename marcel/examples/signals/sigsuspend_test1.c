#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

void traitant(int sig) 
{  
   fprintf(stderr,"traitant du signal %d par thread %lx\n",sig,pthread_self());   
}


void *suspend(void * arg) 
{ 
   sigset_t setwait; 
   sigemptyset(&setwait);
   
   if (!sigaddset(&setwait, SIGINT))
	  fprintf(stderr,"signal %d dans masque sigsuspend du thread %lx\n",SIGINT,pthread_self()); 
   if (!sigaddset(&setwait, SIGQUIT))
  	  fprintf(stderr,"signal %d dans masque sigsuspend du thread %lx\n",SIGQUIT,pthread_self()); 

   fprintf(stderr,"on lance sigsuspend pour le thread %lx:\n",pthread_self());
   
   if(sigsuspend(&setwait) == -1)
      fprintf(stderr,"après sigsuspend de thread %lx= -1\n",pthread_self()); 
   
   fprintf(stderr,"on lance pause pour le thread %lx:\n",pthread_self());
   pause();
   fprintf(stderr,"le pause est fini pour le thread %lx:\n",pthread_self());

   pthread_exit(0);
}

int main(void)
{
   fprintf(stderr,"le thread main s'appelle %lx\n",pthread_self());
   
   void *retour;
   static pthread_t thr1,thr2;
   struct sigaction action;
   
   sigemptyset(&action.sa_mask);
   action.sa_handler = traitant;
   action.sa_flags = 0;
	
   sigaction(SIGINT,&action,NULL);
   sigaction(SIGQUIT,&action,NULL);
   sigaction(SIGWINCH,&action,NULL);
   
   pthread_create(&thr1,NULL,suspend,NULL);
   pthread_create(&thr2,NULL,suspend,NULL);  
   pthread_join(thr1,&retour);
   pthread_join(thr2,&retour);

   printf("main fini\n");
   return 0;
}
