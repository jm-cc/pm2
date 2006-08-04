#include<signal.h>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<errno.h>

void traitant(int numero)
{
   switch(numero)
   {
      case SIGQUIT:
	 fprintf(stderr,"SIGQUIT reçu\n");
	 break;
      case SIGINT:
         fprintf(stderr,"SIGINT reçu\n");
         break;
   }
}

int main(void)
{
   struct sigaction action;
   action.sa_handler = traitant;
   sigisemptyset(&action.sa_mask);
   action.sa_flags = 0;
   if (sigaction(SIGQUIT,&action,NULL) != 0)
   {
     fprintf(stderr,"erreur %d\n",errno);
     exit(-1);
   }
   action.sa_handler = traitant;
   sigisemptyset(&action.sa_mask);
   action.sa_flags = SA_RESTART | SA_RESETHAND;
   if (sigaction(SIGINT,&action,NULL) != 0)
   {
      fprintf(stderr,"erreur %d\n",errno);
      exit(-1);
   }
   /* lecture continue, pour avoir un appel système lent bloqué */
   while(1)
   {
      int i;
      fprintf(stderr,"appel read()\n");
      if (read(0,&i,sizeof(int))<0)
	 if (errno == EINTR)
	   fprintf(stderr,"EINTR\n");
   }
   return 0;
}
