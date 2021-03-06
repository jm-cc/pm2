\page README README

NewMadeleine README
===================

This document describes nmad installation and configuration.

for any question, mailto: Alexandre.Denis@inria.fr

Installation
------------

Requirements:
  - autoconf (v 2.50 or later)
  - pkg-config
  - hwloc (optional, recommended)
  - libexpat XML parser
  - ibverbs/OFED for IB support (set $IBHOME if not installed in /usr)
  - MX for Myrinet support (set $MX_DIR if not installed in /usr)

**Automated build (recommended)**:
To build all modules required by nmad, we recommend to use the build
script located in pm2/scripts/pm2-build-packages using a given or a
custom configuration file, e.g.:

     % cd pm2/scripts
     % ./pm2-build-packages ./madmpi.conf --prefix=$HOME/soft/x86_64


**Manual build (not recommended, advanced users only)**.

Module nmad requires other pm2 modules: tbx, Puk, PukABI (optionnal),
pioman (optionnal). PadicoTM is the prefered launcher, and should be
built *after* nmad.

For each module:

    ./autogen.sh
    mkdir build ; cd build
    ../configure [your options here]
    make
    make install

*Note*: nmad purposely cannot be configured in its source
 directory. Please use a separate build directory.

Usefull configure flags (see ./configure --help)

    --enable-taghuge        Enable 64 bit tags
    --enable-tagarray       Enable 8 bit tags in flat array
    --enable-sampling       Enable network sampling
    --enable-nuioa          Enable NUMA control on I/O
    --enable-mpi            Enable builtin MPI implementation MadMPI
    --with-pioman           use pioman I/O manager [default=no]
    --with-ibverbs          use Infiniband ibverbs [default=check]
    --with-mx               use Myrinet MX [default=check]


Building application code
-------------------------

For an MPI applicatiion using MadMPI, use the standard `mpicc`,
`mpif77` and `mpif90` compiler frontends to build and link.

To build an application using native NewMadeleine interface, get the
required flags through pkg-config. For CFLAGS:

    % pkg-config --cflags nmad

For libraries:

    % pkg-config --libs nmad


Launcher
--------

For MadMPI use the standard `mpirun` as launcher. Please see `mpirun --help`
for up-to-date documentation. Please note that MadMPI `mpirun` is a
frontend to `padico-launch` so it accepts all options described below.

For native NewMadeleine applications, it is recommended to use
`padico-launch` as a launcher for nmad. It accepts parameters similar
to `mpirun`. Please see `padico-launch --help` for up-to-date
documentation. For example:

    % padico-launch -n 2 -nodelist jack0,jack1 nm_bench_sendrecv

starts program 'nm_bench_sendrecv' on hosts jack0 and jack1, using
auto-detected network.

Environment variables may be set using -D parameters, e.g.:

    % padico-launch -c -p -n 2 -nodelist jack0,jack1 -DNMAD_DRIVER=ib+mx -DNMAD_STRATEGY=split_balance nm_bench_sendrecv

starts program 'nm_bench_sendrecv' on hosts jack0 and jack1, using multi-rail
over Infiniband and Myrinet, using one console per process.

Advanced Tuning
---------------

### Strategy

The strategy used by nmad is selected using the following rules:

1. if the environment variable `NMAD_STRATEGY` is set, it is used
whatever the other configuration parameters are.

2. if the variable is not set, strategy 'aggreg' is used by default.

Valid strategies are: 
  default, aggreg, aggreg_autoextended, split_balance.

The following are deprecated/unmatained:
  split_all, qos

The default choice should fit most cases.

### Drivers

The drivers used by nmad are selected using the following rules:

1. if the environment variable NMAD_DRIVER is set, it is used by
default. It may contain the name of a single driver for single rail,
or a list of multiple drivers separated by '+' for multirail,
e.g. NMAD_DRIVER=mx+ibverbs
An optionnal board number may be given if multiple boards with the
same driver are present, e.g. NMAD_DRIVER=ibverbs:0+ibverbs:1 for
dual-IB.

2. if nmad is launched mpirun or padico-launch, then PadicoTM
default NetSelector rules apply:
  + intra-process uses 'self'
  + intra-node inter-process uses 'shm'
  + inter-node uses 'ibverbs' if InfiniBand is present (auto-detected)
    and nodes are on the same IB subnet (same subnet manager). Thus it
    is important to configure subnet GID prefix and not keep the
    factory default GID prefix in opensm.
  + inter-node uses 'mx' if Myrinet is auto-detetected and nodes are
    on the same Myrinet network (same mapper).
  + inter-node uses 'tcp' if IP is available, and neither IB nor MX
    are available.
  + as last resort, routed messages over control channel is used if
    no direct connection is possible.
Usual NetSelector customization is possible.

3. if nmad is launched through the cmdline launcher, then a
"-R <string>" parameter is taken as a railstring, with the same syntax
as NMAD_DRIVER. Please note that cmdline launcher is only for debug
purpose and manages only 2 nodes.

4. if another custom launcher is used, it may set a selector using the
'sesion' interface.

5. in any other case, 'self' is used for intra-process; 'tcp' for
inter-process.


### Infiniband tuning

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


### Tags

The tag width may be configured as follows:
- tag_as_flat_array: tags are 8 bits, stored in flat arrays.
- tag_as_hashtable: tags are 32 bits, stored in hashtables. It is
  expected that the tags actually used will be sparse.
- tag_huge: users tags are 64 bits + 32 bits session tag added by
  nmad core, stored in hashtables with indirect hashing. Performance
  penalty is ~250ns compared to flat_array.

*Note*: only 'tag_huge' supports multiple sessions, and is mandatory for MadMPI.

Configure with --enable-taghuge for tag_huge, --enable-tagarray for
tag_array. Default is tag_as_hashtable.


Documentation
-------------

To generate doxygen documentation:

    % cd $prefix/build/nmad
    % make docs

It is available online at http://pm2.gforge.inria.fr/newmadeleine/doc/

