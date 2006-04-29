#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <fxt/fut.h>
#include <fxt/fxt.h>
#include <fxt/fxt-tools.h>

//---------------------------------------------------------------

#include <stdint.h>
#include <inttypes.h>

struct fxt_code_name fut_code_table [] =
{
#include "fut_print.h"
   {0, NULL }
};

// ---------------------------------------------------------------
#include "read_trace.h"

//#define DEBUG_VERB2

/* la fonction qui analyse chaque événement et la met dans une structure événement */

ev_t set_etat_ev( ev_t evt, int * ad_nb_levels, fxt_blockev_t blockev_machin,
				  int ev_type, struct fxt_ev ev,int * leveltab, int mode)
{
   //int i;
   int retour = fxt_next_ev(blockev_machin, ev_type, &ev);

   /* initialisation */
   evt.code_ev = 0;
   evt.fin_trace = 0;
   evt.id_thread = -1;
   evt.level_rq = -2;
   evt.taille_lv = -1;
   evt.nb_levels = -1;
   /*                */

   if (retour != 0)
   {
	  /* on est a la fin de la trace */
	  evt.fin_trace = 1;
	  return evt;
   }
  
   if (mode == ANALYSE)
   {
	  my_printf("\n*************************\n\n");

	  evt.code_ev = ev.ev64.code;
	  my_printf("Code evmnt      %p\n",evt.code_ev);
	  evt.time = ev.ev64.time;  
	  my_printf("Time            %16llu\n",evt.time);

	  my_printf("\n");

	  /* pour chaque différent code, quelques champs sont mis à jour */

	  switch (evt.code_ev) 
      {
         case BUBBLE_SCHED_NEW: { /* champ adr_bulle */
            my_printf("BUBBLE_SCHED_NEW\n\n");
            evt.adr_bulle = ev.ev64.param[0];
            my_printf("adresse bulle %llx\n",evt.adr_bulle);
            break;
         }
         case BUBBLE_SCHED_CLOSE: {		
            my_printf("BUBBLE_SCHED_CLOSE\n");
            break;
         }
         case BUBBLE_SCHED_CLOSING: {
            my_printf("BUBBLE_SCHED_CLOSING\n");
            break;
         }
         case BUBBLE_SCHED_CLOSED: {
            my_printf("BUBBLE_SCHED_CLOSED\n");
            break;
         }
         case BUBBLE_SCHED_SWITCHRQ: { /* champs adr_bulle,adr_rq */
            my_printf("BUBBLE_SCHED_SWITCHRQ\n\n");
            evt.adr_bulle = ev.ev64.param[0];
            my_printf("adresse   bulle %llx\n",evt.adr_bulle);
            evt.adr_rq = ev.ev64.param[1];
            my_printf("adresse   runqu %llx\n",evt.adr_rq);
            break;
         }
         case BUBBLE_SCHED_EXPLODE: {
            my_printf("BUBBLE_SCHED_EXPLODE\n");
            break;
         }
         case BUBBLE_SCHED_GOINGBACK: {
            my_printf("BUBBLE_SCHED_GOINGBACK\n");
            break;
         }
         case BUBBLE_SCHED_INSERT_BUBBLE: { /* champs adr_bulle,mere_bulle */
            my_printf("BUBBLE_SCHED_INSERT_BUBBLE\n\n");
            evt.adr_bulle = ev.ev64.param[0];
            my_printf("adresse   bulle %llx\n",evt.adr_bulle);
            evt.mere_bulle = ev.ev64.param[1];
            my_printf("adresse   bmere %llx\n",evt.mere_bulle);
            break;
         }
         case BUBBLE_SCHED_INSERT_THREAD: {/* champs adr_thread,mere_bulle */
            my_printf("BUBBLE_SCHED_INSERT_THREAD\n\n");
            evt.adr_thread = ev.ev64.param[0];
            my_printf("adresse   thd   %llx\n",evt.adr_thread);
            evt.mere_bulle = ev.ev64.param[1];
            my_printf("adresse   bmere %llx\n",evt.mere_bulle);
            break;
         }
         case BUBBLE_SCHED_WAKE: {/* champs adr_bulle,adr_rq */
            my_printf("BUBBLE_SCHED_WAKE\n\n");
            evt.adr_bulle = ev.ev64.param[0];
            my_printf("adresse   bulle %llx\n",evt.adr_bulle);
            evt.adr_rq = ev.ev64.param[1];
            my_printf("adresse   runqu %llx\n",evt.adr_rq);
            break;
         }
         case BUBBLE_SCHED_JOIN: {
            my_printf("BUBBLE_SCHED_JOIN\n");
            break;
         }
         case FUT_RQS_NEWLEVEL: {/* champs nb_levels,level_rq,taille_lv, place_rq */
            my_printf("FUT_RQS_NEWLEVEL\n\n");
            (*ad_nb_levels) ++;
            evt.nb_levels = (*ad_nb_levels);
            my_printf("nb levels     %d  \n",evt.nb_levels);
            evt.level_rq = evt.nb_levels - 1;
            my_printf("level crée    %d  \n",evt.level_rq);
            evt.taille_lv = ev.ev64.param[0];
            my_printf("taille level  %d  \n",evt.taille_lv);
	  
            break;
         }
         case FUT_RQS_NEWLWPRQ: {/* champs adr_rq,level_rq,taille_lv, place_rq */
            my_printf("FUT_RQS_NEWLWPRQ\n\n");
            evt.adr_rq = ev.ev64.param[1];
            my_printf("adresse runqu   %llx\n",evt.adr_rq);
            evt.level_rq = (*ad_nb_levels) - 1;
            my_printf("level newrq     %d  \n",evt.level_rq);
            evt.place_rq = ev.ev64.param[0];
            my_printf("place level     %d  \n",evt.place_rq);
            break;
         }
         case FUT_RQS_NEWRQ: {/* champs level_rq, adr_rq */ 
            my_printf("FUT_RQS_NEWRQ\n\n");
            evt.level_rq = ev.ev64.param[0];
            evt.adr_rq = ev.ev64.param[1];
            if (evt.level_rq == -1)
			{
               my_printf("rq stockage lvl -1  \n");
			}
            else
			{
               my_printf("level newrq?    %d  \n",evt.level_rq);
               evt.place_rq = leveltab[evt.level_rq];
               my_printf("place level     %d  \n",evt.place_rq);
               leveltab[evt.level_rq] ++;
			}
            my_printf("adresse runqu   %llx\n",evt.adr_rq);
            break;
         }
         case FUT_SETUP_CODE:
         {
			my_printf("FUT_SETUP_CODE\n");
			break;
         }
         case FUT_RESET_CODE:
         {
			my_printf("FUT_RESET_CODE\n");
			break;
         }
		  
         case FUT_CALIBRATE0_CODE:
         {
			my_printf("FUT_CALIBRATE0_CODE\n");
			break;
         }
		  
         case FUT_CALIBRATE1_CODE:
         {
			my_printf("FUT_CALIBRATE1_CODE\n");
			break;
         }
		  
         case FUT_CALIBRATE2_CODE:
         {
			my_printf("FUT_CALIBRATE2_CODE\n");
			break;
         }
		  
         case FUT_NEW_LWP_CODE:
         {
			my_printf("FUT_NEW_LWP_CODE\n");
			my_printf("params      %d\n",ev.ev64.nb_params);
			my_printf("param[0]    %d\n",ev.ev64.param[0]);
			my_printf("param[1]    %llx\n",ev.ev64.param[1]);
			break;
         }
		  
         case FUT_GCC_INSTRUMENT_ENTRY_CODE:
         {
			my_printf("FUT_GCC_INSTRUMENT_ENTRY_CODE\n");
			break;
         }
		  
         case FUT_GCC_INSTRUMENT_EXIT_CODE:
         {
			my_printf("FUT_GCC_INSTRUMENT_EXIT_CODE\n");
			break;
         }

         case SCHED_IDLE_START: {
            my_printf("SCHED_IDLE_START\n");
            break;
         }
         case SCHED_IDLE_STOP: {
            my_printf("SCHED_IDLE_START\n");
            break;
         }
         case FUT_START_PLAYING: {
            my_printf("FUT_START_PLAYING\n");
            break;
         }
         case FUT_THREAD_BIRTH_CODE: { /* champ adr_thread */
            my_printf("FUT_THREAD_BIRTH_CODE\n\n");
            evt.adr_thread = ev.ev64.param[0];
            my_printf("adresse thd     %llx\n",evt.adr_thread);
            break;
         }
         case FUT_THREAD_DEATH_CODE: { /* champ adr_thread */
            evt.adr_thread = ev.ev64.param[0];
            my_printf("adresse thd     %llx\n",evt.adr_thread);
            break;
         }
         case SET_THREAD_ID: { /*champ adr_tread,id_thread */
            my_printf("SET_THREAD_ID\n\n");
            evt.adr_thread = ev.ev64.param[0];
            my_printf("adr thread      %llx\n",evt.adr_thread);
            evt.id_thread = ev.ev64.param[1];
            my_printf("id thread       %d\n",evt.id_thread);
            break;
         }
         case FUT_SET_THREAD_NAME_CODE: { /* champ adr_thread, nom_thread, id_thread */
            my_printf("FUT_THREAD_NAME_CODE\n\n");
            evt.adr_thread = ev.ev64.param[0];
            my_printf("adresse thd     %llx\n",evt.adr_thread);
            char name[16];
            uint32_t *ptr = (uint32_t *) name;
            ptr[0] = ev.ev64.param[1];
            ptr[1] = ev.ev64.param[2];
            ptr[2] = ev.ev64.param[3];
            ptr[3] = ev.ev64.param[4];
            name[15] = 0;
            evt.nom_thread = strdup(name);
            my_printf("nom thread      %s\n",evt.nom_thread);
            break;
         }
         case SCHED_SETPRIO: { /* champ adr_thread, adr_bulle, prio */
            my_printf("SCHED_SETPRIO\n\n");
            evt.adr_thread = ev.ev64.param[0];
            evt.adr_bulle = ev.ev64.param[0];
            my_printf("adr entité      %llx\n",evt.adr_thread);
            evt.prio = ev.ev64.param[1];
            my_printf("priorité        %d\n",evt.prio);
            break;
         }
         case SCHED_TICK: {
            my_printf("SCHED_TICK\n");
            break;
         }
         case SCHED_THREAD_BLOCKED: { /* champ adr_thread */
            my_printf("SCHED_THREAD_BLOCKED\n\n");
            evt.adr_thread = ev.ev64.user.tid;
            my_printf("thread bloqué   %llx\n",evt.adr_thread);
            break;
         }
         case SCHED_THREAD_WAKE: { /* champs adr_thread, next_thread */
            my_printf("SCHED_THREAD_WAKE\n\n");
            evt.adr_thread = ev.ev64.user.tid;
            my_printf("thread aslept   %llx\n",evt.adr_thread);
            evt.next_thread = ev.ev64.param[0];
            my_printf("thread waken    %llx\n",evt.next_thread);
            break;
         }
         case SCHED_RESCHED_LWP: {
            my_printf("SCHED_RESCHED_LWP\n");
            break;	
         }
         case FUT_SWITCH_TO_CODE: { /* champs adr_thread, next_thread */
            my_printf("FUT_SWITCH_TO_CODE\n\n");
            evt.adr_thread = ev.ev64.user.tid;
            my_printf("thread aslept   %llx\n",evt.adr_thread);
            evt.next_thread = ev.ev64.param[0];
            my_printf("thread waken    %llx\n",evt.next_thread);
            //evt.id_thread = ev.ev64.param[1];
            break;
         }
				  /*
         case MARCEL_CREATE_INTERNAL_ENTRY: {
            my_printf("MARCEL_CREATE_INTERNAL_ENTRY\n");
            break;	
         }
         case MARCEL_CREATE_INTERNAL_EXIT: {
            my_printf("MARCEL_CREATE_INTERNAL_EXIT\n");
            break;	
         } 
         case MARCEL_SLOT_ALLOC_ENTRY: {
            my_printf("MARCEL_SLOT_ALLOC_ENTRY\n");
            break;
         }
         case MARCEL_SLOT_ALLOC_EXIT: {
            my_printf("MARCEL_SLOT_ALLOC_EXIT\n");
            break;
         }
         case THREAD_STACK_ALLOCATED: {
            my_printf("THREAD_STACK_ALLOCATED\n");
            break;
         }
         case MARCEL_SCHED_INTERNAL_INIT_MARCEL_THREAD_ENTRY: {
            my_printf("MARCEL_SCHED_INTERNAL_INIT_MARCEL_THREAD_ENTRY\n");
            break;
         }
         case MARCEL_SCHED_INTERNAL_INIT_MARCEL_THREAD_EXIT: {
            my_printf("MARCEL_SCHED_INTERNAL_INIT_MARCEL_THREAD_EXIT\n");
            break;
         }  
         case MARCEL_POSTEXIT_INTERNAL_ENTRY: {
            my_printf("MARCEL_POXTEXIT_INTERNAL_ENTRY\n");
            break;
         }  
         case MARCEL_POSTEXIT_INTERNAL_EXIT: {
            my_printf("MARCEL_POXTEXIT_INTERNAL_EXIT\n");
            break;
         }   
         case MARCEL_SCHED_CREATE_ENTRY: {
            my_printf("MARCEL_SCHED_CREATE_ENTRY\n");
            break;
         } 
         case MARCEL_SCHED_CREATE_EXIT: {
            my_printf("MARCEL_SCHED_CREATE_EXIT\n");
            break;
         } 
         case MARCEL_SCHED_INTERNAL_CREATE_ENTRY: {
            my_printf("MARCEL_SCHED_INTERNAL_CREATE_ENTRY\n");
            break;
         } 
         case MARCEL_SCHED_INTERNAL_CREATE_EXIT: {
            my_printf("MARCEL_SCHED_INTERNAL_CREATE_EXIT\n");
            break;
         }  
         case MARCEL_SCHED_INTERNAL_CREATE_DONTSTART_ENTRY: {
            my_printf("MARCEL_SCHED_INTERNAL_CREATE_DONTSTART_ENTRY\n");
            break;
         }    
         case MARCEL_SCHED_INTERNAL_CREATE_DONTSTART_EXIT: {
            my_printf("MARCEL_SCHED_INTERNAL_CREATE_DONTSTART_EXIT\n");
            break;
         }    
         case MARCEL_WAKE_UP_CREATED_THREAD_ENTRY: {
            my_printf("MARCEL_WAKE_UP_CREATED_THREAD_ENTRY\n");
            break;
         }    
         case MARCEL_WAKE_UP_CREATED_THREAD_EXIT: {
            my_printf("MARCEL_WAKE_UP_CREATED_THREAD_EXIT\n");
            break;
         }
         case MARCEL_BUBBLE_INSERTENTITY_ENTRY: {
            my_printf("MARCEL_BUBBLE_INSERTENTITY_ENTRY\n");
            break;
         }    
         case MARCEL_BUBBLE_INSERTENTITY_EXIT: {
            my_printf("MARCEL_BUBBLE_INSERTENTITY_EXIT\n");
            break;
         } 
         case MARCEL_SEM_P_ENTRY: {
            my_printf("MARCEL_SEM_P_ENTRY\n");
            break;
         }    
         case MARCEL_SEM_P_EXIT: {
            my_printf("MARCEL_SEM_P_EXIT\n");
            break;
         }     
         case MARCEL_WAKE_UP_BUBBLE_ENTRY: {
            my_printf("MARCEL_WAKE_UP_BUBBLE_ENTRY\n");
            break;
         }    
         case MARCEL_WAKE_UP_BUBBLE_EXIT: {
            my_printf("MARCEL_WAKE_UP_BUBBLE_EXIT\n");
            break;
         }      
					  */
         default:
            break;
      }
   }
   else
   {
	  if (ev.ev64.code == FUT_RQS_NEWLEVEL)
         (*ad_nb_levels) ++;
   }
   return evt;
}

