

# PM2: Parallel Multithreaded Machine
# Copyright (C) 2001 "the PM2 team" (pm2-dev@listes.ens-lyon.fr)
# 
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or (at
# your option) any later version.
# 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.

LIBRARY=pm2

include $(PM2_ROOT)/make/libs-vars.mak

ifeq (,$(findstring _$(MAKECMDGOALS)_,$(DO_NOT_GENERATE_MAK_FILES)))
-include $(PM2_MAK_DIR)/pm2-specific.mak
endif

LIB_REP_TO_BUILD += $(LIB_GEN_INC)

dot_h: $(LIB_GEN_INC)/dsm_page_size.h

include $(PM2_ROOT)/make/libs-rules.mak

$(LIB_GEN_INC)/dsm_page_size.h:
	$(LIB_PREFIX) $(CC) $(CURDIR)/bin/src/dsm_page_size.c -o $(LIB_GEN_OBJ)/dsm_page_size${LIB_EXT}
	$(COMMON_HIDE) echo "#define DSM_PAGE_SIZE `$(LIB_GEN_OBJ)/dsm_page_size$(LIB_EXT)`" > $(LIB_GEN_INC)/dsm_page_size.h
	$(COMMON_HIDE) echo "#ifndef IRIX_SYS" >> $(LIB_GEN_INC)/dsm_page_size.h
	$(COMMON_HIDE) echo "#define DSM_ASM_PAGEALIGN \".align `$(LIB_GEN_OBJ)/dsm_page_size$(LIB_EXT)`\"" >> $(LIB_GEN_INC)/dsm_page_size.h
	$(COMMON_HIDE) echo "#else" >> $(LIB_GEN_INC)/dsm_page_size.h
	$(COMMON_HIDE) echo "#define DSM_ASM_PAGEALIGN \".align 16\"" >> $(LIB_GEN_INC)/dsm_page_size.h
	$(COMMON_HIDE) echo "#endif" >> $(LIB_GEN_INC)/dsm_page_size.h

$(PM2_MAK_DIR)/pm2-specific.mak: $(LIB_STAMP_FLAVOR)
	@_PM2_OUTPUT=`$(PM2_CONFIG) --includedir $(LIBRARY)`; \
	if [ -n "$$_PM2_OUTPUT" ]; then mkdir -p `dirname $@`; \
	echo "LIB_GEN_INC := $$_PM2_OUTPUT" > $@ ;\
	fi;
