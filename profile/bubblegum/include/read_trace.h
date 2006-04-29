/* structure d'infos concernant chaque événements */

#define PARCOURS 0
#define ANALYSE 1

struct infos_tr {
  int nb_evts;
  int nb_levels;
};

struct etat_ev {
  int code_ev; 					/* code evenemnt */
  int time;
  int fin_trace;				/* est-on à la fin de la trace ? */

  /* thread */
  int adr_thread;			
  char * nom_thread;
  int id_thread;
  int next_thread; 

  /* bulle */
  int adr_bulle;
  int mere_bulle;

  /* runqueue */
  int nb_levels;
  int level_rq;
  int taille_lv;
  int adr_rq;
  int place_rq;

  /**/
  int prio;
};

typedef struct etat_ev ev_t;
typedef struct infos_tr inf_t;

ev_t set_etat_ev( ev_t evt, int * ad_nb_levels, fxt_blockev_t blockev_machin,
							int ev_type, struct fxt_ev ev, int * leveltab, int mode);
inf_t lire_trace(char * fichier_trace);
ev_t * analyser_trace(char * fichier,ev_t * tracetab,int * leveltab);
ev_t * malloc_tracetab(char * fichier);
int free_tracetab(ev_t * tracetab);

void Remake_IdNom_Ev(ev_t *);

ev_t * GetEv(ev_t * tracetab,int indice);

int GetCode_Ev(ev_t *);
int GetTime_Ev(ev_t *);
int GetFin_Ev(ev_t *);

int GetAdr_Thread_Ev(ev_t *);
char * GetNom_Thread_Ev(ev_t *);
int GetId_Thread_Ev(ev_t *);
int GetNext_Thread_Ev(ev_t *);

int GetAdr_Bulle_Ev(ev_t *);
int GetMere_Bulle_Ev(ev_t *);

int GetPrio_Ev(ev_t *);

int GetNombre_Lvs_Ev(ev_t *);
int GetLevel_Rq_Ev(ev_t *);
int GetPlace_Rq_Ev(ev_t *);
int GetTaille_Lv_Ev(ev_t *);
int GetAdr_Rq_Ev(ev_t *);

int my_printf(const char* format, ...);
int my_printf_2(const char* format, ...);