/* lire trace */

inf_t lire_trace(char * fichier_trace)
{
   fxt_t trace;
   fxt_blockev_t blockev_trace;
   trace = fxt_open(fichier_trace);

   if (trace == NULL)
   {
      my_printf("oh oh, ouverture de trace foirée\n");
      exit(1);
   }
  
   struct fxt_ev_native ev_native;
   struct fxt_ev_32 ev_32;
   struct fxt_ev_64 ev_64;
   struct fxt_ev ev;
   int ev_type = FXT_EV_TYPE_64;

   ev.native = ev_native;
   ev.ev32 = ev_32;
   ev.ev64 = ev_64;

   blockev_trace = fxt_blockev_enter(trace); 

   ev_t evt;
   inf_t infos_trace;
   infos_trace.nb_evts = 0;
   infos_trace.nb_levels = 0;

   do 
   {
	  evt = set_etat_ev(evt, &infos_trace.nb_levels, blockev_trace, ev_type, ev, NULL, PARCOURS);
	  infos_trace.nb_evts ++;
   }
   while (evt.fin_trace != 1);

   return infos_trace;
}

/* la fonction qui analyse la trace et la met dans ton tableau de structure événement*/

ev_t * analyser_trace(char * fichier_trace,ev_t * tracetab,int * leveltab)
{

   fxt_t trace;
   fxt_blockev_t blockev_trace;
   trace = fxt_open(fichier_trace);
  
   struct fxt_ev_native ev_native;
   struct fxt_ev_32 ev_32;
   struct fxt_ev_64 ev_64;
   struct fxt_ev ev;
   int ev_type = FXT_EV_TYPE_64;

   ev.native = ev_native;
   ev.ev32 = ev_32;
   ev.ev64 = ev_64;

   blockev_trace = fxt_blockev_enter(trace); 

   int num_evt_i = 0;
   int nb_levels = 0;
  
   num_evt_i = 0;

   do 
   {
	  tracetab[num_evt_i] = set_etat_ev(tracetab[num_evt_i],&nb_levels,blockev_trace,ev_type,ev,leveltab,ANALYSE);
	  num_evt_i ++;
   }
   while (tracetab[num_evt_i - 1].fin_trace != 1);


   return tracetab;
}

