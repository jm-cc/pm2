/*segfault au dessus d'environ 35 signaux non traités dans f1 */ 

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

static pthread_t thr1,thr2,thr3;

void traitant_winch(int sig)
{
   fprintf(stderr,"traitement du signal %d par thread %lx\n",sig,pthread_self());   
   fprintf(stderr,"thread %lx s'envoie signal %d\n",pthread_self(),SIGUSR1);   
   pthread_kill(pthread_self(),SIGUSR1);
}

void traitant(int sig) 
{  
   fprintf(stderr,"traitement du signal %d par thread %lx\n",sig,pthread_self());   
   if (pthread_equal(pthread_self(),thr1))
	  {
       fprintf(stderr,"signal %d envoyé par thread %lx au thread %lx\n",SIGWINCH,pthread_self(),thr3);
		 pthread_kill(thr3,SIGWINCH);
	  }
}

void *f2(void * arg);
void *f3(void * arg);

void *f1(void * arg) 
{ 
   fprintf(stderr,"f1 : %lx\n",pthread_self());
	pthread_create(&thr3,NULL,f3,NULL);  
   pthread_create(&thr2,NULL,f2,NULL);  
   int i,sig;
   for (i=0;i<10;i++)
	{
	   sig = SIGUSR1;
      fprintf(stderr,"boucle f1 %d : signal %d envoyé par thread %lx au thread %lx\n",i,sig,pthread_self(),thr2);   
	   pthread_kill(thr2,sig);
   }
   
   while(1)
	  pause(); 
 
   pthread_exit(0);
}

void *f2(void * arg) 
{ 
   fprintf(stderr,"f2 : %lx\n",pthread_self());
   int i,sig;
   for (i=0;i<10;i++)
	{  
	   sig = SIGUSR2;
      fprintf(stderr,"boucle f2 %d : signal %d envoyé par thread %lx au thread %lx\n",i,sig,pthread_self(),thr1);   
		pthread_kill(thr1,sig);
      pause();
   }
   
   pthread_exit(0);
}

void *f3(void * arg)
{
   fprintf(stderr,"f3 : %lx\n",pthread_self());
   while(1)
	  pause();

   pthread_exit(0);
}

int main(void)
{
   void *retour;
   struct sigaction action;
   
   sigemptyset(&action.sa_mask);
   action.sa_handler = traitant;
   action.sa_flags = 0;
	
   sigaction(SIGINT,&action,NULL);
   sigaction(SIGQUIT,&action,NULL);
   sigaction(SIGUSR1,&action,NULL);
   sigaction(SIGUSR2,&action,NULL);

   action.sa_handler = traitant_winch;
	sigaction(SIGWINCH,&action,NULL);

   pthread_create(&thr1,NULL,f1,NULL);
   pthread_join(thr1,&retour);
   pthread_join(thr2,&retour);

   printf("sortie du programme\n");
   return 0;
}
