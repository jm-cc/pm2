
#include <marcel.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#define	  SIZE		256

typedef int matrix_t[SIZE][SIZE];

typedef struct {
  int i, j;
} req;

int       nb_threads;
marcel_t  *tids;
req       *reqs;
matrix_t  a, b, c;
int       block_size;


static struct timeval _tv;
static struct timezone _tz;

void init_time()
{
  gettimeofday(&_tv, &_tz);
}

unsigned long cpu_time()
{
  struct timeval t;

  gettimeofday(&t, &_tz);
  return ( 1000000L * (t.tv_sec - _tv.tv_sec) +
	   t.tv_usec - _tv.tv_usec ) ;
}


void get(matrix_t m)
{
  int i, j;

  for(i=0; i<SIZE; i++) {
    for(j=0; j<SIZE; j++) {
      m[i][j] = 1;
    }
  }
}

void put(matrix_t m, char *s)
{
  int i, j;

  printf("matrix %s\n", s);

  for(i=0; i<SIZE; i++) {
    for(j=0; j<SIZE; j++) {
      printf("%3d ", m[i][j]);
    }
    printf ("\n");
  }
}
 

any_t bloc(any_t arg)
{
  req *r = (req *)arg;
  int i, j, k;
  int x=r->i, y=r->j;
  int res;

  tfprintf(stderr, "Thread %p on LWP %d\n", marcel_self(), marcel_current_vp());

  for(i=x; i<x+block_size; i++) {
    for(j=y; j<y+block_size; j++) {
      res = 0;
      for(k=0; k<SIZE; k++) {
	res += a[i][k] * b[k][j] ; 
      }
      c[i][j] = res;
    }
  }
  return NULL;
}



void mult() 
{
  int i, j, x = 0;
  marcel_attr_t attr;

  nb_threads = (SIZE*SIZE)/(block_size*block_size);
  tids = (marcel_t *)malloc(nb_threads*sizeof(int));
  reqs = (req *)tmalloc(nb_threads*sizeof(req));

  marcel_attr_init(&attr);
  marcel_attr_setschedpolicy(&attr, MARCEL_SCHED_BALANCE);

  for(i=0; i<SIZE; i+=block_size) {
    for(j=0; j<SIZE; j+=block_size) {
      reqs[x].i = i;
      reqs[x].j = j;
      if(i || j)
	marcel_create(&tids[x], &attr, bloc, &reqs[x]);
      x++;
    }
  }

  bloc(&reqs[0]);

  for(x=1; x<nb_threads; x++)
    marcel_join(tids[x], NULL);

  tfree(tids);
  tfree(reqs);
}


int marcel_main(int argc, char *argv[])
{
  unsigned long temps;

  marcel_init(&argc, argv);

  if(argc > 1)
    block_size = atoi(argv[1]);
  else
    block_size = SIZE/2;

  get(a);
  get(b);

  init_time(); 
  mult();
  temps = cpu_time();

  printf("time = %ld.%03ldms\n", temps/1000, temps%1000);

  return 0;
}



