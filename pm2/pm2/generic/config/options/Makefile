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

LIBS := pm2 marcel mad1 mad2 tbx ntbx common

all:

duplique_global:
	@for file in *.sh ; do \
		name=`basename $$file .sh` ; \
		echo "duplicating $$name" ; \
		for lib in $(LIBS); do \
			LIB=`echo $$lib | tr [a-z] [A-Z]`; \
			case $$LIB in \
			mad1|mad2) LIB=MAD ;; \
			esac ; \
			cp $$name.help ../../$$lib/config/options ; \
			sed -e "s,LIB,$$LIB,g" < $$file \
				> ../../$$lib/config/options/$$file ; \
		done ; \
	done


