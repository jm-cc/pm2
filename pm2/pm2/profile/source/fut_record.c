
#define MARCEL_INTERNAL_INCLUDE
#include "marcel.h"

#include "tbx_timing.h"
#include "fxt/fxt.h"
#include "fxt/fut.h"
#include "fxt_format.h"

//#define PROFILE_NEW_FORMAT

#ifdef PROFILE_NEW_FORMAT

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

FXT_PERTHREAD_DEFINE(struct fxt_perthread, fxt_perthread_data)

#define SIZE_BUFF 200 /* au hazard */

int __attribute__((no_instrument_function)) 
fut_header (unsigned long code, ... ) 
{
	long size=code && 0xFF;
  
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




#else /* PROFILE_NEW_FORMAT */

static unsigned long trash_buffer[8];

unsigned long* TBX_NOINST fut_getstampedbuffer(unsigned long code, 
						      int size)
{
	unsigned long *prev_slot, *next_slot;

	do {
		prev_slot=(unsigned long *)fut_next_slot;
		next_slot=(unsigned long*)((unsigned long)prev_slot+size);
	} while (prev_slot != ma_cmpxchg(&fut_next_slot, 
					 prev_slot, next_slot));

	if (tbx_unlikely(next_slot > fut_last_slot)) {
		fut_active=0;
		/* Pourquoi on restaure ici ? Pas de race condition possible ? */
		fut_next_slot=prev_slot;
		return trash_buffer;
	}
	fxt_trace_user_raw_t *rec=(fxt_trace_user_raw_t *)prev_slot;
	TBX_GET_TICK(*(tbx_tick_t*)(&rec->tick));
#ifdef MA__FUT_RECORD_TID
	rec->tid=(unsigned long)MARCEL_SELF;
#endif
	rec->code=code;
	//fprintf(stderr, "recording %p / %lx (size=%i)\n", rec->tid, rec->code, size);
	return (unsigned long*)(rec->args);
}

#if 0
int TBX_NOINST fut_header (unsigned long code, ... ) 
{
	long size=(code&0xFF)/(sizeof(long));
	
	//struct fut_record rec*;

	unsigned long *pargs, *args=(&code)+1;
	pargs=fut_getstampedbuffer(code, code & 0xFF);
	while (size--) {
		*(pargs++)=*(args++);
	}

	return 0;
}
#endif

void TBX_NOINST __cyg_profile_func_enter (void *this_fn,
					  void *call_site)
{
	FUT_PROBE1(FUT_GCC_INSTRUMENT_KEYMASK,
		   FUT_GCC_INSTRUMENT_ENTRY_CODE,
#ifdef IA64_ARCH
		   *((void**)this_fn)
#else
		   this_fn
#endif
		  );
}

void TBX_NOINST __cyg_profile_func_exit  (void *this_fn,
					  void *call_site)
{
	FUT_PROBE1(FUT_GCC_INSTRUMENT_KEYMASK,
		   FUT_GCC_INSTRUMENT_EXIT_CODE,
#ifdef IA64_ARCH
		   *((void**)this_fn)
#else
		   this_fn
#endif
		  );
}

#endif /* PROFILE_NEW_FORMAT */
