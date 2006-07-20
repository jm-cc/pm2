#include <signal.h>
#include <semaphore.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

/* envoie d'un signal au processus de l'extérieur */
volatile int fini;
int i = 0;
sem_t sem1;
sem_t sem2;
pthread_t t1,t2;

static void traitant(int signo)
{
   fprintf(stderr,":) :) %lx est dans traitant :) :)\n",pthread_self());
   fprintf(stderr,"c'est le signal %d\n",signo);
   fini = 1;
   return;
}

void *addi(void * arg)
{
  sem_wait(&sem2);
  fprintf(stderr,"entree de t1 dans addi\n");
  pthread_kill(t2,SIGUSR1);
  fini = 0;
   while(!fini)
   {
     fprintf(stderr,"...\n");
     usleep(1000000);
   }
   if (fini)
     fprintf(stderr,"retour de t1 dans addi\n");
   fprintf(stderr,"sortie de t1 de addi : pthread_exit(1)\n");   
   pthread_exit((void *)1);   
}

void *multi(void * arg)
{   
  sem_wait(&sem1);
  fprintf(stderr,"entree de t2 dans multi\n");
  pthread_kill(t1,SIGUSR2);
  fini = 0;
  while(!fini)
    {
      fprintf(stderr,"...\n");
      usleep(1000000);
    }
  if (fini)
     fprintf(stderr,"retour de t2 dans multi\n");
   fprintf(stderr,"sortie de t2 de multi : pthread_exit(2)\n");   
   pthread_exit((void *)2);
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
      fprintf(stderr,"sigaction ok : traitant associe au signal %d\n",SIGUSR2);
   else 
      fprintf(stderr,"erreur sigaction\n");
   if (!sigaction(SIGUSR1,&act,NULL))
      fprintf(stderr,"sigaction ok : traitant associe au signal %d\n",SIGUSR1);
   else 
      fprintf(stderr,"erreur sigaction\n");

   pthread_attr_t attr;
   pthread_attr_init(&attr);
   sem_init(&sem1,0,0);
   sem_init(&sem2,0,0);

   fprintf(stderr,"attr initialisee\n");      

   fprintf(stderr,"t1 est cree avec attr\nil doit executer addi(t2)\n");
   pthread_create(&t1,&attr,addi,NULL);
   sem_post(&sem1);

   fprintf(stderr,"t2 est cree avec attr\nil doit executer multi(t1)\n");
   pthread_create(&t2,&attr,multi,NULL);
   sem_post(&sem2);
 
   pthread_join(t1,&value_ptr1);
   fprintf(stderr,"main join t1 : value_ptr1 = %d\n",(int)value_ptr1);
   pthread_join(t2,&value_ptr2);
   fprintf(stderr,"main join t2 : value_ptr2 = %d\n",(int)value_ptr2);

   return 0;
}

