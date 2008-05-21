#!/bin/bash
############

DIR=$(pm2-config --builddir)
LIBS=$(pm2-config --libs-only-l)
gcc -nodefaultlibs ${DIR}/examples/obj/$1.o \
-L${DIR}/lib -L ${DYLD_LIBRARY_PATH} \
${LIBS} -lc -limf -lirc -lirc_s -o ${DIR}/examples/bin/$1

#
