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
#                 Yves Denneulin (PM2_LMC - Grenoble),
#                 Benoit Planquelle (PM2_LIFL - Lille)
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
# Neither the institutions (PM2_Ecole Normale Superieure de Lyon,
# Laboratoire de L'informatique du Parallelisme, Universite des
# Sciences et Technologies de Lille, Laboratoire d'Informatique
# Fondamentale de Lille), nor the Authors make any representations
# about the suitability of this software for any purpose. This
# software is provided ``as is'' without express or implied warranty.
#


default: all

include $(PM2_ROOT)/make/common.mak

all: simple ping migr_hook


# executables :


# simple :

$(PM2_OBJ)/simple.o: simple.c $(PM2_HEADERS)
	$(PM2_CC2)

simple: $(PM2_OBJ)/simple.o $(PM2_LIBS)
	$(PM2_LINK) $(PM2_LLIBS)
	$(PM2_MOVE)

# ping :

$(PM2_OBJ)/ping.o: ping.c $(PM2_HEADERS)
	$(PM2_CC2)

ping: $(PM2_OBJ)/ping.o $(PM2_LIBS)
	$(PM2_LINK) $(PM2_LLIBS)
	$(PM2_MOVE)

# migr_hook :

$(PM2_OBJ)/migr_hook.o: migr_hook.c $(PM2_HEADERS)
	$(PM2_CC2)

migr_hook: $(PM2_OBJ)/migr_hook.o $(PM2_LIBS)
	$(PM2_LINK) $(PM2_LLIBS)
	$(PM2_MOVE)

