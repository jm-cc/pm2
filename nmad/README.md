\page README README

NewMadeleine README
===================

This document describes nmad installation and configuration.

for any question, mailto: Alexandre.Denis@inria.fr

Installation
------------

nmad requires other pm2 modules: tbx, Puk, PukABI, (optionnally) pioman.
PadicoTM may be used as a launcher, and should be built *after* nmad.

Requirements:
  - autoconf (v 2.50 or later)
  - pkg-config
  - hwloc (optional)
  - libnuma (optional)
  - ibverbs for IB support (set $IBHOME if not installed in /usr)
  - MX for Myrinet support (set $MX_DIR if not installed in /usr)

**Automated build** (recommended):
To build multiple modules at once, we recommend to use the build script
located in pm2/scripts/pm2-build-packages using a given or a custom
configuration file, e.g.:

     % cd pm2/scripts
     % ./pm2-build-packages ./bench-nmad+pioman+pthread.conf --prefix=$HOME/soft/x86_64


Manual build (not recommended). For each module:

    ./autogen.sh
    mkdir build ; cd build
    ../configure [your options here]
    make
    make install

*Note*: nmad purposely cannot be configured in its source directory

Usefull configure flags (see ./configure --help)

    --enable-taghuge        Enable 64 bit tags
    --enable-tagarray       Enable 8 bit tags in flat array
    --enable-sampling       Enable network sampling
    --enable-nuioa          Enable NUMA control on I/O
    --enable-mpi            Enable builtin MPI implementation Mad-MPI
    --with-pioman           use pioman I/O manager [default=no]
    --with-ibverbs          use Infiniband ibverbs [default=check]
    --with-mx               use Myrinet MX [default=check]


Building application code
-------------------------

To build an application using NewMadeleine, get the required flags
through pkg-config. For CFLAGS:

    % pkg-config --cflags nmad

For libraries:

    % pkg-config --libs nmad

For an MPI applicatiion using Mad-MPI, use the standard `mpicc`,
`mpif77` and `mpif90` compiler frontends to build and link.


Launcher
--------

If upper layers do not provide their own launcher, it is recommended
to use `padico-launch` as a launcher for nmad. It accepts parameters
similar to `mpirun`. Please see `padico-launch --help` for up-to-date
documentation.

Environment variables may be set using -D parameters, e.g.:

    % padico-launch -c -n 2 -nodelist jack0,jack1 -DNMAD_DRIVER=ib+mx -DNMAD_STRATEGY=split_balance sr_bench

starts program 'sr_bench' on hosts jack0 and jack1, using multi-rail
over Infiniband and Myrinet.

For Mad-MPI use the standard `mpirun` as launcher. Please see `mpirun --help`
for up-to-date documentation.


Strategy
--------

The strategy used by nmad is selected using the following rules:

1. if the environment variable `NMAD_STRATEGY` is set, it is used
whatever the other configuration parameters are.

2. if the variable is not set, strategy 'aggreg' is used by default.

Valid strategies are: 
  default, aggreg, aggreg_autoextended, split_balance.

The following are deprecated/unmatained:
  split_all, qos


Drivers
-------

The drivers used by nmad are selected using the following rules:

1. if the environment variable NMAD_DRIVER is set, it is used by
default. It may contain the name of a single driver for single rail,
or a list of multiple drivers separated by '+' for multirail,
e.g. NMAD_DRIVER=mx+ibverbs
An optionnal board number may be given if multiple boards with the
same driver are present, e.g. NMAD_DRIVER=ibverbs:0+ibverbs:1 for
dual-IB.

2. if nmad is launched through the newmadico nmad launcher (default
choice if session is started with padico-launch), then PadicoTM
default NetSelector rules apply:
  + intra-process uses 'self'
  + intra-node inter-process uses 'shm'
  + inter-node uses 'ibverbs' if InfinibandVerbs is loaded
    (i.e. "-net ib" was given to padico-launch) and nodes are on the
    same IB subnet (same subnet manager).
  + inter-node uses 'mx' if PSP-MX is loaded (i.e. "-net mx" was given
    to padico-launch) and nodes are on the same Myrinet network (same
    mapper).
  + inter-node uses 'tcp' if IP is available, and neither IB nor MX
    are available.
  + as last resort, routed messages over control channel is used if
    no direct connection is possible.
Usual NetSelector customization is possible.

3. if nmad is launched through the cmdline launcher, then a
"-R <string>" parameter is taken as a railstring, with the same syntax
as NMAD_DRIVER

4. if another launcher is used, it may set a selector using the
'sesion' interface.

5. in any other case, 'self' is used for intra-process; 'tcp' for
inter-process.


Infiniband tuning
-----------------

Infiniband may be tuned at run time through environment variables:
- NMAD_IBVERBS_RCACHE=1 enables the registration cache (requires PukABI)
- NMAD_IBVERBS_CHECKSUM=1 enables the checksum computation on the fly
   in the driver.

When `opensm` is used as subnet manager, subnet GID must be customized
with a value unique to the given subnet, so as nmad is able to
automatically detect IB connectivity. As root:
- create the default opensm config file:

      % opensm -o -c /var/cache/opensm/opensm.opts

- in the above file, customize the line with subnet_prefix to some
other value than the factory default 0xfe80000000000000. Set the same
subnet GID on all nodes of the subnet.
- restart opensm:

      % /etc/init.d/infiniband restart


Tags
----

- tag_as_flat_array: tags are 8 bits, stored in flat arrays.
- tag_as_hashtable: tags are 32 bits, stored in hashtables. It is
  expected that the tags actually used will be sparse.
- tag_huge: users tags are 64 bits + 32 bits session tag added by
  nmad core, stored in hashtables with indirect hashing. Performance
  penalty is ~250ns compared to flat_array.

*Note*: only 'tag_huge' supports multiple sessions, and is mandatory for Mad-MPI.

Configure with --enable-taghuge for tag_huge, --enable-tagarray for
tag_array. Default is tag_as_hashtable.


Documentation
-------------

To generate doxygen documentation:

    % cd $prefix/build/nmad
    % make docs

It is available online at http://pm2.gforge.inria.fr/newmadeleine/doc/

