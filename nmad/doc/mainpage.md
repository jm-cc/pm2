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
  - The \ref mpi_interface known as Mad-MPI, which is a regular MPI implementation

or the low-level APIs:
  - The \ref sr_interface as general purpose send receive
  - The \ref launcher_interface to interface with launcher to initialize sessions
  - The \ref pack_interface for compatibility with legacy Mad3


Getting the source code
-----------------------

NewMadeleine files are hosted by the PM2 project on the
InriaGforge at https://gforge.inria.fr/projects/pm2/

The source code is managed by a *Subversion* server hosted by
the InriaGforge. Anonymous SVN access is available, so that you
don't need to become a project member to access the repository
with the latest source. Alternatively, you can download a
released PM2 tarball.

Prerequisites
-------------

The following development tools are required to compile NewMadeleine:

- GNU C Compiler `gcc` (version 3.2 and higher).
- GNU `make` (version 3.81 and higher).
- To use InfiniBand, you must get the OFED drivers stack.

Installation
------------

For up to date installation details see \subpage README.md 

