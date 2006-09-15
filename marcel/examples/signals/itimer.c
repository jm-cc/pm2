#include<signal.h>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<errno.h>
#include<sys/time.h>

void traitant(int numero)
{
   if (numero == SIGALRM)
  	  fprintf(stderr,"SIGALRM reçu\n");
}

int main(void)
{
   struct sigaction action;
   action.sa_handler = traitant;
   sigisemptyset(&action.sa_mask);
   action.sa_flags = 0;
	action.sa_handler = traitant;
	if (sigaction(SIGALRM,&action,NULL) != 0)
   {
      fprintf(stderr,"erreur %d\n",errno);
      exit(-1);
   } 

   struct itimerval value;
   value.it_interval.tv_sec = 1;
   value.it_interval.tv_usec = 500000;
   value.it_value.tv_sec = 3;
   value.it_value.tv_usec = 0;

   setitimer(ITIMER_REAL,&value,NULL);

   int i = 0;
   
	while(i<5)
	{
      pause();
      i++;
   }

   getitimer(ITIMER_REAL,&value);

   fprintf(stderr,"get : value en usec : %ld\n", 1000000 * value.it_value.tv_sec + value.it_value.tv_usec);
   fprintf(stderr,"get : interval en usec : %ld\n", 1000000 * value.it_interval.tv_sec + value.it_interval.tv_usec);
  
   value.it_interval.tv_sec = 0;
   value.it_interval.tv_usec = 500000;
   value.it_value.tv_sec = 5;
   value.it_value.tv_usec = 0;
	
   struct itimerval ovalue;
   setitimer(ITIMER_REAL,&value,&ovalue);
   
    fprintf(stderr,"nv set ovalue : value en usec : %ld\n", 1000000 * ovalue.it_value.tv_sec + ovalue.it_value.tv_usec);
   fprintf(stderr,"nv set ovalue : interval en usec : %ld\n", 1000000 * ovalue.it_interval.tv_sec + ovalue.it_interval.tv_usec);
 
	while(i<10)
	{
      pause();
      i++;
   }

   return 0;
}
