
#include <stdio.h>
#include <unistd.h>
#include <termios.h>

#include "marcel.h"

struct termios tinit, tchg;

void passer_en_mode_interactif()
{
   tcgetattr(0, &tinit);
   tchg = tinit;
   tchg.c_iflag &= ~(IXON|IXOFF);
   tchg.c_lflag &= ~(ICANON|ECHO|ISIG);
   tchg.c_cc[VMIN]=1;
   tcsetattr(0, TCSANOW, &tchg);
}

void repasser_en_mode_normal()
{
   tcsetattr(0, TCSANOW, &tinit);
}

any_t ma_fonction(any_t arg)
{
   while(1) {
      printf("%s ", (char *)arg); fflush(stdout);
      { int j = 500000; while(j--); } /* On passe son temps comme on peut... */
   }
}

int marcel_main(int argc, char *argv[])
{ 
  marcel_t pid; /* Identificateur de processus leger */
  any_t status; /* Code de retour */

   passer_en_mode_interactif();

   /* Initialisation de la librairie de gestion des processus legers */
   marcel_init(&argc, argv);

   /* Creation d'un "thread" qui va executer ma_fonction("Hello") */
   marcel_create(&pid, NULL, ma_fonction, "Hello !");

   /* On attend une touche au clavier, mais pendant ce temps, 
      le "thread" s'execute tranquillement... */
   getchar();

   /* Ici, on va tuer le thread. Je sais, c'est pas beau. Mais rien de vous
      empeche de programmer une solution utilisant une variable globale ou 
      encore d'envoyer un signal au thread, mais ceci est une autre histoire... */
   marcel_cancel(pid);

   /* On attend la fin du "thread" pour sortir proprement */
   marcel_join(pid, &status);

   printf("\nThread %p completed.\n", pid);

   /* Au revoir les threads... */
/*   marcel_end(); */

   repasser_en_mode_normal();
   return 0;
}
