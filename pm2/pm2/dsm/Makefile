#
#                      PM2 HIGH-PERF/ISOMALLOC
#           High Performance Parallel Multithreaded Machine
#                           version 2.0
#
#             Gabriel Antoniu, Luc Bouge, Christian Perez,
#                Jean-Francois Mehaut, Raymond Namyst
#
#            Laboratoire de l'Informatique du Parallelisme
#                        UMR 8512 CNRS-INRIA
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

DSM_OPTIONS = 

DSM_SRC		:= $(DSM_ROOT)/source
DSM_DEP		:= $(DSM_SRC)/depend
DSM_OBJ		:= $(DSM_SRC)/obj/$(PM2_ARCH_SYS)
DSM_INC		:= $(DSM_ROOT)/include
DSM_BIN		:= $(DSM_ROOT)/bin
DSM_LIBD	:= $(DSM_ROOT)/lib/$(PM2_ARCH_SYS)

DSM_MAKEFILE	= $(DSM_ROOT)/Makefile \
		  $(DSM_INC)/dsm_page_size.h

ifneq ($(wildcard $(DSM_DEP)),$(DSM_DEP))
DUMMY_BUILD	:=	$(shell mkdir -p $(DSM_DEP))
endif

ifneq ($(wildcard $(DSM_OBJ)),$(DSM_OBJ))
DUMMY_BUILD	:=	$(shell mkdir -p $(DSM_OBJ))
endif

ifneq ($(wildcard $(DSM_LIBD)),$(DSM_LIBD))
DUMMY_BUILD	:=	$(shell mkdir -p $(DSM_LIBD))
endif

DSM_CC		=	gcc
DSM_AS		:=	$(DSM_CC)

DSM_CFLAGS	=	$(DSM_OPTIONS) $(PM2_CFLAGS) -I$(DSM_INC)
DSM_LDFLAGS	=	
DSM_ASFLAGS	=	-D$(PM2_ARCH) -D$(PM2_SYS)

DSM_LIB		=	$(DSM_LIBD)/libdsm.a
DSM_LLIB	=	

DSM_C_SOURCES	:=	$(wildcard $(DSM_SRC)/*.c)
DSM_S_SOURCES	:=	$(wildcard $(DSM_SRC)/*.S)

DSM_C_OBJECTS	:=	$(patsubst %.c,%.o,$(subst $(DSM_SRC),$(DSM_OBJ),$(DSM_C_SOURCES)))
DSM_S_OBJECTS	:=	$(patsubst %.S,%.o,$(subst $(DSM_SRC),$(DSM_OBJ),$(DSM_S_SOURCES)))
DSM_OBJECTS	:=	$(DSM_C_OBJECTS) $(DSM_S_OBJECTS)

DSM_C_DEPENDS	:=	$(patsubst %.o,%.d,$(subst $(DSM_OBJ),$(DSM_DEP),$(DSM_C_OBJECTS)))
DSM_S_DEPENDS	:=	$(patsubst %.o,%.d,$(subst $(DSM_OBJ),$(DSM_DEP),$(DSM_S_OBJECTS)))
DSM_DEPENDS	:=	($strip $(DSM_C_DEPENDS) $(DSM_S_DEPENDS))

DSM_DEP_TO_OBJ	=	$(DSM_OBJ)/$(patsubst %.d,%.o,$(notdir $@))
DSM_OBJ_TO_S	=	$(DSM_SRC)/$(patsubst %.o,%.s,$(notdir $@))

.PHONY: dsm_default
dsm_default: $(DSM_LIB)

ifneq ($(MAKECMDGOALS),clean)
ifeq ($(wildcard $(DSM_DEPENDS)),$(DSM_DEPENDS))
include $(DSM_DEPENDS)
endif
endif

$(DSM_OBJECTS): $(DSM_OBJ)/%.o: $(DSM_DEP)/%.d
$(DSM_DEPENDS): $(PM2_MAKEFILE)

$(DSM_LIB): $(DSM_OBJECTS)
	rm -f $(DSM_LIB)
	ar cr $(DSM_LIB) $(DSM_OBJECTS)


$(DSM_C_OBJECTS): $(DSM_OBJ)/%.o: $(DSM_SRC)/%.c
	$(DSM_CC) $(DSM_CFLAGS) -c $< -o $@

$(DSM_C_DEPENDS): $(DSM_DEP)/%.d: $(DSM_SRC)/%.c
	$(SHELL) -ec '$(DSM_CC) -MM $(DSM_CFLAGS) $< \
		| sed '\''s/.*:/$(subst /,\/,$(DSM_DEP_TO_OBJ)) $(subst /,\/,$@) :/g'\'' > $@'

$(DSM_S_OBJECTS): $(DSM_OBJ)/%.o: $(DSM_SRC)/%.S
	$(DSM_CC) -E -P $(DSM_ASFLAGS) $< > $(DSM_OBJ_TO_S)
	$(DSM_AS) $(DSM_ASFLAGS) -c $(DSM_OBJ_TO_S) -o $@
	rm -f $(DSM_OBJ_TO_S)

$(DSM_S_DEPENDS): $(DSM_DEP)/%.d: $(DSM_SRC)/%.S
	$(SHELL) -ec '$(DSM_CC) -MM $(DSM_CFLAGS) $< \
		| sed '\''s/.*:/$(subst /,\/,$(DSM_DEP_TO_OBJ)) $(subst /,\/,$@) :/g'\'' > $@'


$(DSM_INC)/dsm_page_size.h: $(DSM_INC)/$(PM2_ARCH_SYM)_dsm_page_size.h
	cd $(DSM_INC) ; ln -f $(PM2_ARCH_SYM)_dsm_page_size.h dsm_page_size.h

$(DSM_INC)/$(PM2_ARCH_SYM)_dsm_page_size.h:
	$(DSM_CC) $(DSM_BIN)/src/dsm_page_size.c -o $(DSM_BIN)/dsm_page_size
	echo "#define DSM_PAGE_SIZE `$(DSM_BIN)/dsm_page_size`" > $(DSM_INC)/$(PM2_ARCH_SYM)_dsm_page_size.h
	echo "#define DSM_ASM_PAGEALIGN \".align `$(DSM_BIN)/dsm_page_size`\"" >> $(DSM_INC)/$(PM2_ARCH_SYM)_dsm_page_size.h
	rm -f $(DSM_BIN)/dsm_page_size


ifdef PM2_FLAGS
.PHONY: dsmclean
dsmclean:
	rm -f $(wildcard $(DSM_LIBD)/*.a $(DSM_OBJ)/*.o $(DSM_DEP)/*.d \
		$(DSM_INC)/*dsm_page_size.h)

dsmdistclean:
	rm -rf $(wildcard $(DSM_ROOT)/lib \
		$(DSM_ROOT)/source/obj \
		$(DSM_ROOT)/source/depend \
		$(DSM_INC)/*dsm_page_size.h)
else
.PHONY: clean distclean
clean:
	rm -f $(wildcard $(DSM_LIBD)/*.a $(DSM_OBJ)/*.o $(DSM_DEP)/*.d \
		$(DSM_INC)/*dsm_page_size.h)

distclean:
	rm -rf $(wildcard $(DSM_ROOT)/lib \
		$(DSM_ROOT)/source/obj \
		$(DSM_ROOT)/source/depend \
		$(DSM_INC)/*dsm_page_size.h)
endif

