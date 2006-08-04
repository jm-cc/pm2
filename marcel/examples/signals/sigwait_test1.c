#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>

static pthread_t thr1,thr2;
void *receive(void * arg);

void traitant(int sig)
{
   fprintf(stderr,"%lx dans traitant du signal %d\n",pthread_self(),sig);
}

void *send(void * arg) 
{ 
   sigset_t setwait; 
   sigemptyset(&setwait);
   sigaddset(&setwait, SIGINT); 
   sigaddset(&setwait, SIGQUIT);
   pthread_sigmask(SIG_BLOCK,&setwait,NULL);
   pthread_create(&thr2,NULL,receive,NULL);  

   pthread_sigmask(SIG_UNBLOCK,&setwait,NULL);

   int sig;
   int loterie;
   sigemptyset(&setwait);
   sigaddset(&setwait, SIGUSR1); 
   sigaddset(&setwait, SIGWINCH);

   pthread_sigmask(SIG_BLOCK,&setwait, NULL);
  
   srand(time(NULL));
 send:
  
   loterie = rand(); 
  
   if (loterie < (RAND_MAX/2))
	{
	  fprintf(stderr,"%lx envoie 1 a %lx\n",thr1,thr2);
     pthread_kill(thr2,SIGQUIT);
	}
	else
	{
	   fprintf(stderr,"%lx envoie 0 a %lx\n",thr1,thr2);
	   pthread_kill(thr2,SIGINT);
	}
   
   //fprintf(stderr,"sigwait : %lx attend %d ou %d :\n",pthread_self(),SIGINT,SIGQUIT);
   sigwait(&setwait,&sig);
   //fprintf(stderr,"thread %lx après sigwait : *sig vaut %d\n",pthread_self(),sig); 

   if (sig == SIGUSR1)
	{
	   goto send;
	}
   else if (sig == SIGWINCH)
   {
	   fprintf(stderr,"fin des messages\n");
	}
   else
	{
	   fprintf(stderr,"erreur !\n");
	}

   pthread_exit(0);
}

void *receive(void * arg) 
{ 
   int sig;
   sigset_t setwait; 
   sigemptyset(&setwait);
   sigaddset(&setwait, SIGINT); 
   sigaddset(&setwait, SIGQUIT);
   pthread_sigmask(SIG_BLOCK,&setwait,NULL);

   int max = 10;  
   int iterations = 0;
   int somme = 0;
   int produit = 1;

 wait:

   //fprintf(stderr,"sigwait : %lx attend %d ou %d :\n",pthread_self(),SIGINT,SIGQUIT);
   sigwait(&setwait,&sig);
   //fprintf(stderr,"thread %lx après sigwait : *sig vaut %d\n",pthread_self(),sig); 

	if (sig == SIGINT)
	{
	   fprintf(stderr,"%lx reçoit 0 de %lx\n",thr2,thr1);
	}
   else if (sig == SIGQUIT)
   {
	   somme += produit;
  	   fprintf(stderr,"%lx reçoit 1 de %lx\n",thr2,thr1);
	}
   else
	{
	   fprintf(stderr,"erreur !\n");
	}

   iterations ++;
   produit *= 2;

	if (iterations < max)
	{
      fprintf(stderr,"%lx demande autre bit a %lx\n",thr2,thr1);
      pthread_kill(thr1,SIGUSR1);
      goto wait;
	}
   else
	{
      fprintf(stderr,"%lx envoie signal de fin a %lx\n",thr2,thr1);
	   pthread_kill(thr1,SIGWINCH);
   }

   fprintf(stderr,"somme = %d\n",somme);

   pthread_exit(0);
}

int main(void)
{
   void *retour;
   struct sigaction action;
   
   sigemptyset(&action.sa_mask);
   memset(&action,0,sizeof(action));
   action.sa_handler = traitant;
   sigaction(SIGINT,&action,NULL);
   sigaction(SIGQUIT,&action,NULL);

   action.sa_handler = traitant;
   sigaction(SIGUSR1,&action,NULL);
	sigaction(SIGWINCH,&action,NULL);

   sigaction(SIGUSR2,&action,NULL);
   sigaction(SIGALRM,&action,NULL);
   
   pthread_create(&thr1,NULL,send,NULL);
   
   pthread_join(thr1,&retour);
   pthread_join(thr2,&retour);

   fprintf(stderr,"fin du programme\n");
   return 0;
}