/*--------------------------------accesseurs----------------------------------*/

int GetCode_Ev(ev_t * adrev)
{
   int code;
   code = adrev->code_ev;
   switch (code) 
   {
      case BUBBLE_SCHED_NEW: { /* champ adr_bulle */
         my_printf_2("BUBBLE_SCHED_NEW\n");
         my_printf_2("GetAdr_Bulle_Ev\n");
         break;
      }
      case BUBBLE_SCHED_CLOSE: {		
         my_printf_2("BUBBLE_SCHED_CLOSE\n");
         my_printf_2("rien à faire\n");
         break;
      }
      case BUBBLE_SCHED_CLOSING: {
         my_printf_2("BUBBLE_SCHED_CLOSING\n");
         my_printf_2("rien à faire\n");
         break;
      }
      case BUBBLE_SCHED_CLOSED: {
         my_printf_2("BUBBLE_SCHED_CLOSED\n");
         my_printf_2("rien à faire\n");
         break;
      }
      case BUBBLE_SCHED_SWITCHRQ: { /* champs adr_bulle,adr_rq */
         my_printf_2("BUBBLE_SCHED_SWITCHRQ\n");
         my_printf_2("GetAdr_Bulle_Ev\n");
         my_printf_2("GetAdr_Rq_Ev\n");
         break;
      }
      case BUBBLE_SCHED_EXPLODE: {
         my_printf_2("BUBBLE_SCHED_EXPLODE\n");
         my_printf_2("rien à faire\n");
         break;
      }
      case BUBBLE_SCHED_GOINGBACK: {
         my_printf_2("BUBBLE_SCHED_GOINGBACK\n");
         my_printf_2("rien à faire\n");
         break;
      }
      case BUBBLE_SCHED_INSERT_BUBBLE: { /* champs adr_bulle,mere_bulle */
         my_printf_2("BUBBLE_SCHED_INSERT_BUBBLE\n");
         my_printf_2("GetAdr_Bulle_Ev\n");
         my_printf_2("GetMere_Bulle_Ev\n");
         break;
      }
      case BUBBLE_SCHED_INSERT_THREAD: {/* champs adr_thread,mere_bulle */
         my_printf_2("BUBBLE_SCHED_INSERT_THREAD\n");
         my_printf_2("GetAdr_Thread_Ev\n");
         my_printf_2("GetMere_Bulle_Ev\n");
         break;
      }
      case BUBBLE_SCHED_WAKE: {/* champs adr_bulle,adr_rq */
         my_printf_2("BUBBLE_SCHED_WAKE\n");
         my_printf_2("GetAdr_Bulle_Ev\n");
         my_printf_2("GetAdr_Rq_Ev\n");
         break;
      }
      case BUBBLE_SCHED_JOIN: {
         my_printf_2("BUBBLE_SCHED_JOIN\n");
         my_printf_2("rien à faire\n");
         break;
      }
      case FUT_RQS_NEWLEVEL: {/* champs nb_levels,level_rq,taille_lv*/
         my_printf_2("FUT_RQS_NEWLEVEL\n");
         my_printf_2("GetNombre_Lvs_Ev\n");
         my_printf_2("GetLevel_Rq_Ev\n");
         my_printf_2("GetTaille_Lv_Ev\n");
         break;
      }
      case FUT_RQS_NEWLWPRQ: {/* champs adr_rq,level_rq place_rq */
         my_printf_2("FUT_RQS_NEWLWPRQ\n");
         my_printf_2("GetAdr_Rq_Ev\n");
         my_printf_2("GetLevel_Rq_Ev\n");
         my_printf_2("GetPlace_Rq_Ev\n");
         break;
      }
      case FUT_RQS_NEWRQ: {/* champs level_rq, adr_rq,place level */ 
         my_printf_2("FUT_RQS_NEWRQ\n");
         my_printf_2("GetAdr_Rq_Ev\n");
         my_printf_2("GetLevel_Rq_Ev\n");
         my_printf_2("GetPlace_Rq_Ev si Level_Rq != -1\n");
         break;
      }
      case FUT_SETUP_CODE:
      {
         my_printf_2("FUT_SETUP_CODE\n");
         my_printf_2("rien à faire\n");
         break;
      }
      case FUT_RESET_CODE:
      {
         my_printf_2("FUT_RESET_CODE\n");
         my_printf_2("rien à faire\n");
         break;
      }
		  
      case FUT_CALIBRATE0_CODE:
      {
         my_printf_2("FUT_CALIBRATE0_CODE\n");
         my_printf_2("rien à faire\n");
         break;
      }
		  
      case FUT_CALIBRATE1_CODE:
      {
         my_printf_2("FUT_CALIBRATE1_CODE\n");
         my_printf_2("rien à faire\n");
         break;
      }
		  
      case FUT_CALIBRATE2_CODE:
      {
         my_printf_2("FUT_CALIBRATE2_CODE\n");
         my_printf_2("rien à faire\n");
         break;
      }
		  
      case FUT_NEW_LWP_CODE:
      {
         my_printf_2("FUT_NEW_LWP_CODE\n");
         my_printf_2("rien à faire\n");
         break;
      }
		  
      case FUT_GCC_INSTRUMENT_ENTRY_CODE:
      {
         my_printf_2("FUT_GCC_INSTRUMENT_ENTRY_CODE\n");
         my_printf_2("rien à faire\n");
         break;
      }
		  
      case FUT_GCC_INSTRUMENT_EXIT_CODE:
      {
         my_printf_2("FUT_GCC_INSTRUMENT_EXIT_CODE\n");
         my_printf_2("rien à faire\n");
         break;
      }

      case SCHED_IDLE_START: {
         my_printf_2("SCHED_IDLE_START\n");
         my_printf_2("rien à faire\n");
         break;
      }
      case SCHED_IDLE_STOP: {
         my_printf_2("SCHED_IDLE_START\n");
         my_printf_2("rien à faire\n");
         break;
      }
      case FUT_START_PLAYING: {
         my_printf_2("FUT_START_PLAYING\n");
         my_printf_2("rien à faire\n");
         break;
      }
      case FUT_THREAD_BIRTH_CODE: { /* champ adr_thread */
         my_printf_2("FUT_THREAD_BIRTH_CODE\n");
         my_printf_2("GetAdr_Thread_Ev\n");
         break;
      }
      case FUT_THREAD_DEATH_CODE: { /* champ adr_thread */
         my_printf_2("FUT_THREAD_DEATH_CODE\n");
         my_printf_2("GetAdr_Rq_Ev si tu veux exprimer la mort\n");
         break;
      }
      case SET_THREAD_ID: { /*champ adr_tread,id_thread */
         my_printf_2("SET_THREAD_ID\n");
         my_printf_2("GetAdr_Thread_Ev\n");
         my_printf_2("GetId_Thread_Ev\n");
         break;
      }
      case FUT_SET_THREAD_NAME_CODE: { /* champ adr_thread, nom_thread, id_thread */
         my_printf_2("FUT_THREAD_NAME_CODE\n");
         my_printf_2("GetAdr_Thread_Ev\n");
         my_printf_2("GetNom_Thread_Ev\n");
         break;
      }
      case SCHED_SETPRIO: { /* champ adr_thread, adr_bulle, prio */
         my_printf_2("SCHED_SETPRIO\n");
         my_printf_2("GetAdr_Thread_Ev\n");
         my_printf_2("GetAdr_Bulle_Ev\n");
         my_printf_2("GetPrio_Ev\n");
         break;
      }
      case SCHED_TICK: {
         my_printf_2("SCHED_TICK\n");
         my_printf_2("rien à faire\n");
         break;
      }
      case SCHED_THREAD_BLOCKED: { /* champ adr_thread */
         my_printf_2("SCHED_THREAD_BLOCKED\n");
         my_printf_2("GetAdr_Thread_Ev, à voir quoi faire avec\n");
         break;
      }
      case SCHED_THREAD_WAKE: { /* champs adr_thread, next_thread */
         my_printf_2("SCHED_THREAD_WAKE\n");
         my_printf_2("GetAdr_Thread_Ev\n");
         my_printf_2("GetNext_Thread_Ev\n");
         break;
      }
      case SCHED_RESCHED_LWP: {
         my_printf_2("SCHED_RESCHED_LWP\n");
         my_printf_2("rien à faire\n");
         break;	
      }
      case FUT_SWITCH_TO_CODE: { /* champs adr_thread, next_thread */
         my_printf_2("FUT_SWITCH_TO_CODE\n");
         my_printf_2("GetAdr_Thread_Ev\n");
         my_printf_2("GetNext_Thread_Ev\n");
         break;
      }
			       /*
      case MARCEL_CREATE_INTERNAL_ENTRY: {
         my_printf_2("MARCEL_CREATE_INTERNAL_ENTRY\n");
         my_printf_2("rien à faire\n");
         break;	
      }
      case MARCEL_CREATE_INTERNAL_EXIT: {
         my_printf_2("MARCEL_CREATE_INTERNAL_EXIT\n");
         my_printf_2("rien à faire\n");
         break;	
      } 
      case MARCEL_SLOT_ALLOC_ENTRY: {
         my_printf_2("MARCEL_SLOT_ALLOC_ENTRY\n");
         my_printf_2("rien à faire\n");
         break;
      }
      case MARCEL_SLOT_ALLOC_EXIT: {
         my_printf_2("MARCEL_SLOT_ALLOC_EXIT\n");
         my_printf_2("rien à faire\n");
         break;
      }
      case THREAD_STACK_ALLOCATED: {
         my_printf_2("THREAD_STACK_ALLOCATED\n");
         my_printf_2("rien à faire\n");
         break;
      }
      case MARCEL_SCHED_INTERNAL_INIT_MARCEL_THREAD_ENTRY: {
         my_printf_2("MARCEL_SCHED_INTERNAL_INIT_MARCEL_THREAD_ENTRY\n");
         my_printf_2("rien à faire\n");
         break;
      }
      case MARCEL_SCHED_INTERNAL_INIT_MARCEL_THREAD_EXIT: {
         my_printf_2("MARCEL_SCHED_INTERNAL_INIT_MARCEL_THREAD_EXIT\n");
         my_printf_2("rien à faire\n");
         break;
      }  
      case MARCEL_POSTEXIT_INTERNAL_ENTRY: {
         my_printf_2("MARCEL_POXTEXIT_INTERNAL_ENTRY\n");
         my_printf_2("rien à faire\n");
         break;
      }  
      case MARCEL_POSTEXIT_INTERNAL_EXIT: {
         my_printf_2("MARCEL_POXTEXIT_INTERNAL_EXIT\n");
         my_printf_2("rien à faire\n");
         break;
      }   
      case MARCEL_SCHED_CREATE_ENTRY: {
         my_printf_2("MARCEL_SCHED_CREATE_ENTRY\n");
         my_printf_2("rien à faire\n");
         break;
      } 
      case MARCEL_SCHED_CREATE_EXIT: {
         my_printf_2("MARCEL_SCHED_CREATE_EXIT\n");
         my_printf_2("rien à faire\n");
         break;
      } 
      case MARCEL_SCHED_INTERNAL_CREATE_ENTRY: {
         my_printf_2("MARCEL_SCHED_INTERNAL_CREATE_ENTRY\n");
         my_printf_2("rien à faire\n");
         break;
      } 
      case MARCEL_SCHED_INTERNAL_CREATE_EXIT: {
         my_printf_2("MARCEL_SCHED_INTERNAL_CREATE_EXIT\n");
         my_printf_2("rien à faire\n");
         break;
      }  
      case MARCEL_SCHED_INTERNAL_CREATE_DONTSTART_ENTRY: {
         my_printf_2("MARCEL_SCHED_INTERNAL_CREATE_DONTSTART_ENTRY\n");
         my_printf_2("rien à faire\n");
         break;
      }    
      case MARCEL_SCHED_INTERNAL_CREATE_DONTSTART_EXIT: {
         my_printf_2("MARCEL_SCHED_INTERNAL_CREATE_DONTSTART_EXIT\n");
         my_printf_2("rien à faire\n");
         break;
      }    
      case MARCEL_WAKE_UP_CREATED_THREAD_ENTRY: {
         my_printf_2("MARCEL_WAKE_UP_CREATED_THREAD_ENTRY\n");
         my_printf_2("rien à faire\n");
         break;
      }    
      case MARCEL_WAKE_UP_CREATED_THREAD_EXIT: {
         my_printf_2("MARCEL_WAKE_UP_CREATED_THREAD_EXIT\n");
         my_printf_2("rien à faire\n");
         break;
      }
      case MARCEL_BUBBLE_INSERTENTITY_ENTRY: {
         my_printf_2("MARCEL_BUBBLE_INSERTENTITY_ENTRY\n");
         my_printf_2("rien à faire\n");
         break;
      }    
      case MARCEL_BUBBLE_INSERTENTITY_EXIT: {
         my_printf_2("MARCEL_BUBBLE_INSERTENTITY_EXIT\n");
         my_printf_2("rien à faire\n");
         break;
      } 
      case MARCEL_SEM_P_ENTRY: {
         my_printf_2("MARCEL_SEM_P_ENTRY\n");
         my_printf_2("rien à faire\n");
         break;
      }    
      case MARCEL_SEM_P_EXIT: {
         my_printf_2("MARCEL_SEM_P_EXIT\n");
         my_printf_2("rien à faire\n");
         break;
      }     
      case MARCEL_WAKE_UP_BUBBLE_ENTRY: {
         my_printf_2("MARCEL_WAKE_UP_BUBBLE_ENTRY\n");
         my_printf_2("rien à faire\n");
         break;
      }    
      case MARCEL_WAKE_UP_BUBBLE_EXIT: {
         my_printf_2("MARCEL_WAKE_UP_BUBBLE_EXIT\n");
         my_printf_2("rien à faire\n");
         break;
      }      
	     */
      case 0:{
         my_printf_2("FIN DE TRACE\n");
         my_printf_2("rien à faire\n");
         break;
      }
      default:
         break;
   }
   return code;
}

