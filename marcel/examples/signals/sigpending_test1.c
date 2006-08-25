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
   sigset_t mask,pending;
   
   action.sa_handler = traitant;
   sigemptyset(&action.sa_mask);
   action.sa_flags = 0; //pas de restart
   for(i=1 ; i<32 ; i++)
     if (sigaction(i,&action,NULL) != 0)
       fprintf(stderr,"%ld : %d pas capturé\n",(long) getpid(),i);

   /* on bloque tout sauf SIGINT */
   sigfillset(&mask);
   sigdelset(&mask,SIGINT);
   sigprocmask(SIG_BLOCK,&mask,NULL);
   
   /* un appel systeme lent bloqué */
   fprintf(stderr,"un read : \n");
   read(0,&i,sizeof(int));
   
   /* voyons maintenant qui est en attente */
	
   sigpending(&pending);
   for(i=1;i<32;i++)
     if (sigismember(&pending,i))
       fprintf(stderr,"signal %d sigpending\n",i);

   /* on débloque les signaux pour les voir arriver */
   sigemptyset(&pending);
   sigprocmask(SIG_SETMASK,&pending,NULL);

   return 0;
}
