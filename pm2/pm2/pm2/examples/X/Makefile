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

CONFIG_MODULES=marcel pm2 mad2 tbx ntbx

ifndef FLAVOR
export FLAVOR:=default
endif

CFLAGS += -I/usr/X11R6/include -I$$OPENWINHOME/include
LDFLAGS += -L/usr/X11R6/lib -L$$OPENWINHOME/lib -lxview -lolgx -lX11

PROGS := polygon

include $(PM2_ROOT)/make/config.mak
include $(PM2_ROOT)/make/apps.mak

$(APP_BIN)/polygon$(APP_EXT): $(APP_OBJ)/regul$(APP_EXT).o $(APP_OBJ)/custom$(APP_EXT).o

