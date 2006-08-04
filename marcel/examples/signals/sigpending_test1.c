#include<stdio.h>
#include<stdlib.h>
#include<signal.h>
#include<errno.h>
#include<unistd.h>

void traitant(int sig)
{
  fprintf(stderr,"traitant : signal %d reçu\n",sig);
}

int main(void)
{
   int i;
   struct sigaction action;
   sigset_t ensemble;
   
   action.sa_handler = traitant;
   sigemptyset(&action.sa_mask);
   action.sa_flags = 0; //pas de restart
   for(i=1 ; i<32 ; i++)
     if (sigaction(i,&action,NULL) != 0)
       fprintf(stderr,"%ld : %d pas capturé\n",(long) getpid(),i);
     else
		 fprintf(stderr,"%d\n",i);

   /* on bloque tout sauf SIGINT */
   sigfillset(&ensemble);
   sigdelset(&ensemble,SIGINT);
   sigprocmask(SIG_BLOCK,&ensemble,NULL);
   
   /* un appel systeme lent bloqué */
   read(0,&i,sizeof(int));
   
   /* voyons maintenant qui est en attente */
	fprintf(stderr,"loup blanc\n");

   sigpending(&ensemble);
   for(i=1;i<32;i++)
     if (sigismember(&ensemble,i))
       fprintf(stderr,"signal en attente : %d\n",i);
   
   fprintf(stderr,"loup noir\n");

   /* on débloque les signaux pour les voirr arriver */
   sigemptyset(&ensemble);
   sigprocmask(SIG_SETMASK,&ensemble,NULL);

   return 0;
}
