

# PM2: Parallel Multithreaded Machine
# Copyright (C) 2001 "the PM2 team" (see AUTHORS file)
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

MOD_GEN_C_INC += $(MOD_GEN_INC)/dsm_page_size.h

dot_h: $(MOD_GEN_INC)/dsm_page_size.h

include $(PM2_ROOT)/make/libs-rules.mak

source/console.c-cflags = -fno-strict-aliasing

$(MOD_GEN_INC)/dsm_page_size.h:
	$(COMMON_BUILD)
	$(COMMON_MAIN) $(CC) $(CURDIR)/bin/src/dsm_page_size.c -o $(MOD_GEN_OBJ)/dsm_page_size${LIB_EXT}
	$(COMMON_HIDE) echo "#define DSM_PAGE_SIZE `$(MOD_GEN_OBJ)/dsm_page_size$(LIB_EXT)`" > $@
	$(COMMON_HIDE) echo "#ifndef IRIX_SYS" >> $@
	$(COMMON_HIDE) echo "#define DSM_ASM_PAGEALIGN \".align `$(MOD_GEN_OBJ)/dsm_page_size$(LIB_EXT)`\"" >> $@
	$(COMMON_HIDE) echo "#else" >> $@
	$(COMMON_HIDE) echo "#define DSM_ASM_PAGEALIGN \".align 16\"" >> $@
	$(COMMON_HIDE) echo "#endif" >> $@

