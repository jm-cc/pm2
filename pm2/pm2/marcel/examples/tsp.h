
extern int tsp (int hops, int len, Path_t path, int *cuts, int num_worker) ;

extern void distributor (int hops, int len, Path_t path, TSPqueue *q, DistTab_t distance) ;

extern void GenerateJobs () ;


extern void *worker (void *) ;
