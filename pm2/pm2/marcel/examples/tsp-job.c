#include "tsp-types.h"
#include <malloc.h>

#ifdef MT
#ifdef MARCEL
#include <marcel.h>
#endif
#endif

void init_queue (TSPqueue *q)
{
 q->first = 0 ;
 q->last = 0 ;
 q->end = 0 ;
#ifdef MT
#ifdef MARCEL
 marcel_mutex_init (&q->mutex, NULL) ;
#else
 pthread_mutex_init (&q->mutex, NULL) ;
#endif
#endif
}


int empty_queue (TSPqueue q)
{
 int b  ;

#ifdef MT
#ifdef MARCEL
 marcel_mutex_lock (&q.mutex) ;
#else
 pthread_mutex_lock (&q.mutex) ;
#endif
#endif

   b = ((q.first == 0) && (q.end == 1)) ;

#ifdef MT
#ifdef MARCEL
 marcel_mutex_unlock (&q.mutex) ;
#else
 pthread_mutex_unlock (&q.mutex) ;
#endif
#endif
 return b ;

}


void add_job (TSPqueue *q, Job_t j) 
{
 Maillon *ptr ;
 unsigned int i ;

 ptr = (Maillon *) malloc (sizeof (Maillon)) ;
 ptr->next = 0 ;
 ptr->tsp_job.len=j.len ;
 for (i=0;i<MAXE;i++)
   ptr->tsp_job.path [i] = j.path [i] ;

#ifdef MT
#ifdef MARCEL
 marcel_mutex_lock (&q->mutex) ;
#else
 pthread_mutex_lock (&q->mutex) ;
#endif
#endif

   if (q->first   == 0)
      q->first = q->last = ptr ;
    else
      {
       q->last->next = ptr ;
       q->last = ptr ;
      }

#ifdef MT
#ifdef MARCEL
 marcel_mutex_unlock (&q->mutex) ;
#else
 pthread_mutex_unlock (&q->mutex) ;
#endif
#endif
}


int get_job (TSPqueue *q, Job_t *j)
{
 Maillon *ptr ;
 unsigned int i ;

#ifdef MT
#ifdef MARCEL
 marcel_mutex_lock (&q->mutex) ;
#else
 pthread_mutex_lock (&q->mutex) ;
#endif
#endif

     if (q->first == 0)
       {
#ifdef MT
#ifdef MARCEL
        marcel_mutex_unlock (&q->mutex) ;
#else
        pthread_mutex_unlock (&q->mutex) ;
#endif
#endif
        return 0 ;
       }

     ptr = q->first ;

     q->first = ptr->next ;
     if (q->first == 0)
     q->last = 0 ;

#ifdef MT
#ifdef MARCEL
 marcel_mutex_unlock (&q->mutex) ;
#else
 pthread_mutex_unlock (&q->mutex) ;
#endif
#endif

 j->len = ptr->tsp_job.len ;
 for (i=0;i<MAXE;i++)
  j->path[i] = ptr->tsp_job.path[i] ;


 free (ptr) ;
 return 1 ;

} 

void no_more_jobs (TSPqueue *q)
{

#ifdef MT
#ifdef MARCEL
 marcel_mutex_lock (&q->mutex) ;
#else
 pthread_mutex_lock (&q->mutex) ;
#endif
#endif

    q->end = 1 ;

#ifdef MT
#ifdef MARCEL
 marcel_mutex_unlock (&q->mutex) ;
#else
 pthread_mutex_unlock (&q->mutex) ;
#endif
#endif
}
