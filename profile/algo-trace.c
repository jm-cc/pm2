

/* pseudo fonction pour trouver où écrire un événement dans les buffers */

/*
 * Structure du buffer de trace
 *
 * ADDR (a<b<c...)
 * ||
 * \/
 * 
 * a /--------------- 1ère zone
 *     struct record
 *        last_event = c
 *        start = a
 *        end = e
 *
 * b   ev1
 *
 *     ev2
 *
 *     ...
 *
 * c   evn
 *
 * d   FREE SPACE
 * 
 * e /--------------- 2ième zone
 *     struct record
 *        last_event = g
 *        start = e
 *        end = i
 *        
 * f   ev1
 *
 *     ...
 *
 * g   evn
 *
 * h   FREE SPACE
 *
 * i /--------------- 3ième zone
 *   ...
 * 
 * */


// Variables globales :
void *start_buffer;
void *pos_buffer;
void *end_buffer;

struct record {
	void* last_event;
	void* start;
	void* end;
}

// Variables par thread (ie faire toto=th_buff revient à faire un
// get_specifique ou équivalent):
__thread struct th_buff {
	void* pos; /* pointe sur le FREE SPACE correspondant */
	struct record* infos; /* pointe sur le record de ce thread */
} th_buff;

#define SIZE_BUFF 200 /* au hazard */



int write_ev(size) {
  
	th_buff old_th, new_th;
  
  restart:
 	/* lecture atomique des 2 valeurs (8 octets) */
 	read8(th_buff, old_th);

	new_th = old_th;
	new_th.pos += size;

	if (new_th.pos > new_th.infos->end) {
     		/* les infos ne tiennent pas dans la place du buffer 
		 * On cherche un nouveau buffer/record */
		void* old_pos = pos_buffer;
		
		void* new_pos = old_pos + SIZE_BUFF;

		if (new_pos > end_buffer) {
			/* On stoppe tout comme actuellement */
			...

			return -E_NOSPACELEFT;
		}
		if (!lock_cmpxchg(pos_buffer, old_pos, new_pos)) {
			/* Des choses ont changés, peut-être nous qui avons été
			 * interrompu et avons alors déjà réexécuté ce code
			 * (donc donné une nouvelle zone à ce thread */
			goto restart;
		}
		/* Ok, on est propriétaire de la nouvelle région qui va de
		 * old_pos à new_pos */
		
		/* On prévoit de la place pour :
		 * 1) les données de controle (struct record)
		 * 2) notre événement
		 * */
		new_th.pos=old_pos+sizeof(record_t)+size;
		
		*((struct record*)old_pos) = {
			.start=old_pos;
			.end=new_pos;
			.last_event=old_pos;
		};
		
		/* Sur ia32, on a cmpxchg8,
		 * mais sur ia64, on a seulement cmp4xchg8 (en fait cmp8xchg16
		 * vu qu'on est en 64 bits)
		 * J'écris donc l'algo avec le plus contraignant
		 *
		 * cmp4xchg8 : compare 4 octets, mais en cas d'égalité on en écrit 8
		 * */
		if (!cmp4xchg8(th_buff.pos, old_th.pos, new_th)) {
			/* Arghhh, on a été interrompu par une autre mesure qui
			 * a mis en place un nouveau buffer
			 * */
			if (lock_cmpxchg(pos_buffer, new_pos, old_pos)) {
				/* On redonne la zone qu'on vient d'allouer si
				 * c'est encore possible */
				goto restart;
			}
			/* Bon, ben on a une zone avec une mesure et pas grand
			 * chose d'autre à mettre dedans...
			 * Tant pis
			 *
			 * cas (*) (voir TODO)
			 * */
		}
	}
	/* Écriture des données à l'adresse :
	 * new_th.pos - size
	 * */
	...
  restart_update:
	/* Et mise à jour du pointeur sur la dernière écriture de la zone */
	int old_last_ev=new_th.infos->last_event;
	if (new_th.pos > old_last_ev) {
		if (!cmpxchg(new_th.infos->last_event, old_last_ev, new_th.pos)) {
			goto restart_update;
		}
	}
	return 0;
}


TODO :
* Gérer les FREE SPACE abandonnées
  En faire une liste chaînée
    - ajout dans cette fonction dans le cas (*)
    - ajout après la terminaison d'un thread qui laisse un grand FREE SPACE
    - consommation dans cette fonction avant de tenter d'allouer une nouvelle zone
      dans le buffer global

* Dumper tous les ev sur disque

* Faire en sorte que les trois variables globales et les deux variables par thread
  soit en fait des tableaux de variables indexé par la QBB courrante
  (on peut se planter de QBB parfois si on change de QBB entre le début de la
   fonction où on se renseigne et la fin de la fonction, mais ce n'est pas
   grave (juste une légère perte de perf))

* BEAUCOUP plus dur :
  faire en sorte que le buffer principal puisse être libéré/savé/changé en
  cours d'exécution

REMARQUE :
* en mettant le PID du thread dans la structure record, on n'a plus besoin de
  le mettre dans chaque événement :-)
 
