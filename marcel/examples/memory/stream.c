/* stream based on the STREAM benchmark
   (http://www.cs.virginia.edu/stream).
   Original licence below:
 */

/*-----------------------------------------------------------------------*/
/* Copyright 1991-2005: John D. McCalpin                                 */
/*-----------------------------------------------------------------------*/
/* License:                                                              */
/*  1. You are free to use this program and/or to redistribute           */
/*     this program.                                                     */
/*  2. You are free to modify this program for your own use,             */
/*     including commercial use, subject to the publication              */
/*     restrictions in item 3.                                           */
/*  3. You are free to publish results obtained from running this        */
/*     program, or from works that you derive from this program,         */
/*     with the following limitations:                                   */
/*     3a. In order to be referred to as "STREAM benchmark results",     */
/*         published results must be in conformance to the STREAM        */
/*         Run Rules, (briefly reviewed below) published at              */
/*         http://www.cs.virginia.edu/stream/ref.html                    */
/*         and incorporated herein by reference.                         */
/*         As the copyright holder, John McCalpin retains the            */
/*         right to determine conformity with the Run Rules.             */
/*     3b. Results based on modified source code or on runs not in       */
/*         accordance with the STREAM Run Rules must be clearly          */
/*         labelled whenever they are published.  Examples of            */
/*         proper labelling include:                                     */
/*         "tuned STREAM benchmark results"                              */
/*         "based on a variant of the STREAM benchmark code"             */
/*         Other comparable, clear and reasonable labelling is           */
/*         acceptable.                                                   */
/*     3c. Submission of results to the STREAM benchmark web site        */
/*         is encouraged, but not required.                              */
/*  4. Use of this program or creation of derived works based on this    */
/*     program constitutes acceptance of these licensing restrictions.   */
/*  5. Absolutely no warranty is expressed or implied.                   */
/*-----------------------------------------------------------------------*/

# include <float.h>
# include <marcel.h>

# define N	3000000
# define NTIMES	10
# define OFFSET	0

#define TAB_SIZE  ((N + OFFSET) * sizeof (double))

#define USE_LIBNUMA 0

#if USE_LIBNUMA
# include <numa.h>
#endif

# define HLINE "-------------------------------------------------------------\n"

# ifndef MIN
# define MIN(x,y) ((x)<(y)?(x):(y))
# endif
# ifndef MAX
# define MAX(x,y) ((x)>(y)?(x):(y))
# endif

# define NB_NUMA_NODES 4
# define NB_THREADS_PER_TEAMS 4

double **a, **b, **c;

static double	avgtime[4] = {0}, maxtime[4] = {0},
  mintime[4] = {FLT_MAX,FLT_MAX,FLT_MAX,FLT_MAX};

static char	*label[4] = {"Copy:      ", "Scale:     ",
			     "Add:       ", "Triad:     "};

static double	bytes[4] = {
  2 * sizeof(double) * N,
  2 * sizeof(double) * N,
  3 * sizeof(double) * N,
  3 * sizeof(double) * N
};

extern double mysecond();
extern void checkSTREAMresults();
extern void tuned_STREAM_Copy();
extern void tuned_STREAM_Scale(double scalar);
extern void tuned_STREAM_Add();
extern void tuned_STREAM_Triad(double scalar);

typedef struct arg_s {
  int node, thread;
} arg_t;

arg_t                   args[NB_NUMA_NODES*NB_THREADS_PER_TEAMS];
marcel_attr_t           attr;
marcel_t                threads[NB_NUMA_NODES*NB_THREADS_PER_TEAMS];
double                  scalar;

any_t init_matrices(any_t arg) {
  arg_t *myarg = (arg_t *) arg;
  int start=myarg->thread*(N/NB_THREADS_PER_TEAMS);
  int j;

  for (j=start; j<start+N/NB_THREADS_PER_TEAMS; j++) {
    a[myarg->node][j] = 1.0;
    b[myarg->node][j] = 2.0;
    c[myarg->node][j] = 0.0;
  }
  return 0;
}

any_t copy_matrices(any_t arg) {
  arg_t *myarg = (arg_t *) arg;
  int start=myarg->thread*(N/NB_THREADS_PER_TEAMS);
  int j;

  for (j=start; j<start+N/NB_THREADS_PER_TEAMS; j++) {
    c[myarg->node][j] = a[myarg->node][j];
  }
  return 0;
}

