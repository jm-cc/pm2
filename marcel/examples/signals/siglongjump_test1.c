/* ce test ne fonctionne pas, faudra débugger */

#include<stdio.h>
#include<stdlib.h>
#include<signal.h>
#include<setjmp.h>
#include<unistd.h>
#include<pthread.h>

sigjmp_buf contexte;
int fin = 0;
pthread_t thr1,thr2;

void traitant_mask(int sig)
{
   fprintf(stderr,"thread %lx dans traitant_mask signal %d\n",pthread_self(),sig);
}

void traitant_fin(int sig)
{
   fprintf(stderr,"thread %lx dans traitant_fin signal %d\n",pthread_self(),sig);
   fin = 1;
}

void traitant_sigfpe(int sig)
{
   fprintf(stderr,"traitant : signal %d reçu\n",sig);
   siglongjmp(contexte,1);
   /* si on est ici, le saut a raté */
   fprintf(stderr,"saut raté\n");
}

void *send_usr(void *arg)
{  
   sigset_t set;
   sigemptyset(&set);
   sigaddset(&set,SIGFPE);
   pthread_sigmask(SIG_BLOCK,&set,NULL);  

  fprintf(stderr,"entree dans send_usr (%lx)\n", pthread_self());   

  while(!fin)
	 {
       pthread_kill(thr1,SIGUSR1);
       sleep(1);
	 }
  pthread_exit(0);
}

void *division(void *arg)
{
   fprintf(stderr,"entree dans division\n");   

   int p,q,r;
   
   pthread_create(&thr2,NULL,send_usr,NULL);  

   sigset_t set;
   sigemptyset(&set);
   sigaddset(&set,SIGUSR1);

	if(sigsetjmp(contexte,1) == 0) // essayer 0 aussi
	{
	   pthread_sigmask(SIG_BLOCK,&set,NULL);  
 
	   while(1) {
		  fprintf(stderr,"entrer dividende p :");
		  if (fscanf(stdin,"%d",&p) == 1)
			 break;
		}
     
		while(1) {
		  fprintf(stderr,"entrer diviseur q : (&q = %p)\n",&q);
		  if (fscanf(stdin,"%d",&q) == 1)
			 break;
		}

		r = p/q;
		fprintf(stderr,"rapport p/q = %d\n",r);
	}
	else 
   {
	  /* on est arrivé ici par siglongjmp */
	  fprintf(stderr,"retour de siglongjmp : division par zéro\n");
	  // pendant la pause, on devrait voir de nouveau s'afficher des lignes
	}

	  sleep(10);
	pthread_kill(thr2,SIGQUIT);

   pthread_exit(0);
}

int main(void)
{
   fprintf(stderr,"main s'appelle %lx\n",pthread_self());

   struct sigaction action;
	action.sa_flags = 0;	
   //sigfillset(&action.sa_mask);
	action.sa_handler = traitant_sigfpe;
   sigaction(SIGFPE,&action,NULL); 
   action.sa_handler = traitant_mask;  
   sigaction(SIGUSR1,&action,NULL); 
    action.sa_handler = traitant_fin;  
   sigaction(SIGQUIT,&action,NULL); 

   pthread_create(&thr1,NULL,division,NULL);
   
	pthread_join(thr1,NULL);
	pthread_join(thr2,NULL);

   fprintf(stderr,"fin du main\n");

   return 0;
}
