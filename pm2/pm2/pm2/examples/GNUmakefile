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

SUBDIR := acteurs  gimp-plugin  migration  simple  tree-rpc \
	X     dsm      isomalloc    rpc        tree

examples:

include $(PM2_ROOT)/make/common-rules.mak

.PHONY: all clean distclean examples
all clean distclean examples:
	@set -e;\
	for rep in $(SUBDIR); do \
		$(MAKE) -C $$rep $@; \
	done
all:
clean:
distclean:
examples:

SUBDIRSLASH := $(foreach DIR, $(SUBDIR), $(DIR)/)
.PHONY: $(SUBDIR) $(SUBDIRSLASH)
$(SUBDIR) $(SUBDIRSLASH):
	@$(MAKE) -C $@ examples

.PHONY: help bannerhelpapps targethelpapps
help: globalhelp

bannerhelp: bannerhelpapps

bannerhelpapps:
	@echo "This is PM2 Makefile for examples repertory"

targethelp: targethelpapps

DIRLIST:=$(foreach DIR,$(SUBDIR),$(DIR))
targethelpapps:
	@echo "  all|examples: build the examples in each directory"
	@echo "  help: this help"
	@echo "  clean: clean examples source tree for current flavor"
	@echo "  distclean: clean examples source tree for all flavors"
	@echo
	@echo "Repertories with examples:"
	@echo "  $(DIRLIST)"


