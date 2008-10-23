/* marcel_stream interface, based on the STREAM benchmark
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
  unsigned int i;
  stream_nb_threads = nb_threads;
  stream_array_size = array_size;
  stream_chunk_size = stream_array_size / stream_nb_threads;

  stream_a = t1;
  stream_b = t2;
  stream_c = t3;

  for (i = 0; i < array_size; i++) {
    stream_a[i] = 1.0;
    stream_b[i] = 2.0;
    stream_c[i] = 0.0;
  }
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