int GetTime_Ev(ev_t * adrev)
{
   return adrev->time;
}

int GetFin_Ev(ev_t * adrev)
{
   return adrev->fin_trace;
}

int GetAdr_Thread_Ev(ev_t * adrev)
{
   return adrev->adr_thread;
}

char * GetNom_Thread_Ev(ev_t * adrev)
{
   return adrev->nom_thread;
}

int GetId_Thread_Ev(ev_t * adrev)
{
   return adrev->id_thread;
}

int GetNext_Thread_Ev(ev_t * adrev)
{
   return adrev->next_thread;
}

int GetAdr_Bulle_Ev(ev_t * adrev)
{
   return adrev->adr_bulle;
}

int GetPrio_Ev(ev_t * adrev)
{
   return adrev->prio;
}

int GetMere_Bulle_Ev(ev_t * adrev)
{
   return adrev->mere_bulle;
}

int GetNombre_Lvs_Ev(ev_t * adrev)
{
   return adrev->nb_levels;
}

int GetLevel_Rq_Ev(ev_t * adrev)
{
   return adrev->level_rq;
}

int GetPlace_Rq_Ev(ev_t * adrev)
{
   return adrev->place_rq;
}

int GetTaille_Lv_Ev(ev_t * adrev)
{
   return adrev->taille_lv;
}

