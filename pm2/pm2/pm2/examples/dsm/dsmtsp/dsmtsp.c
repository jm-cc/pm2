/*
                      PM2 HIGH-PERF/ISOMALLOC
           High Performance Parallel Multithreaded Machine
                           version 3.0

     Gabriel Antoniu, Olivier Aumage, Luc Bouge, Vincent Danjean,
       Christian Perez, Jean-Francois Mehaut, Raymond Namyst

            Laboratoire de l'Informatique du Parallelisme
                        UMR 5668 CNRS-INRIA
                 Ecole Normale Superieure de Lyon

                      External Contributors:
                 Yves Denneulin (LMC - Grenoble),
                 Benoit Planquelle (LIFL - Lille)

                    1998 All Rights Reserved


                             NOTICE

 Permission to use, copy, modify, and distribute this software and
 its documentation for any purpose and without fee is hereby granted
 provided that the above copyright notice appear in all copies and
 that both the copyright notice and this permission notice appear in
 supporting documentation.

 Neither the institutions (Ecole Normale Superieure de Lyon,
 Laboratoire de L'informatique du Parallelisme, Universite des
 Sciences et Technologies de Lille, Laboratoire d'Informatique
 Fondamentale de Lille), nor the Authors make any representations
 about the suitability of this software for any purpose. This
 software is provided ``as is'' without express or implied warranty.
*/

#include <math.h>
#include <limits.h>

#include "pm2.h"
#include "tbx.h"
#include "dsmlib_hbrc_mw_update_protocol.h"
#include "dsmlib_erc_sw_inv_protocol.h"

// *************************************************

#define MAX_TOWNS 32

#define DEFAULT_INITIAL_DEPTH 3

BEGIN_DSM_DATA
dsm_lock_t L;
END_DSM_DATA

static unsigned *best_found;

static marcel_mutex_t queue_mutex = MARCEL_MUTEX_INITIALIZER,
  display_mutex = MARCEL_MUTEX_INITIALIZER;

static marcel_sem_t sem;

typedef struct {
  unsigned ToCity;
  unsigned dist;
} pair_t;

typedef pair_t DistArray_t[MAX_TOWNS];

typedef DistArray_t DTab_t[MAX_TOWNS];

typedef struct {
  int NrTowns;
  DTab_t dst;
} DistTab_t;

typedef unsigned Path_t[MAX_TOWNS];

typedef struct {
  unsigned len;
  Path_t path;
} Job_t;

typedef struct Maillon {
  Job_t tsp_job;
  struct Maillon *next;
} Maillon ;

typedef struct {
  Maillon *first, *last;
} queuet_t ;

static unsigned nb_threads, seed, prof_ini, nb_towns;

static DistTab_t *TSPdata;
static queuet_t TSPqueue;


void genmap (int n, DTab_t pairs) 
{
  typedef struct {
    int x, y ;
  } coor_t ;

  typedef coor_t coortab_t [MAX_TOWNS];
  int tempdist [MAX_TOWNS] ;
  coortab_t towns ;
  int i, j, k, x =0 ;
  int dx, dy, tmp ;

  dsm_lock(L);

  srand(seed) ;

  for (i=0; i<n; i++) {
    towns [i].x = rand () % 100 ;
    towns [i].y = rand () % 100 ;
  }

  for (i=0; i<n; i++) {
    for (j=0; j<n; j++) {
      dx = towns [i].x - towns [j].x ;
      dy = towns [i].y - towns [j].y ;
      tempdist [j] = (int) sqrt ((double) ((dx * dx) + (dy * dy))) ;
    }

    for (j=0; j<n; j++) {
      tmp = INT_MAX ;
      for (k=0; k<n; k++) {
	if (tempdist [k] < tmp) {
          tmp = tempdist [k] ;
          x = k ;
	}
      }
      tempdist [x] = INT_MAX ;
      pairs [i][j].ToCity = x ;
      pairs [i][j].dist = tmp ;
    }
  }
  dsm_unlock(L);

  //  fprintf(stderr, "TSP data pseudo-randomly generated\n");
}

static void init_queue (queuet_t *q)
{
 q->first = q->last = NULL;
}

static int empty_queue(queuet_t *q)
{
  return q->first == NULL;
}

