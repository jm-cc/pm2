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

###################### CUSTOMIZATION SECTION #####################

NTBX_OPTIONS		=  # -DDEBUG

NTBX_STANDALONE_OPTIONS	=	-g -Wall # -O6 -DMAD_TIMING -DUSE_SAFE_MALLOC

################### END OF CUSTOMIZATION SECTION #################

ifndef NTBX_ROOT
NTBX_ROOT	=	$(PM2_ROOT)/toolbox/net
endif

ifndef PM2_ARCH_SYS
PM2_ARCH	:=	$(shell $(PM2_ROOT)/bin/pm2_arch)
PM2_SYS		:=	$(shell $(PM2_ROOT)/bin/pm2_sys)
PM2_ARCH_SYS	:=	$(shell basename $(PM2_SYS) _SYS)/$(shell basename $(PM2_ARCH) _ARCH)
endif

NTBX_MAKEFILE	=	$(NTBX_ROOT)/Makefile

NTBX_OBJ        :=      $(NTBX_ROOT)/obj
NTBX_SRC	:=	$(NTBX_ROOT)/src
NTBX_DEP	:=	$(NTBX_SRC)/depend
NTBX_INC	:=	$(NTBX_ROOT)/include
NTBX_LIBD	=	$(NTBX_ROOT)/lib/$(PM2_ARCH_SYS)


ifneq ($(wildcard $(NTBX_DEP)),$(NTBX_DEP))
DUMMY_BUILD	:=	$(shell mkdir -p $(NTBX_DEP))
endif

ifneq ($(wildcard $(NTBX_OBJ)),$(NTBX_OBJ))
DUMMY_BUILD	:=	$(shell mkdir -p $(NTBX_OBJ))
endif

ifneq ($(wildcard $(NTBX_LIBD)),$(NTBX_LIBD))
DUMMY_BUILD	:=	$(shell mkdir -p $(NTBX_LIBD))
endif

NTBX_REG_SOURCES	:=	$(wildcard $(NTBX_SRC)/*.c)

NTBX_REG_OBJECTS	:=	$(patsubst %.c,%.o,$(subst $(NTBX_SRC),$(NTBX_OBJ),$(NTBX_REG_SOURCES)))
NTBX_OBJECTS	:=	$(NTBX_REG_OBJECTS) 

NTBX_LIB	=	$(NTBX_LIBD)/libntbx.a
NTBX_LLIB	=	$(ARCHDLIB)

NTBX_CC		=	gcc

NTBX_CFLAGS	=	$(NTBX_OPTIONS) $(PM2_FLAGS) \
			-I$(NTBX_INC) -I$(TBX_ROOT)/include -I$(MAD_ROOT)/include \
			-I$(MAR_INC) -I$(PM2_ROOT)/include

NTBX_LDFLAGS	=	

NTBX_REG_DEPENDS:=	$(patsubst %.o,%.d,$(subst $(NTBX_OBJ),$(NTBX_DEP),$(NTBX_REG_OBJECTS)))
NTBX_DEPENDS	:=	($strip $(NTBX_REG_DEPENDS))

NTBX_DEP_TO_OBJ	=	$(NTBX_OBJ)/$(patsubst %.d,%.o,$(notdir $@))

$(NTBX_OBJECTS): $(NTBX_OBJ)/%.o: $(NTBX_DEP)/%.d
$(NTBX_DEPENDS): $(PM2_MAKEFILE)

.PHONY: ntbx_default
ntbx_default: tbx_default $(NTBX_LIB)

$(NTBX_LIB): $(NTBX_OBJECTS)
	@ rm -f $(NTBX_LIB)
	@ echo "  -  $(@F)"
	@ ar cr $(NTBX_LIB) $(NTBX_OBJECTS)

$(NTBX_REG_OBJECTS): $(NTBX_OBJ)/%.o: $(NTBX_SRC)/%.c
	@ echo "  -  $(@F)"
	@ $(NTBX_CC) $(NTBX_CFLAGS) $(NTBX_FLAGS) -c $< -o $@

$(NTBX_REG_DEPENDS): $(NTBX_DEP)/%.d: $(NTBX_SRC)/%.c
	@ echo "  -  $(@F)"
	@ $(SHELL) -ec '$(NTBX_CC) -MM $(NTBX_CFLAGS) $(NTBX_FLAGS) $< \
		| sed '\''s/.*:/$(subst /,\/,$(NTBX_DEP_TO_OBJ)) $(subst /,\/,$@) :/g'\'' > $@'


ifdef PM2_FLAGS
.PHONY: ntbxclean
ntbxclean:
	@echo "Cleaning net toolbox"
	@rm -f $(wildcard $(NTBX_LIBD)/*.a $(NTBX_OBJ)/*.o $(NTBX_DEP)/*.d)

ntbxdistclean:
	rm -rf $(wildcard $(NTBX_ROOT)/lib \
		$(NTBX_ROOT)/obj \
		$(NTBX_ROOT)/source/depend)
else
.PHONY: clean distclean
clean:
	@echo "Cleaning net toolbox"
	@rm -f $(wildcard $(NTBX_LIBD)/*.a $(NTBX_OBJ)/*.o $(NTBX_DEP)/*.d) \

distclean:
	rm -rf $(wildcard $(NTBX_ROOT)/lib \
		$(NTBX_ROOT)/obj \
		$(NTBX_ROOT)/source/depend)
endif