int GetAdr_Rq_Ev(ev_t * adrev)
{
   return adrev->adr_rq;
}

/* accesseur de l'événement i de tracetab */

ev_t * GetEv(ev_t * tracetab,int i)
{
   return &tracetab[i];
}

/************************************************************/
/* allocation mémoire d'un tableau de trace */
/* ne pas oublier le free */
 
ev_t * malloc_tracetab(char * fichier)
{
   inf_t infos_trace;
   infos_trace = lire_trace(fichier);

   my_printf("\n\n****** Nombre d'événements   %d ******\n",infos_trace.nb_evts);
   my_printf("****** Nombre de levels   %d ******\n",infos_trace.nb_levels);
  
   /* tableau de structures et de levels !!!! très important */
   ev_t * tracetab = malloc((infos_trace.nb_evts+1) * sizeof(ev_t));
   if (tracetab == NULL)
      return -1;
   int * leveltab = malloc((infos_trace.nb_levels) * sizeof(int));
   if (leveltab == NULL)
      return -1;

   /* initialisation des champs du tableau leveltab à 0 */
   int i = 0;
   for (i=0;i<infos_trace.nb_levels;i++)
      leveltab[i] = 0;

   /* la fonction qui analyse la trace */
   tracetab = analyser_trace(fichier,tracetab,leveltab);
  
   free(leveltab);
   return tracetab;
}

