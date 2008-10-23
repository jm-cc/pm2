/* marcel_stream interface, based on the STREAM benchmark
   (http://www.cs.virginia.edu/stream). */

/* Note that you need to set the 'id' marcel thread attribute in your
   application for the STEAM functions to run as expected. */

#ifndef MARCEL_STREAM_H
#define MARCEL_STREAM_H

/* Number of working threads */
unsigned int stream_nb_threads;
/* Size of the stream_* array */
unsigned int stream_array_size;
/* Set of iterations to compute per thread */
unsigned int stream_chunk_size;

/* Arrays for the computation */
double *stream_a;
double *stream_b;
double *stream_c;

/* Gather information for the STREAM interface */
void STREAM_init (unsigned int nb_threads, unsigned int array_size, double *t1, double *t2, double *t3) {
  stream_nb_threads = nb_threads;
  stream_array_size = array_size;
  stream_chunk_size = stream_array_size / stream_nb_threads;

  stream_a = t1;
  stream_b = t2;
  stream_c = t3;
}

/* Copy stream_a into stream_c */
void STREAM_copy () {
  unsigned int j;
  unsigned int thread_id = marcel_self ()->id;
  for (j = thread_id * stream_chunk_size; j < (thread_id + 1) * stream_chunk_size; j++)
    stream_c[j] = stream_a[j];
}

/* Multiply stream_c with _scalar_ and store the resulting array into
   stream_b */
void STREAM_scale (double scalar) {
  unsigned int j;
  unsigned int thread_id = marcel_self ()->id;
  for (j = thread_id * stream_chunk_size; j < (thread_id + 1) * stream_chunk_size; j++)
    stream_b[j] = scalar * stream_c[j];
}

/* Add stream_a to stream_b into stream_c */
void STREAM_add () {
  unsigned int j;
  unsigned int thread_id = marcel_self ()->id;
  for (j = thread_id * stream_chunk_size; j < (thread_id + 1) * stream_chunk_size; j++)
    stream_c[j] = stream_a[j] + stream_b[j];
}

/* Add stream_b to scalar * stream_c into stream_c */
void STREAM_triad (double scalar) {
  unsigned int j;
  unsigned int thread_id = marcel_self ()->id;
  for (j = thread_id * stream_chunk_size; j < (thread_id + 1) * stream_chunk_size; j++)
    stream_a[j] = stream_b[j] + scalar * stream_c[j];
}

#endif /* MARCEL_STREAM_H */
