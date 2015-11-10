# -*- mode: Markdown;-*-
README for MadMPI benchmark
===========================

This document describes **MadMPI benchmark** installation and configuration.

For any question, mailto: `Alexandre.Denis@inria.fr`

For more information, see: http://pm2.gforge.inria.fr/mpibenchmark/


Quick Start
-----------

A quick cheat sheet for the impatient:

    ./configure
    make
    mpiexec -n 2 -host host1,host2 ./mpi_bench_overlap | tee out.dat

It runs from 10 minutes through 1h, depanding on network speed. Then build the
**performance report** using:

    ./mpi_bench_extract out.dat

It outputs data in `out.dat.d/`

Please send the `out.dat` to <Alexandre.Denis@inria.fr> to have it integrated on
the MadMPI benchmark web site.


Requirements
------------
  - MPI library
  - autoconf (v 2.50 or later, for svn users)
  - hwloc (optional)
  - gnuplot (optional)
  - doxygen (optional, for doc generation)
  

Installation
------------

MadMPI benchmark follows usual autoconf procedure:

    ./configure [your options here]
    make
    make install

The `make install` step is optional. The benchmark may be run from its
build directory.


Documentation
-------------

- Benchmarks may be run separetely (single benchmark per binary), or as
  a binary running a full series.

- For overlap benchmarks, run `mpi_bench_overlap` on 2 nodes, capture its
  standard output in a file, and pass this file to `mpi_bench_extract`.
  The processed data is outputed to a `${file}.d/` directory containing:

    + raw series for each packet size (files `${bench}-series/${bench}-s${size}.dat`)
    + 2D data formated to feed gnuplot pm3d graphs, joined with referece non-overlapped values
	  (files `${bench}-ref.dat`)
    + gnuplot scripts (files `${bench}.gp`)
    + individual graphs for each benchmark (files `${bench}.png`)
    + synthetic graphs (`all.png`)
    
- The benchmarks are:

    + `mpi_bench_sendrecv`: send/receive pingpong, used as a reference
    + `mpi_bench_noncontig`: send/receive pingpong with non-contiguous datatype, used as a reference
    + `mpi_bench_send_overhead`: processor time consumed on sender side to send data
      (the overhead from LogP). Usefull to explain overlap benchmarks.
    + `mpi_bench_overlap_sender`: overlap on sender side
      (i.e. MPI_Isend, computation, MPI_Wait), total time
    + `mpi_bench_overlap_recv`: overlap on receiver side
      (i.e. MPI_Irecv, computation, MPI_Wait), total time
    + `mpi_bench_overlap_bidir`: overlap on both sides
    + `mpi_bench_overlap_sender_noncontig`: overlap on sender side, with non-contiguous datatype
    + `mpi_bench_overlap_send_overhead`: overlap on sender side
      (i.e. MPI_Isend, computation, MPI_Wait), measure time on sender side only
    + `mpi_bench_overlap_Nload`: overlap on sender side, with multi-threaded computation load

