#!/bin/sh
#
#                      PM2 HIGH-PERF/ISOMALLOC
#           High Performance Parallel Multithreaded Machine
#                           version 3.0
#
#     Gabriel Antoniu, Olivier Aumage, Luc Bouge, Vincent Danjean,
#       Christian Perez, Jean-Francois Mehaut, Raymond Namyst
#
#            Laboratoire de l'Informatique du Parallelisme
#                        UMR 5668 CNRS-INRIA
#                 Ecole Normale Superieure de Lyon
#
#                      External Contributors:
#                 Yves Denneulin (LMC - Grenoble),
#                 Benoit Planquelle (LIFL - Lille)
#
#                    1998 All Rights Reserved
#
#
#                             NOTICE
#
# Permission to use, copy, modify, and distribute this software and
# its documentation for any purpose and without fee is hereby granted
# provided that the above copyright notice appear in all copies and
# that both the copyright notice and this permission notice appear in
# supporting documentation.
#
# Neither the institutions (Ecole Normale Superieure de Lyon,
# Laboratoire de L'informatique du Parallelisme, Universite des
# Sciences et Technologies de Lille, Laboratoire d'Informatique
# Fondamentale de Lille), nor the Authors make any representations
# about the suitability of this software for any purpose. This
# software is provided ``as is'' without express or implied warranty.
#


flavor=$1

if [ $# -gt 1 ]; then
    target=$2
else
    target=all
fi
echo " gen_mak : $@ " >> $HOME/OUT

#############################################################
# Setting destination directory

make_dir() {
    ldir=$1

    if [ ! -d $ldir ]; then
#	echo "Crating directory : $ldir"
	mkdir $ldir
    fi
}

############################################################
# Generating Mafiles

gen_libs() {

LIBRARY=$1
dst=$2

cat > $dst <<EOF
LIB_GEN_LIB := `$PM2_CONFIG --libdir $LIBRARY`
LIB_GEN_OBJ := `$PM2_CONFIG --objdir $LIBRARY`
LIB_GEN_ASM := `$PM2_CONFIG --asmdir $LIBRARY`
LIB_GEN_DEP := `$PM2_CONFIG --depdir $LIBRARY`
LIB_GEN_STAMP := `$PM2_CONFIG --stampdir $LIBRARY`
LIB_STAMP_FLAVOR := `$PM2_CONFIG --stampfile`
LIB_STAMP_FILE := `$PM2_CONFIG --stampfile $LIBRARY`
LIB_EXT := `$PM2_CONFIG --ext $LIBRARY`

LIB_CFLAGS := `$PM2_CONFIG --kernel --cflags $LIBRARY`
CFLAGS += \$(LIB_CFLAGS)

CC := `$PM2_CONFIG --cc $LIBRARY`
EOF

}

## App

gen_app() {

LIBRARY=$1
dst=$2

cat > $dst <<EOF

APP_STAMP := `$PM2_CONFIG --stampdir`
APP_EXT := `$PM2_CONFIG --ext`
APP_LOADER := `$PM2_CONFIG --loader`

CC := `$PM2_CONFIG --cc $LIBRARY`

APP_CFLAGS += `$PM2_CONFIG --cflags`
APP_LDFLAGS += `$PM2_CONFIG --libs`
CFLAGS += \$(APP_CFLAGS)
LDFLAGS += \$(APP_LDFLAGS)
APP_LLIBS += `$PM2_CONFIG --libs-only-l`
PM2_MODULES += `$PM2_CONFIG --modules`
APP_STAMP_FILES := `$PM2_CONFIG --stampfile`
APP_STAMP_FILES += `$PM2_CONFIG --stampfile all`
THIS_DIR := \$(shell basename `pwd`)
ifeq (\$(THIS_DIR),examples)
APP_BUILDDIR := `$PM2_CONFIG --builddir`/examples
else
APP_BUILDDIR := `$PM2_CONFIG --builddir`/examples/\$(THIS_DIR)
endif

APP_BIN := \$(APP_BUILDDIR)/bin
APP_OBJ := \$(APP_BUILDDIR)/obj
APP_ASM := \$(APP_BUILDDIR)/asm
APP_DEP := \$(APP_BUILDDIR)/dep
EOF

}

############################################################
# Main

PM2_CONFIG="pm2-config --flavor=$flavor"

# Gerating directories

dir=`$PM2_CONFIG --builddir`

destdir=$dir/mak

LIBRARIES=`$PM2_CONFIG --modules`

# Generating .mak for libraries

make_dir $dir
make_dir $dir/mak

if [ $target = dir ]; then
    exit 0
fi

if [ $target = app ]; then 
    gen_app $flavor  $destdir/app.mak
    exit 0
fi

if [ $target = all -o $target = main-libs ]; then
    echo "LIBS := `$PM2_CONFIG --modules`" > $destdir/main-libs.mak
    exit 0
fi

for i in $LIBRARIES; do
    if [ $target = all -o $target = $i ]; then
	gen_libs $i $destdir/$i.mak
    fi
done


