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

default: all

include ../make/common.mak

all: pm2 xpm2

# executables :


# console :

$(PM2_OBJ)/pm2.o: pm2.c $(PM2_HEADERS)
	$(PM2_CC2)

pm2: $(PM2_OBJ)/pm2.o $(PM2_LIBS)
	$(PM2_LINK) $(PM2_LLIBS)
	$(PM2_MOVE)


# Xconsole :

$(PM2_OBJ)/gestion_listes.o: gestion_listes.c gestion_listes.h structures.h
	$(PM2_XCC)

$(PM2_OBJ)/lire_console.o: lire_console.c lire_console.h gestion_listes.h structures.h
	$(PM2_XCC)

$(PM2_OBJ)/xpm2.o: xpm2.c old-lire_console.c
	$(PM2_XCC)

xpm2: $(PM2_OBJ)/xpm2.o
	$(PM2_XLINK) $(PM2_XLLIBS)
	$(PM2_MOVE)


$(PM2_OBJ)/xpm2b.o: xpm2b.c lire_console.h gestion_listes.c structures.h
	$(PM2_CC) $(PM2_CFLAGS) $(PM2_AFLAGS) -I$$OPENWINHOME/include -c $(X11_INC) -o $@ $(@F:.o=.c)

xpm2b: $(PM2_OBJ)/xpm2b.o $(PM2_OBJ)/lire_console.o $(PM2_OBJ)/gestion_listes.o
	$(PM2_XLINK) $(PM2_OBJ)/lire_console.o $(PM2_OBJ)/gestion_listes.o $(PM2_XLLIBS)
	$(PM2_MOVE)
