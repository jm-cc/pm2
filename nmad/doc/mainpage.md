The NewMadeleine API Documentation
==================================

Introduction
------------

NewMadeleine is a complete redesign and rewrite of the
communication library Madeleine. The new architecture aims at
enabling the use of a much wider range of communication flow
optimization techniques. It is entirely modular: The request
scheduler itself is interchangeable, allowing experimentations
with multiple approaches or on multiple issues with regard to
processing communication flows. We have implemented an
optimizing scheduler called SchedOpt. SchedOpt targets
applications with irregular, multi-flow communication schemes such
as found in the increasingly common application conglomerates made
of multiple programming environments and coupled pieces of code,
for instance. SchedOpt itself is easily extensible through the
concepts of optimization strategies  (what to optimize for, what
the optimization goal is) expressed in terms of tactics (how to
optimize to reach the optimization goal). Tactics themselves are
made of basic communication flows operations such as packet
merging or reordering.

More information on NewMadeleine can be
 found at http://runtime.bordeaux.inria.fr/newmadeleine/.


User APIs
---------

Several APIs are provided to NewMadeleine users:
  - The \ref pack_interface
  - The \ref sr_interface
  - The \ref mpi_interface known as Mad-MPI

