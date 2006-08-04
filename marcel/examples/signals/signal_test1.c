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
   int sig;
   sigset_t setwait; 
   sigemptyset(&setwait);
   sigaddset(&setwait, SIGINT); 
   sigaddset(&setwait, SIGQUIT);
  
   fprintf(stderr,"sigwait : %lx attend %d et %d :\n",pthread_self(),SIGINT,SIGQUIT);
   sigwait(&setwait,&sig);
   fprintf(stderr,"après sigwait : *sig vaut %d\n",sig); 

   pthread_exit(0);
}

int main(void)
{
   void *retour;
   static pthread_t thr1,thr2;
	
   signal(SIGINT,traitant);
   signal(SIGQUIT,traitant);
   signal(SIGWINCH,traitant);
   
   pthread_create(&thr1,NULL,wait,NULL);
   pthread_create(&thr2,NULL,wait,NULL);  
   pthread_join(thr1,&retour);
   pthread_join(thr2,&retour);
   
   fprintf(stderr,"\n"); 

   pthread_create(&thr1,NULL,wait,NULL);
   fprintf(stderr,"envoi de SIGWINCH 28\n"); 
   pthread_kill(thr1,SIGWINCH);
   fprintf(stderr,"envoi de SIGQUIT 3\n"); 
   pthread_kill(thr1,SIGQUIT);
   pthread_join(thr1,&retour);

   printf("main fini\n");
   return 0;
}
