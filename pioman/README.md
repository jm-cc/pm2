# 
README for PIOMan
=================

This document describes pioman installation and configuration.

for any question, mailto: Alexandre.Denis@inria.fr

for information on what pioman is, see http://runtime.bordeaux.inria.fr/pioman/

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
  In this case, pioman is not visible to th end-user.


Documentation
-------------

- To locally generate doxygen documentation:

      % cd $prefix/build/pioman
      % make docs

- It is also available online at http://pm2.gforge.inria.fr/pioman/doc/

- Reference API documentation: \ref pioman
