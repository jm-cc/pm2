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

// *************************************************

#define MAX_TOWNS 32

#define DEFAULT_INITIAL_DEPTH 3

static unsigned best_found = UINT_MAX;

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
} queue_t ;

static unsigned nb_threads, seed, prof_ini;

static DistTab_t    TSPdata;
static queue_t TSPqueue;

#define nb_towns  (TSPdata.NrTowns)

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
  fprintf(stderr, "TSP data pseudo-randomly generated\n");
}

static void init_queue (queue_t *q)
{
 q->first = q->last = NULL;
}

static int empty_queue(queue_t *q)
{
  return q->first == NULL;
}

static unsigned stat_jobs = 0;

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

  stat_jobs++;
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
   for (i=0;i<TSPdata.NrTowns;i++)
    {
     city = TSPdata.dst[me][i].ToCity ;
     if (!present(city, hops, path))
      {
       path [hops] = city ;
       dist = TSPdata.dst[me][i].dist ;
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
}

static void bcast_best_found(unsigned len);

static void tsp (int hops, int len, Path_t path, int *cuts, int num_worker)
{
  int i ;
  int city, me, dist ;

  if (len >= best_found) {
    (*cuts)++ ;
    return;
  }

 if (hops == TSPdata.NrTowns)
  {
    if (len < best_found) {
      bcast_best_found(len);

      marcel_mutex_lock(&display_mutex);
      fprintf(stderr, "worker[%d] finds path len = %3d :", num_worker, len) ;
      for (i=0; i < TSPdata.NrTowns; i++)
	fprintf(stderr, "%2d ", path[i]) ;
      fprintf(stderr, "\n") ;
      marcel_mutex_unlock(&display_mutex);
    }
  }
 else
  {
   me = path [hops-1] ;

   for (i=0; i < TSPdata.NrTowns; i++)
    {
     city = TSPdata.dst [me][i].ToCity ;
     if (!present (city, hops, path))
      {
       path [hops] = city ;
       dist = TSPdata.dst[me][i].dist ;
       tsp (hops+1, len+dist, path, cuts, num_worker) ;
      }
    }
  }
}

// *************************************************

static unsigned INIT, ADD_JOB, BEST, WORKER, FINISHED;

static void INIT_service(void)
{
  pm2_unpack_int(SEND_CHEAPER, RECV_EXPRESS, &nb_towns, 1);
  pm2_unpack_int(SEND_CHEAPER, RECV_EXPRESS, &prof_ini, 1);
  pm2_unpack_int(SEND_CHEAPER, RECV_EXPRESS, &seed, 1);
  pm2_rawrpc_waitdata();

  genmap(nb_towns, TSPdata.dst);
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
  pm2_unpack_int(SEND_CHEAPER, RECV_EXPRESS, &best_found, 1);
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

  fprintf(stderr, "Worker [%2d] starts \n", num_worker);

  fprintf(stderr, "thread %p on LWP %d\n",
	  marcel_self(), marcel_current_vp ());

  while(get_job (&job)) {
    jobcount++ ;
    tsp (prof_ini, job.len, job.path, &cuts, num_worker) ;
  }

  fprintf (stderr, "Worker [%2d] terminates, %4d jobs done with %4d cuts.\n",
	   num_worker, jobcount, cuts) ;

  signal_finished();
}

static void WORKER_service(void)
{
  unsigned n;

  pm2_rawrpc_waitdata();

  for(n=0; n<nb_threads; n++)
    pm2_thread_create(worker, (void*)(n + (pm2_self()-1)*nb_threads));
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

  target = (target + 1) % pm2_config_size();
}

static void bcast_best_found(unsigned len)
{
  unsigned i;

  for(i=0; i<pm2_config_size(); i++)
    if(i == pm2_self())
      best_found = len;
    else {
      pm2_rawrpc_begin(i, BEST, NULL);
      pm2_pack_int(SEND_CHEAPER, RECV_EXPRESS, &len, 1);
      pm2_rawrpc_end();
    }
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

static void startup_func(int argc, char *argv[], void *arg)
{
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
}

int pm2_main(int argc, char **argv)
{
  int i;

  pm2_rawrpc_register(&INIT, INIT_service);
  pm2_rawrpc_register(&ADD_JOB, ADD_JOB_service);
  pm2_rawrpc_register(&BEST, BEST_service);
  pm2_rawrpc_register(&WORKER, WORKER_service);
  pm2_rawrpc_register(&FINISHED, FINISHED_service);

  init_queue(&TSPqueue);

  pm2_push_startup_func(startup_func, NULL);

  pm2_init(&argc, argv);

  if(pm2_self() == 0) {

    genmap(nb_towns, TSPdata.dst);

    for(i=1; i<pm2_config_size(); i++) {
      pm2_rawrpc_begin(1, INIT, NULL);
      pm2_pack_int(SEND_CHEAPER, RECV_EXPRESS, &nb_towns, 1);
      pm2_pack_int(SEND_CHEAPER, RECV_EXPRESS, &prof_ini, 1);
      pm2_pack_int(SEND_CHEAPER, RECV_EXPRESS, &seed, 1);
      pm2_rawrpc_end();
    }

    generate_jobs();

    spawn_workers();

    pm2_halt();
  }

  pm2_exit();

  fprintf(stderr, "%d jobs generated\n", stat_jobs);
  return 0;
}
