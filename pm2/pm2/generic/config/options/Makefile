

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

LIBS := pm2 marcel mad1 mad2 tbx ntbx common

all:

duplique_global:
	@for file in *.sh ; do \
		name=`basename $$file .sh` ; \
		echo "duplicating $$name" ; \
		for lib in $(LIBS); do \
			LIB=`echo $$lib | tr [a-z] [A-Z]`; \
			cp $$name.help \
				$$PM2_ROOT/modules/$$lib/config/options ; \
			[ -f $$name.dep ] && cp $$name.dep \
				$$PM2_ROOT/modules/$$lib/config/options ; \
			sed -e "s,LIB_,$${LIB}_,g" < $$file \
			   > $$PM2_ROOT/modules/$$lib/config/options/$$file ; \
		done ; \
	done
