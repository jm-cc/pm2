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
include ../make/common.mak

# objets :

$(MAR_OBJ)/sum.o: $(MAR_SRC)/sum.c $(MAR_HEADERS)
	$(MAR_CC2)

$(MAR_OBJ)/sumtime.o: $(MAR_SRC)/sumtime.c $(MAR_HEADERS)
	$(MAR_CC2)

$(MAR_OBJ)/sumdyn.o: $(MAR_SRC)/sumdyn.c $(MAR_HEADERS)
	$(MAR_CC2)

$(MAR_OBJ)/sample.o: $(MAR_SRC)/sample.c $(MAR_HEADERS)
	$(MAR_CC2)

$(MAR_OBJ)/delay.o: $(MAR_SRC)/delay.c $(MAR_HEADERS)
	$(MAR_CC2)

$(MAR_OBJ)/static.o: $(MAR_SRC)/static.c $(MAR_HEADERS)
	$(MAR_CC2)

$(MAR_OBJ)/keys.o: $(MAR_SRC)/keys.c $(MAR_HEADERS)
	$(MAR_CC2)

$(MAR_OBJ)/cleanup.o: $(MAR_SRC)/cleanup.c $(MAR_HEADERS)
	$(MAR_CC2)

$(MAR_OBJ)/userspace.o: $(MAR_SRC)/userspace.c $(MAR_HEADERS)
	$(MAR_CC2)

$(MAR_OBJ)/usersched.o: $(MAR_SRC)/usersched.c $(MAR_HEADERS)
	$(MAR_CC2)

$(MAR_OBJ)/prio.o: $(MAR_SRC)/prio.c $(MAR_HEADERS)
	$(MAR_CC2)

$(MAR_OBJ)/changeprio.o: $(MAR_SRC)/changeprio.c $(MAR_HEADERS)
	$(MAR_CC2)

$(MAR_OBJ)/active.o: $(MAR_SRC)/active.c $(MAR_HEADERS)
	$(MAR_CC2)

$(MAR_OBJ)/derout.o: $(MAR_SRC)/derout.c $(MAR_HEADERS)
	$(MAR_CC2)

$(MAR_OBJ)/contsw.o: $(MAR_SRC)/contsw.c $(MAR_HEADERS)
	$(MAR_CC2)

$(MAR_OBJ)/create.o: $(MAR_SRC)/create.c $(MAR_HEADERS)
	$(MAR_CC2)

$(MAR_OBJ)/timeout.o: $(MAR_SRC)/timeout.c $(MAR_HEADERS)
	$(MAR_CC2)

$(MAR_OBJ)/testmig.o: $(MAR_SRC)/testmig.c $(MAR_HEADERS)
	$(MAR_CC2)

$(MAR_OBJ)/dump.o: $(MAR_SRC)/dump.c $(MAR_HEADERS)
	$(MAR_CC2)

$(MAR_OBJ)/clavier.o: $(MAR_SRC)/clavier.c $(MAR_HEADERS)
	$(MAR_CC2)

$(MAR_OBJ)/qsort.o: $(MAR_SRC)/qsort.c $(MAR_HEADERS)
	$(MAR_CC2)

$(MAR_OBJ)/stack.o: $(MAR_SRC)/stack.c $(MAR_HEADERS)
	$(MAR_CC2)

$(MAR_OBJ)/once.o: $(MAR_SRC)/once.c $(MAR_HEADERS)
	$(MAR_CC2)

$(MAR_OBJ)/force_raise.o: $(MAR_SRC)/force_raise.c $(MAR_HEADERS)
	$(MAR_CC2)

$(MAR_OBJ)/raise.o: $(MAR_SRC)/raise.c $(MAR_HEADERS)
	$(MAR_CC2)

$(MAR_OBJ)/magic.o: $(MAR_SRC)/magic.c $(MAR_HEADERS)
	$(MAR_CC2)

$(MAR_OBJ)/clone.o: $(MAR_SRC)/clone.c $(MAR_SRC)/clone.h $(MAR_HEADERS)
	$(MAR_CC2)

$(MAR_OBJ)/eclone.o: $(MAR_SRC)/eclone.c $(MAR_SRC)/clone.h $(MAR_HEADERS)
	$(MAR_CC2)

$(MAR_OBJ)/poll.o: $(MAR_SRC)/poll.c $(MAR_HEADERS)
	$(MAR_CC2)

$(MAR_OBJ)/matrice.o: $(MAR_SRC)/matrice.c $(MAR_HEADERS)
	$(MAR_CC2)

$(MAR_OBJ)/daube.o: $(MAR_SRC)/daube.c $(MAR_HEADERS)
	$(MAR_CC2)

$(MAR_OBJ)/tube.o: $(MAR_SRC)/tube.c $(MAR_HEADERS)
	$(MAR_CC2)

$(MAR_OBJ)/stack_jump.o: $(MAR_SRC)/stack_jump.c $(MAR_HEADERS)
	$(MAR_CC2)

# tsp
$(MAR_OBJ)/tsp-job.o: $(MAR_SRC)/tsp-job.c $(MAR_HEADERS)
	$(MAR_CC2) -DMT -DMARCEL

$(MAR_OBJ)/tsp-main.o: $(MAR_SRC)/tsp-main.c $(MAR_HEADERS)
	$(MAR_CC2) -DMT -DMARCEL

