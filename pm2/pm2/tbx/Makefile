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

TBX_OPTIONS		=  # -DDEBUG

TBX_STANDALONE_OPTIONS	=	-g -Wall # -O6 -DMAD_TIMING -DUSE_SAFE_MALLOC

################### END OF CUSTOMIZATION SECTION #################

ifndef TBX_ROOT
TBX_ROOT	=	$(PM2_ROOT)/toolbox
endif

ifndef PM2_ARCH_SYS
PM2_ARCH	:=	$(shell $(PM2_ROOT)/bin/pm2_arch)
PM2_SYS		:=	$(shell $(PM2_ROOT)/bin/pm2_sys)
PM2_ARCH_SYS	:=	$(shell basename $(PM2_SYS) _SYS)/$(shell basename $(PM2_ARCH) _ARCH)
endif

TBX_MAKEFILE	=	$(TBX_ROOT)/Makefile

TBX_OBJ         :=      $(TBX_ROOT)/obj
TBX_SRC		:=	$(TBX_ROOT)/src
TBX_DEP		:=	$(TBX_SRC)/depend
TBX_INC		:=	$(TBX_ROOT)/include
TBX_LIBD	=	$(TBX_ROOT)/lib/$(PM2_ARCH_SYS)


ifneq ($(wildcard $(TBX_DEP)),$(TBX_DEP))
DUMMY_BUILD	:=	$(shell mkdir -p $(TBX_DEP))
endif

ifneq ($(wildcard $(TBX_OBJ)),$(TBX_OBJ))
DUMMY_BUILD	:=	$(shell mkdir -p $(TBX_OBJ))
endif

ifneq ($(wildcard $(TBX_LIBD)),$(TBX_LIBD))
DUMMY_BUILD	:=	$(shell mkdir -p $(TBX_LIBD))
endif

TBX_REG_SOURCES	:=	$(wildcard $(TBX_SRC)/*.c)

TBX_REG_OBJECTS	:=	$(patsubst %.c,%.o,$(subst $(TBX_SRC),$(TBX_OBJ),$(TBX_REG_SOURCES)))
TBX_OBJECTS	:=	$(TBX_REG_OBJECTS) 

TBX_LIB		=	$(TBX_LIBD)/libtbx.a
TBX_LLIB	=	$(ARCHDLIB)

TBX_CC		=	gcc

TBX_CFLAGS	=	$(TBX_OPTIONS) $(PM2_FLAGS) \
			-I$(TBX_INC) -I$(MAD_ROOT)/include \
			-I$(MAR_INC) -I$(PM2_ROOT)/include

TBX_LDFLAGS	=	

TBX_REG_DEPENDS	:=	$(patsubst %.o,%.d,$(subst $(TBX_OBJ),$(TBX_DEP),$(TBX_REG_OBJECTS)))
TBX_DEPENDS	:=	($strip $(TBX_REG_DEPENDS))

TBX_DEP_TO_OBJ	=	$(TBX_OBJ)/$(patsubst %.d,%.o,$(notdir $@))

ifneq ($(MAKECMDGOALS),clean)
ifeq ($(wildcard $(TBX_DEPENDS)),$(TBX_DEPENDS))
include $(TBX_DEPENDS)
endif
endif

$(TBX_OBJECTS): $(TBX_OBJ)/%.o: $(TBX_DEP)/%.d
$(TBX_DEPENDS): $(PM2_MAKEFILE)

.PHONY: tbx_default
tbx_default: $(TBX_LIB) 

$(TBX_LIB): $(TBX_OBJECTS)
	@ rm -f $(TBX_LIB)
	@ echo "  -  $(@F)"
	@ ar cr $(TBX_LIB) $(TBX_OBJECTS)

$(TBX_REG_OBJECTS): $(TBX_OBJ)/%.o: $(TBX_SRC)/%.c
	@ echo "  -  $(@F)"
	@ $(TBX_CC) $(TBX_CFLAGS) $(TBX_FLAGS) -c $< -o $@

$(TBX_REG_DEPENDS): $(TBX_DEP)/%.d: $(TBX_SRC)/%.c
	@ echo "  -  $(@F)"
	@ $(SHELL) -ec '$(TBX_CC) -MM $(TBX_CFLAGS) $(TBX_FLAGS) $< \
		| sed '\''s/.*:/$(subst /,\/,$(TBX_DEP_TO_OBJ)) $(subst /,\/,$@) :/g'\'' > $@'


ifdef PM2_FLAGS
.PHONY: tbxclean
tbxclean:
	@ echo "Cleaning toolbox"
	@ rm -f $(wildcard $(TBX_LIBD)/*.a $(TBX_OBJ)/*.o $(TBX_DEP)/*.d)

tbxdistclean:
	rm -rf $(wildcard $(TBX_ROOT)/lib \
		$(TBX_ROOT)/obj \
		$(TBX_ROOT)/source/depend)
else
.PHONY: clean distclean
clean:
	@ echo "Cleaning toolbox"
	@ rm -f $(wildcard $(TBX_LIBD)/*.a $(TBX_OBJ)/*.o $(TBX_DEP)/*.d) \

distclean:
	rm -rf $(wildcard $(TBX_ROOT)/lib \
		$(TBX_ROOT)/obj \
		$(TBX_ROOT)/source/depend)
endif

