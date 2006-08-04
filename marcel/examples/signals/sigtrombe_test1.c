/*appel du signal abort au dessus d'environ 35 signaux non traités */ 

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

void traitant(int sig) 
{  
   fprintf(stderr,"traitant du signal %d\n",sig);   
}


void *wait(void * arg) 
{ 
   fprintf(stderr,"thread waiting %lx\n",pthread_self());
   
   int i;
   int sig;
   sigset_t setwait; 
  
   for (i=0;i<1;i++)
	{
   	sig = SIGUSR1;
      sigemptyset(&setwait);
		sigaddset(&setwait, sig); 
      fprintf(stderr,"sigwait : thread %lx attend signal : %d\n",pthread_self(),sig);
      sigwait(&setwait,&sig);
      fprintf(stderr,"après sigwait : *sig vaut %d\n",sig); 
	}
   pthread_exit(0);
}

void *send(void * arg) 
{ 
   int i;
   int sig;

   pthread_t thread = (pthread_t)arg;
  
   for (i=0;i<100;i++)
	{
	   sig = SIGWINCH;
      fprintf(stderr,"for %d : envoie signal %d au thread %lx\n",i,sig,thread); 
      pthread_kill(thread,sig);
	}
   for (i=0;i<100;i++)
	{
	   sig = SIGQUIT;
      fprintf(stderr,"for %d : envoie signal %d au thread %lx\n",i,sig,thread); 
      pthread_kill(thread,sig);
	}
   for (i=0;i<100;i++)
	{
	   sig = SIGINT;
      fprintf(stderr,"for %d : envoie signal %d au thread %lx\n",i,sig,thread); 
      pthread_kill(thread,sig);
	}
   pthread_exit(0);
}

int main(void)
{
   void *retour;
   static pthread_t thr1,thr2;
   struct sigaction action;
   
   sigemptyset(&action.sa_mask);
   action.sa_handler = traitant;
   action.sa_flags = 0;
	
   sigaction(SIGINT,&action,NULL);
   sigaction(SIGQUIT,&action,NULL);
   sigaction(SIGWINCH,&action,NULL);
   sigaction(SIGUSR1,&action,NULL);
	sigaction(SIGUSR2,&action,NULL);
   
   pthread_create(&thr1,NULL,wait,NULL);
   pthread_create(&thr2,NULL,send,(void *)thr1);  
   pthread_join(thr1,&retour);
   pthread_join(thr2,&retour);

   printf("sortie du programme\n");
   return 0;
}
