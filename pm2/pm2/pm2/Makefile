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

LIBRARY=pm2
#LIBNAME=marcel

#marcel_CC=gcc
#marcel_AS=gcc

include $(PM2_ROOT)/make/config.mak
include $(PM2_ROOT)/make/libs-vars.mak

include $(PM2_MAK_DIR)/dsm-specific.mak

LIB_REP_TO_BUILD += $(LIB_GEN_INC)

COMMON_DEPS += $(LIB_GEN_INC)/dsm_page_size.h 

clean: clean-bin

include $(PM2_ROOT)/make/libs-rules.mak

$(LIB_GEN_INC)/dsm_page_size.h:	
	$(LIB_PREFIX) $(CC) $(CURDIR)/bin/src/dsm_page_size.c -o $(LIB_GEN_OBJ)/dsm_page_size${LIB_EXT}
	$(COMMON_HIDE) echo "#define DSM_PAGE_SIZE `$(LIB_GEN_OBJ)/dsm_page_size$(LIB_EXT)`" > $(LIB_GEN_INC)/dsm_page_size.h
	$(COMMON_HIDE) echo "#ifndef IRIX_SYS" >> $(LIB_GEN_INC)/dsm_page_size.h
	$(COMMON_HIDE) echo "#define DSM_ASM_PAGEALIGN \".align `$(LIB_GEN_OBJ)/dsm_page_size$(LIB_EXT)`\"" >> $(LIB_GEN_INC)/dsm_page_size.h
	$(COMMON_HIDE) echo "#else" >> $(LIB_GEN_INC)/dsm_page_size.h
	$(COMMON_HIDE) echo "#define DSM_ASM_PAGEALIGN \".align 16\"" >> $(LIB_GEN_INC)/dsm_page_size.h
	$(COMMON_HIDE) echo "#endif" >> $(LIB_GEN_INC)/dsm_page_size.h

clean-bin:
	$(COMMON_CLEAN) $(RM) $(LIB_GEN_OBJ)/dsm_page_size$(LIB_EXT) $(LIB_GEN_INC)/dsm_page_size.h

$(PM2_MAK_DIR)/dsm-specific.mak: $(LIB_STAMP_FLAVOR)
	@_PM2_OUTPUT=`$(PM2_CONFIG) --includedir $(LIBRARY)`; \
	if [ -n "$$_PM2_OUTPUT" ]; then mkdir -p `dirname $@`; \
	echo "LIB_GEN_INC := $$_PM2_OUTPUT" > $@ ;\
	fi;
