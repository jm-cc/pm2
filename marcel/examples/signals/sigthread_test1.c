/* chaque thread i créé bloque tous les signaux, sauf i */ 

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>

#define NB_THREADS 5

static void * fonction_thread(void * void_arg);
static void traitant(int sig);

int main(void)
{
   int i;
   static pthread_t thr1,thr2,thr3,thr4,thr5;
   struct sigaction action;

   fprintf(stderr,"thread main pid : %ld\n",(long)getpid());

   sigfillset(&action.sa_mask);
   action.sa_handler = traitant;
   action.sa_flags = 0;

   for (i=1;i<NB_THREADS+1;i++)
	  if (sigaction(i,&action,NULL) == 0)
		 fprintf(stderr,"signal %d capturé\n",i);
     else
		 fprintf(stderr,"!!! signal %d non capturé\n",i);

	pthread_create(&thr1,NULL,fonction_thread,(void *)1);
	pthread_create(&thr2,NULL,fonction_thread,(void *)2);
	pthread_create(&thr3,NULL,fonction_thread,(void *)3);
	pthread_create(&thr4,NULL,fonction_thread,(void *)4);
	pthread_create(&thr5,NULL,fonction_thread,(void *)5);

   pthread_join(thr1,NULL);
	pthread_join(thr2,NULL);
	pthread_join(thr3,NULL);
	pthread_join(thr4,NULL);
	pthread_join(thr5,NULL);
	
   fprintf(stderr,"thread main, je me termine\n");

   return 0;
}

static void * fonction_thread(void * void_arg)
{
   sigset_t masque;
   int numero = (int)void_arg;
   int i;
   
   fprintf(stderr,"thread %d, pid %ld\n",numero,(long)getpid());

   sigemptyset(&masque);
   for (i=1;i<NB_THREADS+1;i++)
  	   if (i != numero)
		  sigaddset(&masque,i);
   pthread_sigmask(SIG_BLOCK,&masque,NULL);
   
   fprintf(stderr,"thread %d bloque tout sauf signal %d\n",numero,numero);

   while(1)
	{
      pause();
	   fprintf(stderr,"thread %d a reçu un signal\n",numero);
   }
   
   pthread_exit(NULL);    
}

void traitant(int sig)
{
  fprintf(stderr,"traitant : >>> signal %d reçu <<<\n",sig);
}
