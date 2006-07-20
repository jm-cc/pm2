#include <signal.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

/* envoie d'un signal au processus de l'extérieur */
volatile int fini;
static void traitant(int signo)
{
   fprintf(stderr,":) :) t1 est dans traitant :) :)\n");
   if (signo == SIGUSR2)
      fprintf(stderr,"c'est bien le signal SIGUSR2\n");
   else 
      fprintf(stderr,"ce n'est pas le signal SIGUSR2\n");
   fini = 1;
   return;
}

void *givsig(void * arg)
{
   fprintf(stderr,"entree de t2 dans givsig -> on envoie le signal SIGUSR2 a t1\n");
   pthread_t t = (pthread_t)arg;
   pthread_kill(t,SIGUSR2);
   fprintf(stderr,"sortie de t2 de givsig : pthread_exit(2)\n");   
   pthread_exit((void *)2);
}

void *recsig(void * arg)
{   
   fprintf(stderr,"entree de t1 dans recsig -> on attend le signal SIGUSR2 avec usleep\n");
   while(!fini)
   {
      fprintf(stderr,".");
      usleep(1000000);
   }
   if (fini)
      fprintf(stderr,"retour de t1 dans recsig : on a fini le traitement du signal SIGUSR2\n");
   else
      fprintf(stderr,"retour de t1 dans recsig : on a pas fait le traitement du signal SIGUSR2\n");
   fprintf(stderr,"sortie de t1 de recsig : pthread_exit(1)\n");   
   pthread_exit((void *)1);
}

int main(int argc, char *argv[])
{
   fprintf(stderr,"entree dans le main\n");
   void * value_ptr1;
   void * value_ptr2;

   struct sigaction act;
   act.sa_handler = traitant;
   fprintf(stderr,"adresse traitant : %p\n",traitant);
   sigemptyset(&act.sa_mask);
   act.sa_flags = SA_RESTART;
   if (!sigaction(SIGUSR2,&act,NULL))
      fprintf(stderr,"sigaction ok : traitant associe a SIGUSR2\n");
   else 
      fprintf(stderr,"erreur sigaction\n");
   
   pthread_t t1,t2;
   pthread_attr_t attr;

   fprintf(stderr,"structure attr inititialisee\n");
   pthread_attr_init(&attr);
      
   fprintf(stderr,"t1 est cree avec attr : %p\nil doit executer recsig\n",&t1);
   pthread_create(&t1,&attr,recsig,NULL);

   fprintf(stderr,"t2 est cree avec attr : %p\nil doit executer givsig(t1)\n",&t2);
   pthread_create(&t2,&attr,givsig,(void *)t1);
 
   pthread_join(t1,&value_ptr1);
   fprintf(stderr,"main join t1 : value_ptr1 = %d\n",(int)value_ptr1);
   pthread_join(t2,&value_ptr2);
   fprintf(stderr,"main join t2 : value_ptr2 = %d\n",(int)value_ptr2);

   return 0;
}

