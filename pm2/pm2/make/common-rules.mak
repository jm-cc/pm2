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

# Regle par defaut : aide
#---------------------------------------------------------------------
help:

# Declarations Phony
#---------------------------------------------------------------------
.PHONY: all
.PHONY: help globalhelp bannerhelp targethelp commonoptionshelp
.PHONY: bannertargethelp optionshelp thankshelp tailhelp

# Aide
#--------------------------------------------------------------------
globalhelp: bannerhelp bannertargethelp targethelp commonoptionshelp optionshelp thankshelp tailhelp

# Note: interet ?
bannerhelp:

# Note: interet ?
targethelp:

bannertargethelp:
	@echo ""
	@echo "Targets:"

commonoptionshelp:
	@echo
	@echo "Common options:"
	@echo "  FLAVOR=flavor: build for flavor 'flavor'"
	@echo "  BUILD_STATIC_LIBS=true: build static libraries"
	@echo "  BUILD_DYNAMIC_LIBS=true: build dynamic libraries"
	@echo "  SHOW_FLAGS=true: show CFLAGS used"
	@echo "  SHOW_FLAVOR=true: show flavor target"
	@echo "  VERB=verbose|normal|quiet|silent: vebosity of building process"
	@echo
	@echo "Most of these options can be given with command line or"
	@echo "configurated in $(PM2_ROOT)/make/config.mak"

# Note: interet ?
suboptionshelp:

thankshelp:
	@echo
	@echo "Troubles ? Contact Luc.Bouge@ens-lyon.fr"
	@echo "Have a good day with PM2"

# Note: interet ?
tailhelp:

# Construction du repertoire destination des makefiles generes
#---------------------------------------------------------------------
$(PM2_MAK_DIR):
	@mkdir -p $@