static unsigned *stat_jobs;

static void add_job(Job_t *j)
{
  Maillon *ptr;

  ptr = (Maillon *)malloc(sizeof(Maillon));
  ptr->next = NULL;
  ptr->tsp_job = *j;

  if(TSPqueue.first == 0)
    TSPqueue.first = TSPqueue.last = ptr;
  else {
    TSPqueue.last->next = ptr;
    TSPqueue.last = ptr;
  }
}

static int get_job(Job_t *j)
{
  Maillon *ptr;

  marcel_mutex_lock(&queue_mutex);

  if (TSPqueue.first == 0) {
    marcel_mutex_unlock(&queue_mutex);
    return 0;
  }

  ptr = TSPqueue.first;
  TSPqueue.first = ptr->next;
  if (TSPqueue.first == 0)
    TSPqueue.last = 0;
  *j = ptr->tsp_job;
  free (ptr);

  marcel_mutex_unlock(&queue_mutex);

  return 1;
} 

static int present (int city, int hops, Path_t path)
{
  unsigned int i ;

  for (i=0; i<hops; i++)
    if (path [i] == city)
      return 1 ;
  return 0 ;
}

static void one_more_job(Job_t *j);

static Job_t tmp_job;

static void distributor (int hops, int len, Path_t path)
{
 int i ;
 int me, city, dist ;

 if (hops == prof_ini) {
   tmp_job.len = len;
   for(i=0; i<hops; i++)
     tmp_job.path[i] = path[i];
   one_more_job(&tmp_job);
 } else {
   me = path [hops-1] ;
   for (i=0;i<nb_towns;i++)
    {
     city = (*TSPdata).dst[me][i].ToCity ;
     if (!present(city, hops, path))
      {
       path [hops] = city ;
       dist = (*TSPdata).dst[me][i].dist ;
       distributor (hops+1, len+dist, path);
      }
    }
  }
}

void generate_jobs(void) 
{
  Path_t path;

  path[0] = 0;
  distributor(1, 0, path);
  fprintf(stderr, "Jobs done\n");
}

static void bcast_best_found(unsigned len);

static void tsp (int hops, int len, Path_t path, int *cuts, int num_worker)
{
  int i ;
  int city, me, dist ;

  if (len >= *best_found) {
    (*cuts)++ ;
    return;
  }

 if (hops == nb_towns)
  {
    if (len < *best_found) {
      bcast_best_found(len);

      marcel_mutex_lock(&display_mutex);
      /*      fprintf(stderr,"worker[%d] finds path len = %3d :", num_worker, len) ;
      for (i=0; i < nb_towns; i++)
	    fprintf(stderr, "%2d ", path[i]) ;
	    fprintf(stderr, "\n") ; */
      marcel_mutex_unlock(&display_mutex);
    }
  }
 else
  {
   me = path [hops-1] ;

   for (i=0; i < nb_towns; i++)
    {
     city = (*TSPdata).dst [me][i].ToCity ;
     if (!present (city, hops, path))
      {
       path [hops] = city ;
       dist = (*TSPdata).dst[me][i].dist ;
       tsp (hops+1, len+dist, path, cuts, num_worker) ;
      }
    }
  }
}

// *************************************************

static unsigned INIT, ADD_JOB, BEST, WORKER, FINISHED;

static void INIT_service(void)
{
  //  fprintf(stderr,"Initing\n");
  pm2_unpack_int(SEND_CHEAPER, RECV_EXPRESS, &nb_towns, 1);
  pm2_unpack_int(SEND_CHEAPER, RECV_EXPRESS, &prof_ini, 1);
  pm2_unpack_int(SEND_CHEAPER, RECV_EXPRESS, &seed, 1);
  pm2_unpack_byte(SEND_CHEAPER, RECV_EXPRESS, (char*)&TSPdata, sizeof(DistTab_t *));
  pm2_unpack_byte(SEND_CHEAPER, RECV_EXPRESS, (char*)&best_found, sizeof(unsigned *));
  pm2_rawrpc_waitdata();
}

static void ADD_JOB_service(void)
{
  Job_t j;

  pm2_unpack_byte(SEND_CHEAPER, RECV_CHEAPER, (char*)&j, sizeof(Job_t));
  pm2_rawrpc_waitdata();

  add_job(&j);
}

