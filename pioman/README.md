# 
README for PIOMan
=================

This document describes pioman installation and configuration.

for any question, mailto: Alexandre.Denis@inria.fr

for information on what pioman is, see http://pm2.gforge.inria.fr/pioman/

Requirements
------------
  - autoconf (v 2.50 or later)
  - pkg-config
  - hwloc (optional)

Installation
------------

PIOMan may be used standalone or used through NewMadeleine.

- Standalone install follows usual autotools procedure:

      ./autogen.sh
      mkdir build ; cd build
      ../configure [your options here]
      make
      make install

  *Note*: pioman purposely cannot be configured in its source directory

- Through other PM2 modules: please see installation instructions for NewMadeleine
  http://pm2.gforge.inria.fr/newmadeleine/doc/
  In this case, pioman is not visible to the end-user.


Documentation
-------------

- To locally generate doxygen documentation:

      % cd $prefix/build/pioman
      % make docs

- It is also available online at http://pm2.gforge.inria.fr/pioman/doc/

- Reference API documentation: \ref pioman

- For advanced users, fine tuning is performed through environment variables:

  + PIOM_VERBOSE: set to 1 to display information messages in pioman init.
    default is: no display in quiet mode (padico-launch -q or mpirun), display else
  + PIOM_ENABLE_PROGRESSION: whether to enable asynchronous progression.
    default is 1.
  + PIOM_BUSY_WAIT_USEC: time to busy wait before passive wait, on explicit wait, in usec.
    default is 10; 0 disables busy-waiting; -1 does only busy waiting
  + PIOM_BUSY_WAIT_GRANULARITY: number of iterations between time check in busy, to amortize
    cost of clock_gettime. default is 100.
  + PIOM_IDLE_GRANULARITY: time between polling in idle threads, in usec.
    default is 20.
  + PIOM_IDLE_LEVEL: topology level where to bind idle threads.
    default is: socket.
  + PIOM_IDLE_DISTRIB: distribution for idle threads among topo entities of
    given level (all, odd, even, first). default is: all
  + PIOM_TIMER_PERIOD: period of timer-based polling, in usec.
    default is: 4000
  + PIOM_SPARE_LWP: number of spare LWP to export blocking calls.
    defaulty is 0.
  
    