$(MAR_OBJ)/tsp.o: $(MAR_SRC)/tsp.c $(MAR_HEADERS)
	$(MAR_CC2) -DMT -DMARCEL


# executables :

sum: $(MAR_OBJ)/sum.o $(MAR_LIB)
	$(MAR_LINK)
	$(MAR_MOVE)

sumtime: $(MAR_OBJ)/sumtime.o $(MAR_LIB)
	$(MAR_LINK)
	$(MAR_MOVE)

sumdyn: $(MAR_OBJ)/sumdyn.o $(MAR_LIB)
	$(MAR_LINK)
	$(MAR_MOVE)

static: $(MAR_OBJ)/static.o $(MAR_LIB)
	$(MAR_LINK)
	$(MAR_MOVE)

sample: $(MAR_OBJ)/sample.o $(MAR_LIB)
	$(MAR_LINK)
	$(MAR_MOVE)

delay: $(MAR_OBJ)/delay.o $(MAR_LIB)
	$(MAR_LINK)
	$(MAR_MOVE)

keys: $(MAR_OBJ)/keys.o $(MAR_LIB)
	$(MAR_LINK)
	$(MAR_MOVE)

cleanup: $(MAR_OBJ)/cleanup.o $(MAR_LIB)
	$(MAR_LINK)
	$(MAR_MOVE)

userspace: $(MAR_OBJ)/userspace.o $(MAR_LIB)
	$(MAR_LINK)
	$(MAR_MOVE)

usersched: $(MAR_OBJ)/usersched.o $(MAR_LIB)
	$(MAR_LINK)
	$(MAR_MOVE)

prio: $(MAR_OBJ)/prio.o $(MAR_LIB)
	$(MAR_LINK)
	$(MAR_MOVE)

changeprio: $(MAR_OBJ)/changeprio.o $(MAR_LIB)
	$(MAR_LINK)
	$(MAR_MOVE)

active: $(MAR_OBJ)/active.o $(MAR_LIB)
	$(MAR_LINK)
	$(MAR_MOVE)

derout: $(MAR_OBJ)/derout.o $(MAR_LIB)
	$(MAR_LINK)
	$(MAR_MOVE)

contsw: $(MAR_OBJ)/contsw.o $(MAR_LIB)
	$(MAR_LINK)
	$(MAR_MOVE)

create: $(MAR_OBJ)/create.o $(MAR_LIB)
	$(MAR_LINK)
	$(MAR_MOVE)

timeout: $(MAR_OBJ)/timeout.o $(MAR_LIB)
	$(MAR_LINK)
	$(MAR_MOVE)

testmig: $(MAR_OBJ)/testmig.o $(MAR_LIB)
	$(MAR_LINK)
	$(MAR_MOVE)

dump: $(MAR_OBJ)/dump.o $(MAR_LIB)
	$(MAR_LINK)
	$(MAR_MOVE)

clavier: $(MAR_OBJ)/clavier.o $(MAR_LIB)
	$(MAR_LINK)
	$(MAR_MOVE)

qsort: $(MAR_OBJ)/qsort.o $(MAR_LIB)
	$(MAR_LINK)
	$(MAR_MOVE)

stack: $(MAR_OBJ)/stack.o $(MAR_LIB)
	$(MAR_LINK)
	$(MAR_MOVE)

once: $(MAR_OBJ)/once.o $(MAR_LIB)
	$(MAR_LINK)
	$(MAR_MOVE)

force_raise: $(MAR_OBJ)/force_raise.o $(MAR_LIB)
	$(MAR_LINK)
	$(MAR_MOVE)

raise: $(MAR_OBJ)/raise.o $(MAR_LIB)
	$(MAR_LINK)
	$(MAR_MOVE)

magic: $(MAR_OBJ)/magic.o $(MAR_LIB)
	$(MAR_LINK)
	$(MAR_MOVE)

eclone: $(MAR_OBJ)/eclone.o $(MAR_OBJ)/clone.o $(MAR_LIB)
	$(MAR_LINK) $(MAR_OBJ)/clone.o
	$(MAR_MOVE)

poll: $(MAR_OBJ)/poll.o $(MAR_LIB)
	$(MAR_LINK)
	$(MAR_MOVE)

matrice: $(MAR_OBJ)/matrice.o $(MAR_LIB)
	$(MAR_LINK)
	$(MAR_MOVE)

daube: $(MAR_OBJ)/daube.o $(MAR_LIB)
	$(MAR_LINK)
	$(MAR_MOVE)

tube: $(MAR_OBJ)/tube.o $(MAR_LIB)
	$(MAR_LINK)
	$(MAR_MOVE)

stack_jump: $(MAR_OBJ)/stack_jump.o $(MAR_LIB)
	$(MAR_LINK)
	$(MAR_MOVE)

tsp: $(MAR_OBJ)/tsp.o $(MAR_OBJ)/tsp-main.o $(MAR_OBJ)/tsp-job.o $(MAR_LIB)
	$(MAR_CC) $(MAR_LFLAGS) -o $@ $(MAR_OBJ)/$(@F).o  $(MAR_OBJ)/tsp-main.o $(MAR_OBJ)/tsp-job.o $(MAR_LLIB) -lm
	$(MAR_MOVE)

