

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

LIBS := pm2 marcel mad3 nmad tbx ntbx init leoparse profile bubblelib pioman memory
# mad1
MODULES := leonie stackalign bubbles bubblegum fut2paje memory

# Directement dans les sources:
# stackalign

all:

duplique_global:
	@copie() { : FILE LIB DESTFILE ; \
	    LIB=`echo $$2 | tr [a-z] [A-Z]`; \
	    if [ -f $$1 ]; then \
		sed -e "s,@LIB@,$${LIB},g;s,@lib@,$$2,g" \
		    < $$1 > $$3.new ; \
		cmp -s $$3.new $$3 || ( \
		    echo "updating $$lib//$$1" ;\
		    mv -f $$3.new $$3 ;\
		    false ) && rm -f $$3.new ; \
	    fi ;\
	} ;\
	send() { : FILES MODULES ; \
	    for file in $$1 ; do \
		name=`ls *$$file.sh` ; name=`basename $$name .sh` ; \
		echo "duplicating '$$name' for $$2" ; \
		for lib in $$2; do \
		    DEST=$$PM2_ROOT/modules/$$lib/config/options ;\
	    	    mkdir -p $$DEST; \
		    copie $$name.sh $$lib $$DEST/$$name.sh ;\
		    copie $$name.help $$lib $$DEST/$$name.help ;\
		    copie $$name.dep $$lib $$DEST/$$name.dep ;\
		done ; \
	    done; \
	} ;\
	append_ligne_dep() { : LIGNE LIBS ; \
	    echo "adding dep '$$1' for $$2" ; \
	    for lib in $$2; do \
	    	LIB=`echo $$lib | tr [a-z] [A-Z]`; \
		ligne=`echo "$$1"|sed -e "s,@LIB@,$${LIB},g;s,@lib@,$${lib},g"`;\
		DESTFILE=$$PM2_ROOT/modules/$$lib/config/$$lib.dep ;\
		if ! grep -sq "$$ligne" "$$DESTFILE" ; then \
		    echo "updating $$lib with $$ligne" ;\
		    echo $$ligne >> $$DESTFILE ; \
		fi ; \
	    done ; \
	} ; \
	send "debug gdb gprof opt noinline profile gcc_instrument" "$(LIBS) appli" ; \
	send "debug gdb gprof opt noinline" "$(MODULES)" ; \
	send "build_static build_dynamic build_both" "$(LIBS)" ; \
	append_ligne_dep "`cat linkmode.dep | head -1`" "$(LIBS)"