static void BEST_service(void)
{
  pm2_unpack_int(SEND_CHEAPER, RECV_EXPRESS, best_found, 1);
  pm2_rawrpc_waitdata();
}

static void FINISHED_service(void)
{
  pm2_rawrpc_waitdata();

  marcel_sem_V(&sem);
}

static void signal_finished(void)
{
  if(pm2_self() == 0)
    marcel_sem_V(&sem);
  else {
    pm2_rawrpc_begin(0, FINISHED, NULL);
    pm2_rawrpc_end();
  }
}

static void worker(void *arg)
{
  unsigned num_worker = (int)arg;
  int jobcount = 0 ;
  Job_t job ;
  int cuts = 0 ;

  /*  fprintf(stderr, "Worker [%2d] starts on node %d\n", num_worker, pm2_self());

      fprintf(stderr, "thread %p on LWP %d\n",
      marcel_self(), marcel_current_vp ());*/

  while(get_job (&job)) {
    jobcount++ ;
    tsp (prof_ini, job.len, job.path, &cuts, num_worker) ;
  }

/*  fprintf(stderr, "Worker [%2d] terminates, %4d jobs done with %4d cuts.\n",
    num_worker, jobcount, cuts) ;*/

  signal_finished();
}

static void WORKER_service(void)
{
  unsigned n;

  pm2_rawrpc_waitdata();

  for(n=0; n<nb_threads; n++)
    pm2_thread_create(worker, (void*)(n + (pm2_self())*nb_threads));
}

static void one_more_job(Job_t *j)
{
  static unsigned target = 0;

  if(target == 0) {
    add_job(j);
  } else {
    pm2_rawrpc_begin(target, ADD_JOB, NULL);
    pm2_pack_byte(SEND_CHEAPER, RECV_CHEAPER, (char*)j, sizeof(Job_t));
    pm2_rawrpc_end();
  }
  stat_jobs[target]++;
  target = (target + 1) % pm2_config_size();
}

static void bcast_best_found(unsigned len)
{
    dsm_lock(L);
    if (*best_found > len)
      *best_found = len;
    dsm_unlock(L);
}

void spawn_workers(void)
{
  unsigned n, target;

  marcel_sem_init(&sem, 0);

  // créations distantes
  for(target=1; target<pm2_config_size(); target++) {
    pm2_rawrpc_begin(target, WORKER, NULL);
    pm2_rawrpc_end();
  }

  // créations locales
  for(n=0; n<nb_threads; n++) {
    pm2_thread_create(worker, (void*)n);
  }

  for(n=0; n<nb_threads*pm2_config_size(); n++)
    marcel_sem_P(&sem);
}

