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

SWANN_OPTIONS		 = -g -Wall # -DDEBUG -O6 -DMAD_TIMING -DUSE_SAFE_MALLOC

################### END OF CUSTOMIZATION SECTION #################

ifndef SWANN_ROOT
SWANN_ROOT =  $(PM2_ROOT)/swann
endif

SWANN :=  yes

ifndef PM2_ARCH_SYS
PM2_ARCH     :=  $(shell $(PM2_ROOT)/bin/pm2_arch)
PM2_SYS	     :=  $(shell $(PM2_ROOT)/bin/pm2_sys)
PM2_ARCH_SYS :=  $(shell basename $(PM2_SYS) _SYS)/$(shell basename $(PM2_ARCH) _ARCH)
endif

SWANN_MAKEFILE =  $(SWANN_ROOT)/Makefile

SWANN_OBJ  :=  $(SWANN_ROOT)/obj
SWANN_SRC  :=  $(SWANN_ROOT)/src
SWANN_DEP  :=  $(SWANN_SRC)/depend
SWANN_INC  :=  $(SWANN_ROOT)/include
SWANN_BIND  =  $(SWANN_ROOT)/bin/$(PM2_ARCH_SYS)

ifneq ($(wildcard $(SWANN_DEP)),$(SWANN_DEP))
DUMMY_BUILD :=  $(shell mkdir -p $(SWANN_DEP))
endif

ifneq ($(wildcard $(SWANN_OBJ)),$(SWANN_OBJ))
DUMMY_BUILD :=  $(shell mkdir -p $(SWANN_OBJ))
endif

ifneq ($(wildcard $(SWANN_BIND)),$(SWANN_BIND))
DUMMY_BUILD :=  $(shell mkdir -p $(SWANN_BIND))
endif

ifndef TBX_ROOT
TBX_ROOT :=  $(SWANN_ROOT)/../toolbox
endif

ifndef NTBX_ROOT
NTBX_ROOT :=  $(TBX_ROOT)/net
endif

SWANN_SUB_MAKEFILES +=  $(TBX_ROOT)/Makefile $(NTBX_ROOT)/Makefile
SWANN_REG_SOURCES   :=  $(wildcard $(SWANN_SRC)/*.c)
SWANN_REG_OBJECTS   :=  $(patsubst %.c,%.o,$(subst $(SWANN_SRC),$(SWANN_OBJ),$(SWANN_REG_SOURCES)))
SWANN_OBJECTS       :=  $(SWANN_REG_OBJECTS)
SWANN_APP            =  $(SWANN_BIND)/swann

SWANN_REG_DEPENDS :=  $(patsubst %.o,%.d,$(subst $(SWANN_OBJ),$(SWANN_DEP),$(SWANN_REG_OBJECTS)))
SWANN_DEPENDS     :=  ($strip $(SWANN_REG_DEPENDS))
SWANN_DEP_TO_OBJ   =  $(SWANN_OBJ)/$(patsubst %.d,%.o,$(notdir $@))

MASTER         :=  swann
MASTER_CC      :=  gcc
MASTER_OPTIONS :=  $(SWANN_OPTIONS)
MASTER_DEP     :=  $(SWANN_MAKEFILE) $(SWANN_SUB_MAKEFILE)
MASTER_LDFLAGS := 

SWANN_CC      =  $(MASTER_CC)
SWANN_CFLAGS  =  $(MASTER_OPTIONS) $(PM2_FLAGS) \
		 -I$(SWANN_INC) -I$(TBX_ROOT)/include \
		 -I$(SWANN_ROOT)/include -I$(NTBX_ROOT)/include
SWANN_LDFLAGS =	$(MASTER_LDFLAGS)

ifneq ($(MAKECMDGOALS),clean)
ifeq ($(wildcard $(SWANN_DEPENDS)),$(SWANN_DEPENDS))
include $(SWANN_DEPENDS)
endif
endif

.PHONY: swann_default
swann_default: $(SWANN_APP) 

include $(SWANN_SUB_MAKEFILES)

$(SWANN_OBJECTS): $(SWANN_OBJ)/%.o: $(SWANN_DEP)/%.d
$(SWANN_DEPENDS): $(MASTER_DEP) 

$(SWANN_APP): $(SWANN_OBJECTS) tbx_default ntbx_default
	@ echo "  -  $(@F)"
	$(SWANN_CC) -o $(SWANN_APP) $(SWANN_OBJECTS) $(SWANN_LDFLAGS)

$(SWANN_REG_OBJECTS): $(SWANN_OBJ)/%.o: $(SWANN_SRC)/%.c
	@ echo "  -  $(@F)"
	$(SWANN_CC) $(SWANN_CFLAGS) $(SWANN_FLAGS) -c $< -o $@

$(SWANN_REG_DEPENDS): $(SWANN_DEP)/%.d: $(SWANN_SRC)/%.c
	@ echo "  -  $(@F)"
	$(SHELL) -ec '$(SWANN_CC) -MM $(SWANN_CFLAGS) $(SWANN_FLAGS) $< \
		| sed '\''s/.*:/$(subst /,\/,$(SWANN_DEP_TO_OBJ)) $(subst /,\/,$@) :/g'\'' > $@'

.PHONY: clean distclean
clean: tbxclean ntbxclean
	@ echo "Cleaning swann"
	rm -f $(wildcard $(SWANN_OBJ)/*.o $(SWANN_DEP)/*.d) 

distclean: tbxdistclean ntbxdistclean
	rm -rf $(wildcard $(SWANN_ROOT)/bin \
		$(SWANN_ROOT)/obj \
		$(SWANN_ROOT)/source/depend) 
	rm -f $(wildcard $(SWANN_OBJ)/*.o $(SWANN_DEP)/*.d) 