int free_tracetab(ev_t * tracetab)
{
   free(tracetab);
   return 0;
}

int my_printf(const char* format, ...)
{
   va_list ap;
   va_start(ap, format);
   
#ifdef DEBUG_VERB
   vprintf(format, ap);
#endif

   va_end(ap);
}

int my_printf_2(const char* format, ...)
{
   va_list ap;
   va_start(ap, format);
   
#ifdef DEBUG_VERB2
   vprintf(format, ap);
#endif

   va_end(ap);
}

/************************************************************/
/* a toi de faire ton main et de lancer malloc_tracetab*/
/* regarde dans le GetCode_Ev pour savoir 
   quels champs valides en fonction du code événement */
/* n'oublie pas de libérer l'espace mémoire */


/* int main(int argc, char * argv[]) */
/* { */
/*    if (argc != 2) */
/*    { */
/* 	  fprintf(stderr,"veuillez specifier le fichier trace\n"); */
/* 	  exit(EXIT_FAILURE); */
/*    } */

/*    /\***************************************\/   */

/*    /\* la fonction qui alloue le trace tab *\/ */
/*    ev_t * tracetab = malloc_tracetab(argv[1]); */

/*    /\***************************************\/   */
  
/*    printf("\n"); */

/*    /\* parcours du tableau de sortie *\/ */
/*    int i = 0; */
  