int pm2_main(int argc, char **argv)
{
  int i, prot, prot_erc, prot_hbrc, tot_jobs = 0;

  tbx_tick_t t1, t2;
  dsm_lock_attr_t attr;

  pm2_rawrpc_register(&INIT, INIT_service);
  pm2_rawrpc_register(&ADD_JOB, ADD_JOB_service);
  pm2_rawrpc_register(&BEST, BEST_service);
  pm2_rawrpc_register(&WORKER, WORKER_service);
  pm2_rawrpc_register(&FINISHED, FINISHED_service);

  dsm_set_default_protocol(MIGRATE_THREAD);
  isoaddr_page_set_distribution(DSM_BLOCK);

   
   prot_hbrc = dsm_create_protocol(dsmlib_hbrc_mw_update_rfh, // read_fault_handler  
 			     dsmlib_hbrc_mw_update_wfh, // write_fault_handler  
 			     dsmlib_hbrc_mw_update_rs, // read_server 
 			     dsmlib_hbrc_mw_update_ws, // write_server  
 			     dsmlib_hbrc_mw_update_is, // invalidate_server  
 			     dsmlib_hbrc_mw_update_rps, // receive_page_server  
 			     NULL, // expert_receive_page_server 
 			     NULL, // acquire_func  
 			     dsmlib_hbrc_release, // release_func  
 			     dsmlib_hbrc_mw_update_prot_init, // prot_init_func  
 			     dsmlib_hbrc_add_page // page_add_func  
 			     ); 
  
  prot_erc = dsm_create_protocol(dsmlib_erc_sw_inv_rfh, // read_fault_handler 
			     dsmlib_erc_sw_inv_wfh, // write_fault_handler 
			     dsmlib_erc_sw_inv_rs, // read_server
			     dsmlib_erc_sw_inv_ws, // write_server 
			     dsmlib_erc_sw_inv_is, // invalidate_server 
			     dsmlib_erc_sw_inv_rps, // receive_page_server 
			     NULL, // expert_receive_page_server
			     NULL, // acquire_func 
			     dsmlib_erc_release, // release_func 
			     dsmlib_erc_sw_inv_init, // prot_init_func 
			     dsmlib_erc_add_page  // page_add_func 
			     );

  prot = MIGRATE_THREAD;  
			     
  //  dsm_lock_attr_register_protocol(&attr, MIGRATE_THREAD);
  dsm_lock_attr_register_protocol(&attr, prot);
  dsm_lock_init(&L, &attr);
  pm2_set_dsm_page_protection(DSM_OWNER_WRITE_ACCESS_OTHER_NO_ACCESS);

  pm2_init(&argc, argv);
  if(argc < 4) {
    fprintf(stderr, "Usage: %s nbthreads nbcities seed [ ini_depth ]\n",
	    argv[0]);
    exit(1);
  }

  nb_threads = atoi(argv[1]);
  nb_towns = atoi(argv[2]);
  seed = atoi(argv[3]);
  if(argc == 5)
    prof_ini = atoi(argv[4]);
  else
    prof_ini = DEFAULT_INITIAL_DEPTH;
  init_queue(&TSPqueue);
  


  if(pm2_self() == 0) {
    isoaddr_attr_t yaattr;
	isoaddr_attr_set_atomic(&yaattr);
    isoaddr_attr_set_status(&yaattr, ISO_SHARED);
    //    isoaddr_attr_set_protocol(&yaattr, MIGRATE_THREAD);
    isoaddr_attr_set_protocol(&yaattr, prot);
    TSPdata = (DistTab_t *)pm2_malloc(sizeof(DistTab_t), &yaattr);
    best_found = (unsigned *)pm2_malloc(sizeof(unsigned *), &yaattr);
    
    dsm_lock(L);
    *best_found = UINT_MAX;
    dsm_unlock(L);
    
    fprintf(stderr,"Malloc obtained: %d\n", TSPdata);
  
  
	fprintf(stderr, "%d nodes present\n", pm2_config_size());
    stat_jobs = (unsigned *) malloc(pm2_config_size() *sizeof(unsigned));
    for(i = 0; i < pm2_config_size(); i++)
      stat_jobs[i] = 0;
    genmap(nb_towns, (*TSPdata).dst);

    for(i=1; i<pm2_config_size(); i++) {
      pm2_rawrpc_begin(i, INIT, NULL);
      pm2_pack_int(SEND_CHEAPER, RECV_EXPRESS, &nb_towns, 1);
      pm2_pack_int(SEND_CHEAPER, RECV_EXPRESS, &prof_ini, 1);
      pm2_pack_int(SEND_CHEAPER, RECV_EXPRESS, &seed, 1);
      pm2_pack_byte(SEND_CHEAPER, RECV_EXPRESS, (char*)&TSPdata, sizeof(DistTab_t *));
      pm2_pack_byte(SEND_CHEAPER, RECV_EXPRESS, (char*)&best_found, sizeof(unsigned *));
      pm2_rawrpc_end();
    }
	fprintf(stderr, "Here?------------\n");
    generate_jobs();
    fprintf(stderr, "Not There?------------\n");
    tbx_timing_init();
  	TBX_GET_TICK(t1);  
    spawn_workers();
	TBX_GET_TICK(t2);
	fprintf(stderr,"Timed: %f usecs\n", tbx_tick2usec(TBX_TICK_DIFF(t1, t2)));
	
    pm2_halt();
    
    for(i = 0; i < pm2_config_size(); i++){
  	  fprintf(stderr, "%d jobs generated on node %d\n", stat_jobs[i], i);
  	  tot_jobs += stat_jobs[i];
  	}
    fprintf(stderr, "%d jobs generated\n", tot_jobs);
  }

  pm2_exit();

  return 0;
}