any_t scale_matrices(any_t arg) {
  arg_t *myarg = (arg_t *) arg;
  int start=myarg->thread*(N/NB_THREADS_PER_TEAMS);
  int j;

  for (j=start; j<start+N/NB_THREADS_PER_TEAMS; j++) {
    b[myarg->node][j] = scalar*c[myarg->node][j];
  }
  return 0;
}

any_t add_matrices(any_t arg) {
  arg_t *myarg = (arg_t *) arg;
  int start=myarg->thread*(N/NB_THREADS_PER_TEAMS);
  int j;

  for (j=start; j<start+N/NB_THREADS_PER_TEAMS; j++) {
    c[myarg->node][j] = a[myarg->node][j]+b[myarg->node][j];
  }
  return 0;
}

any_t triadd_matrices(any_t arg) {
  arg_t *myarg = (arg_t *) arg;
  int start=myarg->thread*(N/NB_THREADS_PER_TEAMS);
  int j;

  for (j=start; j<start+N/NB_THREADS_PER_TEAMS; j++) {
    a[myarg->node][j] = b[myarg->node][j]+scalar*c[myarg->node][j];
  }
  return 0;
}

int marcel_main(int argc, char **argv)
{
  int			  BytesPerWord;
  int	                  i, j, k;
  double		  t, times[4][NTIMES];
  marcel_memory_manager_t memory_manager;

  marcel_init(&argc,argv);
  marcel_attr_init(&attr);
  marcel_memory_init(&memory_manager);

  a = malloc (NB_NUMA_NODES * sizeof (double *));
  b = malloc (NB_NUMA_NODES * sizeof (double *));
  c = malloc (NB_NUMA_NODES * sizeof (double *));
#if USE_LIBNUMA
  /* Bind the accessed data to the nodes. */
  for (i = 0; i < NB_NUMA_NODES; i++) {
    a[i] = numa_alloc_onnode (TAB_SIZE, i);
    b[i] = numa_alloc_onnode (TAB_SIZE, i);
    c[i] = numa_alloc_onnode (TAB_SIZE, i);
  }
#else
  for (i = 0; i < NB_NUMA_NODES; i++) {
    a[i] = marcel_memory_malloc(&memory_manager, TAB_SIZE, MARCEL_MEMORY_MEMBIND_POLICY_SPECIFIC_NODE, i);
    b[i] = marcel_memory_malloc(&memory_manager, TAB_SIZE, MARCEL_MEMORY_MEMBIND_POLICY_SPECIFIC_NODE, i);
    c[i] = marcel_memory_malloc(&memory_manager, TAB_SIZE, MARCEL_MEMORY_MEMBIND_POLICY_SPECIFIC_NODE, i);
  }

#endif
  for (i = 0; i < NB_NUMA_NODES; i++) {
    marcel_printf("a[%d] = %p - b[%d] = %p - c[%d] = %p\n", i, a[i], i, b[i], i, c[i]);
  }

  /* --- SETUP --- determine precision and check timing --- */

  marcel_printf(HLINE);
  marcel_printf("STREAM version $Revision: 5.8 $\n");
  marcel_printf(HLINE);
  BytesPerWord = sizeof(double);
  marcel_printf("This system uses %d bytes per DOUBLE PRECISION word.\n", BytesPerWord);
  marcel_printf(HLINE);
  marcel_printf("Array size = %d, Offset = %d\n" , N, OFFSET);
  marcel_printf("Total memory required = %.1f MB.\n", (3.0 * BytesPerWord) * ( (double) N*NB_NUMA_NODES / 1048576.0));
  marcel_printf("Each test is run %d times, but only\n", NTIMES);
  marcel_printf("the *best* time for each is used.\n");
  marcel_printf(HLINE);

  /* Get initial value for system clock. */
  for (i = 0; i < NB_NUMA_NODES; i++) {
    for (j=0; j<NB_THREADS_PER_TEAMS; j++) {
      int id=(i*NB_THREADS_PER_TEAMS)+j;
      args[id].node = i;
      args[id].thread = j;
      marcel_attr_settopo_level(&attr, &marcel_topo_core_level[id]);
      marcel_create(&threads[(i*NB_THREADS_PER_TEAMS)+j], &attr, init_matrices, (any_t)&args[id]);
    }
  }

  for (i = 0; i < NB_NUMA_NODES; i++) {
    for (j=0; j<NB_THREADS_PER_TEAMS; j++) {
      marcel_join(threads[(i*NB_THREADS_PER_TEAMS)+j], NULL);
    }
  }

  for (i = 0; i < NB_NUMA_NODES; i++) {
    for (j = 0; j < N; j++) {
      a[i][j] = 2.0E0 * a[i][j];
    }
  }

  /*	--- MAIN LOOP --- repeat test cases NTIMES times --- */
  scalar = 3.0;
  for (k=0; k<NTIMES; k++) {
//#pragma omp parallel for num_threads (NB_NUMA_NODES)
//    //      for (i = 0; i < NB_NUMA_NODES; i++) {
//    //	fgomp_set_current_thread_affinity (c[i], TAB_SIZE);
//    //      }
//
    times[0][k] = mysecond();
    tuned_STREAM_Copy();
    times[0][k] = mysecond() - times[0][k];

    times[1][k] = mysecond();
    tuned_STREAM_Scale(scalar);
    times[1][k] = mysecond() - times[1][k];

    times[2][k] = mysecond();
    tuned_STREAM_Add();
    times[2][k] = mysecond() - times[2][k];

    times[3][k] = mysecond();
    tuned_STREAM_Triad(scalar);
    times[3][k] = mysecond() - times[3][k];
  }

  /*	--- SUMMARY --- */

  for (k=1; k<NTIMES; k++) { /* note -- skip first iteration */
    for (j=0; j<4; j++) {
      avgtime[j] = avgtime[j] + times[j][k];
      mintime[j] = MIN(mintime[j], times[j][k]);
      maxtime[j] = MAX(maxtime[j], times[j][k]);
    }
  }

  marcel_printf("Function      Rate (MB/s)   Avg time     Min time     Max time\n");
  for (j=0; j<4; j++) {
    avgtime[j] = avgtime[j]/(double)(NTIMES-1);

    marcel_printf("%s%11.4f  %11.4f  %11.4f  %11.4f\n", label[j],
                  1.0E-06 * bytes[j]/mintime[j],
                  avgtime[j],
                  mintime[j],
                  maxtime[j]);
  }
  marcel_printf(HLINE);

  /* --- Check Results --- */
  checkSTREAMresults();
  marcel_printf(HLINE);

#if USE_LIBNUMA
  for (i = 0; i < NB_NUMA_NODES; i++) {
    numa_free (a[i], TAB_SIZE);
    numa_free (b[i], TAB_SIZE);
    numa_free (c[i], TAB_SIZE);
  }
#else
  for (i = 0; i < NB_NUMA_NODES; i++) {
    marcel_memory_free(&memory_manager, a[i]);
    marcel_memory_free(&memory_manager, b[i]);
    marcel_memory_free(&memory_manager, c[i]);
  }
#endif
  free (a);
  free (b);
  free (c);

  // Finish marcel
  marcel_memory_exit(&memory_manager);
  marcel_end();
  return 0;
}

