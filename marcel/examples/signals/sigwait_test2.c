#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h> 

#define NB_PAIRES 200

static pthread_t thr1[NB_PAIRES];
static pthread_t thr2[NB_PAIRES];
void *receive(void * arg);

void traitant(int sig)
{
   fprintf(stderr,"%lx dans traitant du signal %d\n",pthread_self(),sig);
}

void *send(void * arg) 
{ 
   int i = (int)arg;

   int somme = 0;
   int produit = 1;

   sigset_t setwait; 
   sigemptyset(&setwait);
   sigaddset(&setwait, SIGINT); 
   sigaddset(&setwait, SIGQUIT);
   
   pthread_sigmask(SIG_BLOCK,&setwait,NULL);
 
   pthread_create(&thr2[i],NULL,receive,(void *)i);  

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
	  //fprintf(stderr,"%lx envoie 1 a %lx\n",thr1[i],thr2[i]);
      pthread_kill(thr2[i],SIGQUIT);
	   somme += produit;
   }
	else
	{
	  //fprintf(stderr,"%lx envoie 0 a %lx\n",thr1[i],thr2[i]);
	   pthread_kill(thr2[i],SIGINT);
	}
   
   //fprintf(stderr,"sigwait : %lx attend %d ou %d :\n",pthread_self(),SIGINT,SIGQUIT);
   sigwait(&setwait,&sig);
   //fprintf(stderr,"thread %lx après sigwait : *sig vaut %d\n",pthread_self(),sig); 

   if (sig == SIGUSR1)
	{
	   produit *= 2;
      goto send;
	}
   else if (sig == SIGWINCH)
   {
	   fprintf(stderr,"2 fin des messages : somme thread %lx : %d\n",thr2[i],somme);
	}
   else
	{
	   fprintf(stderr,"erreur !\n");
	}

   pthread_exit(0);
}

void *receive(void * arg) 
{ 
   int i = (int)arg;

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
	  //fprintf(stderr,"%lx reçoit 0 de %lx\n",thr2[i],thr1[i]);
	}
   else if (sig == SIGQUIT)
   {
	   somme += produit;
  	   //fprintf(stderr,"%lx reçoit 1 de %lx\n",thr2[i],thr1[i]);
	}
   else
	{
	   fprintf(stderr,"erreur !\n");
	}

   iterations ++;
   produit *= 2;

	if (iterations < max)
	{
	  //fprintf(stderr,"%lx demande autre bit a %lx\n",thr2[i],thr1[i]);
      pthread_kill(thr1[i],SIGUSR1);
      goto wait;
	}
   else
	{
	  //fprintf(stderr,"%lx envoie signal de fin a %lx\n",thr2[i],thr1[i]);
	   pthread_kill(thr1[i],SIGWINCH);
   }

	fprintf(stderr,"1 fin des messages : somme thread %lx : %d\n",pthread_self(),somme);

   pthread_exit(0);
}

int main(void)
{
   int i;
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
   
	for (i=0;i<NB_PAIRES;i++)
	  pthread_create(&thr1[i],NULL,send,(void *)i);
   
	for (i=0;i<NB_PAIRES;i++)
      pthread_join(thr1[i],&retour);
   for (i=0;i<NB_PAIRES;i++)
      pthread_join(thr2[i],&retour);

   fprintf(stderr,"fin du programme\n");
   return 0;
}
