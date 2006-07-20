#include <signal.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

/* s'envoie un signal raise trois fois */
volatile int fini;

static void traitant(int signo)
{
   fini ++;
   fprintf(stderr,"--- %d fois t1 dans traitant ---\n",fini);
   fprintf(stderr,"c'est le signal numero %d\n",signo);
   if (fini<3)
      raise(SIGUSR2);
   fprintf(stderr,"--- t1 sorti du traitant ---\n");
   return;
}

void *recsig(void * arg)
{   
   fini = 0;
   fprintf(stderr,"entree de t1 dans recsig -> raise(%d)\n",SIGUSR2);
   raise(SIGUSR2);
   fprintf(stderr,"sortie de t1 de recsig : pthread_exit(1)\n");   
   pthread_exit((void *)1);
}

int main(int argc, char *argv[])
{
   fprintf(stderr,"entree dans le main\n");
   void * value_ptr1;
   
   struct sigaction act;
   act.sa_handler = traitant;
   fprintf(stderr,"adresse traitant : %p\n",traitant);
   sigemptyset(&act.sa_mask);
   act.sa_flags = 0;
   act.sa_flags = SA_RESTART;
   if (!sigaction(SIGUSR2,&act,NULL))
      fprintf(stderr,"sigaction ok : traitant associe a %d\n",SIGUSR2);
   else
      fprintf(stderr,"!! erreur sigaction\n");
   
   pthread_t t1;
   pthread_attr_t attr;

   fprintf(stderr,"structure attr inititialisee\n");
   pthread_attr_init(&attr);
      
   fprintf(stderr,"t1 est cree avec attr : %p\nil doit executer recsig\n",&t1);
   pthread_create(&t1,&attr,recsig,NULL);

   pthread_join(t1,&value_ptr1);
   fprintf(stderr,"main join t1 : value_ptr1 = %d\n",(int)value_ptr1);
 
   return 0;
}