# define	M	20

/* A gettimeofday routine to give access to the wall
   clock timer on most UNIX-like systems.  */

#include <sys/time.h>

double mysecond() {
  struct timeval tp;
  struct timezone tzp;
  int i;

  i = gettimeofday(&tp,&tzp);
  return ( (double) tp.tv_sec + (double) tp.tv_usec * 1.e-6 );
}

void checkSTREAMresults() {
  double aj,bj,cj,scalar;
  double asum,bsum,csum;
  double epsilon;
  int	i, j, k;

  /* reproduce initialization */
  aj = 1.0;
  bj = 2.0;
  cj = 0.0;
  /* a[] is modified during timing check */
  aj = 2.0E0 * aj;
  /* now execute timing loop */
  scalar = 3.0;
  for (k=0; k<NTIMES; k++) {
    cj = aj;
    bj = scalar*cj;
    cj = aj+bj;
    aj = bj+scalar*cj;
  }
  aj = aj * (double) (N*NB_NUMA_NODES);
  bj = bj * (double) (N*NB_NUMA_NODES);
  cj = cj * (double) (N*NB_NUMA_NODES);

  asum = 0.0;
  bsum = 0.0;
  csum = 0.0;
  for (i = 0; i < NB_NUMA_NODES; i++) {
    for (j=0; j<N; j++) {
      asum += a[i][j];
      bsum += b[i][j];
      csum += c[i][j];
    }
  }
#ifdef VERBOSE
  marcel_printf ("Results Comparison: \n");
  marcel_printf ("        Expected  : %f %f %f \n",aj,bj,cj);
  marcel_printf ("        Observed  : %f %f %f \n",asum,bsum,csum);
#endif

#ifndef abs
#define abs(a) ((a) >= 0 ? (a) : -(a))
#endif
  epsilon = 1.e-8;

  if (abs(aj-asum)/asum > epsilon) {
    marcel_printf ("Failed Validation on array a[]\n");
    marcel_printf ("        Expected  : %f \n",aj);
    marcel_printf ("        Observed  : %f \n",asum);
  }
  else if (abs(bj-bsum)/bsum > epsilon) {
    marcel_printf ("Failed Validation on array b[]\n");
    marcel_printf ("        Expected  : %f \n",bj);
    marcel_printf ("        Observed  : %f \n",bsum);
  }
  else if (abs(cj-csum)/csum > epsilon) {
    marcel_printf ("Failed Validation on array c[]\n");
    marcel_printf ("        Expected  : %f \n",cj);
    marcel_printf ("        Observed  : %f \n",csum);
  }
  else {
    marcel_printf ("Solution Validates\n");
  }
}

