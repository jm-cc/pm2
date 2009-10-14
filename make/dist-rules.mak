# -*- mode: makefile;-*-

# PM2: Parallel Multithreaded Machine
# Copyright (C) 2009 "the PM2 team" (see AUTHORS file)
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

# Rules to make a distribution, i.e., a tarball with just what's
# needed for a given PM2 module.


# Files and directories that must always be part of a distribution
# tarball.
PM2_COMPULSORY_DEPENDENCIES :=				\
  bin make modules common appli doc ezflavor		\
  configure configure.ac build-aux			\
  GNUmakefile.in aclocal.m4				\
  ARCHITECTURES AUTHORS COPYING FILES README

# The repository URL.  In the git-svn case, we can't simply use `git
# clone $(PM2_ROOT) foo' to "export" the source tree since we want SVN
# externals to be handled as well.
vcs-url    :=							\
  $(shell if test -d "$(PM2_ROOT)/.git";			\
          then git svn info | grep '^URL' | cut -f 2- -d ' ';	\
	  else echo "$(PM2_ROOT)"; fi)

vcs-export := svn export

# make-module-dist-rules MODULE TARBALL-NAME DEPENDENCIES
#
# Make a `dist-MODULE' rule that creates a `TARBALL-NAME.tar.gz' file
# containing all of MODULE and DEPENDENCIES, with the addition of all
# the PM2 compulsory dependencies (build system, etc.).
define make-module-dist-rules

dist-$(1):
	$(vcs-export) "$(vcs-url)" "$(2)"
	( cd "$(2)" ;							\
	  for mf in `grep '[a-zA-Z0-9/]\+/GNUmakefile' configure.ac` ;	\
	  do								\
	    if ! ( echo "$(1) $(3)" |					\
	           grep -q "`echo $$$$mf | cut -d / -f 1`" ) ;		\
	    then							\
	        echo "will not instantiate makefile \`$$$$mf'" ;	\
	        sed -i "configure.ac" -e"s,$$$$mf,,g" ;			\
	    else							\
	        echo "keeping makefile \`$$$$mf'" ;			\
	    fi ;							\
	  done )
	( cd "$(2)" ; ./autogen.sh )
	GZIP=--best tar czf "$(2).tar.gz"			\
	  $(foreach subdir,$(PM2_COMPULSORY_DEPENDENCIES) $(1) $(3),"$(2)/$(subdir)")
	rm -rf "$(2)"

.PHONY: dist-$(1)

endef

# make-all-dist-rules MODULE...
#
# Produce a `dist-MODULE' rule for each MODULE, along with a `dist'
# rule that depends on all of them.
define make-all-dist-rules

$(foreach module,$(1),								\
  $(call make-module-dist-rules,$(module),$(module)-$($(module)_VERSION),	\
                                $($(module)_DEPENDENCIES)))

dist: $(foreach module,$(1),dist-$(module))

.PHONY: dist

endef


# Create the `dist' rule, assuming $(DIST_MODULES) is defined.
$(eval $(call make-all-dist-rules,$(DIST_MODULES)))
