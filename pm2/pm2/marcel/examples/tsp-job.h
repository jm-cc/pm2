
extern void init_queue (TSPqueue *q) ;

extern int empty_queue (TSPqueue q) ;

extern void add_job (TSPqueue *q, Job_t j)  ;

extern int get_job (TSPqueue *q, Job_t *j) ;

extern void no_more_jobs (TSPqueue *q) ;