void tuned_STREAM_Copy() {
  int i, j;

  for (i = 0; i < NB_NUMA_NODES; i++) {
    for (j=0; j<NB_THREADS_PER_TEAMS; j++) {
      int id=(i*NB_THREADS_PER_TEAMS)+j;
      marcel_attr_settopo_level(&attr, &marcel_topo_core_level[id]);
      marcel_create(&threads[(i*NB_THREADS_PER_TEAMS)+j], &attr, copy_matrices, (any_t)&args[id]);
    }
  }
  for (i = 0; i < NB_NUMA_NODES; i++) {
    for (j=0; j<NB_THREADS_PER_TEAMS; j++) {
      marcel_join(threads[(i*NB_THREADS_PER_TEAMS)+j], NULL);
    }
  }
}

void tuned_STREAM_Scale(double scalar) {
  int i, j;

  for (i = 0; i < NB_NUMA_NODES; i++) {
    for (j=0; j<NB_THREADS_PER_TEAMS; j++) {
      int id=(i*NB_THREADS_PER_TEAMS)+j;
      marcel_attr_settopo_level(&attr, &marcel_topo_core_level[id]);
      marcel_create(&threads[(i*NB_THREADS_PER_TEAMS)+j], &attr, scale_matrices, (any_t)&args[id]);
    }
  }
  for (i = 0; i < NB_NUMA_NODES; i++) {
    for (j=0; j<NB_THREADS_PER_TEAMS; j++) {
      marcel_join(threads[(i*NB_THREADS_PER_TEAMS)+j], NULL);
    }
  }
}

void tuned_STREAM_Add() {
  int i, j;

  for (i = 0; i < NB_NUMA_NODES; i++) {
    for (j=0; j<NB_THREADS_PER_TEAMS; j++) {
      int id=(i*NB_THREADS_PER_TEAMS)+j;
      marcel_attr_settopo_level(&attr, &marcel_topo_core_level[id]);
      marcel_create(&threads[(i*NB_THREADS_PER_TEAMS)+j], &attr, add_matrices, (any_t)&args[id]);
    }
  }
  for (i = 0; i < NB_NUMA_NODES; i++) {
    for (j=0; j<NB_THREADS_PER_TEAMS; j++) {
      marcel_join(threads[(i*NB_THREADS_PER_TEAMS)+j], NULL);
    }
  }
}

void tuned_STREAM_Triad(double scalar) {
  int i, j;

  for (i = 0; i < NB_NUMA_NODES; i++) {
    for (j=0; j<NB_THREADS_PER_TEAMS; j++) {
      int id=(i*NB_THREADS_PER_TEAMS)+j;
      marcel_attr_settopo_level(&attr, &marcel_topo_core_level[id]);
      marcel_create(&threads[(i*NB_THREADS_PER_TEAMS)+j], &attr, triadd_matrices, (any_t)&args[id]);
    }
  }
  for (i = 0; i < NB_NUMA_NODES; i++) {
    for (j=0; j<NB_THREADS_PER_TEAMS; j++) {
      marcel_join(threads[(i*NB_THREADS_PER_TEAMS)+j], NULL);
    }
  }
}