/*    /\*  printf("\n\n*******************************\n"); *\/ */
/*    /\*   printf("\nlecture du tableau de structure\n\n\n"); *\/ */
/*    for(;;){ */
/*       /\* 	printf("%d  ",GetTime_Ev(GetEv(tracetab,i))); *\/ */
/*       printf("%llx  \n",GetCode_Ev(GetEv(tracetab,i)));  */
/*       /\* 	printf("%d  ",GetFin_Ev(GetEv(tracetab,i))); *\/ */
/*       /\* 	printf("%llx  ",GetAdr_Thread_Ev(GetEv(tracetab,i))); *\/ */
/*       /\* 	printf("%d  ",GetId_Thread_Ev(GetEv(tracetab,i))); *\/ */
/*       /\* 	printf("%s  ",GetNom_Thread_Ev(GetEv(tracetab,i)));	 *\/ */
/*       /\* 	printf("%llx  ",GetNext_Thread_Ev(GetEv(tracetab,i))); *\/ */
/*       /\* 	printf("%llx  ",GetAdr_Bulle_Ev(GetEv(tracetab,i))); *\/ */
/*       /\* 	printf("%d  ",GetPrio_Ev(GetEv(tracetab,i))); *\/ */
/*       /\* 	printf("%llx  ",GetMere_Bulle_Ev(GetEv(tracetab,i))); *\/ */
/*       /\* 	printf("%d  ",GetNombre_Lvs_Ev(GetEv(tracetab,i))); *\/ */
/*       /\* 	printf("%d  ",GetLevel_Rq_Ev(GetEv(tracetab,i))); *\/ */
/*       /\* 	printf("%d  ",GetPlace_Rq_Ev(GetEv(tracetab,i))); *\/ */
/*       /\* 	printf("%d  ",GetTaille_Lv_Ev(GetEv(tracetab,i))); *\/ */
/*       /\* 	printf("%llx\n\n",GetAdr_Rq_Ev(GetEv(tracetab,i))); *\/ */
/*       if (GetFin_Ev(GetEv(tracetab,i))) */
/*          break; */
/*       i++; */
/*    } */

/*    /\**************************************\/   */

/*    /\* IMPORTANT : libération de la mémoire du tableau *\/ */
/*    free_tracetab(tracetab); */

/*    return 0; */
/* } */
