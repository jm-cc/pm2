#define MAXE	30

#ifdef MT
#ifdef MARCEL
#include "marcel.h"
#else
#include <pthread.h>
#endif
#endif

typedef struct {
		int ToCity ;
		int dist ;
	       } pair_t ;

typedef pair_t DistArray_t [MAXE] ;

typedef DistArray_t DTab_t [MAXE] ;


typedef struct {
		int NrTowns ;
		DTab_t dst ;
	      } DistTab_t ;

/* Job types */

typedef int Path_t [MAXE] ;

typedef struct {
		int len ;
		Path_t path ;
	       } Job_t ;

/* TSQ Queue */

typedef struct Maillon 
             {
	      Job_t tsp_job ;
              struct Maillon *next ;
	     } Maillon ;

typedef struct {
                Maillon *first ;
		Maillon *last ;
		int end;
#ifdef MT
#ifdef MARCEL
                marcel_mutex_t mutex ;
#else
		pthread_mutex_t mutex ;
#endif
#endif
               } TSPqueue ;
		



