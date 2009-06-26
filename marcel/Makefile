

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

# Numéro de version majeur (pour une bibliothèque dynamique)
# 0 nécessaire pour la libpthread
#---------------------------------------------------------------------
LIB_SO_MAJ=0

# Inclusion des makefiles generiques
#---------------------------------------------------------------------
include $(PM2_ROOT)/make/libs-vars.mak
DO_NOT_GENERATE_MAK_FILES += _clean_autogen_

# TODO LIBNAME devrait provenir des fichiers de la flavor
#---------------------------------------------------------------------
ifeq ($(LIBPTHREAD),true)
LIBNAME := pthread
LINK_LDFLAGS += -Wl,--rpath="$(shell pm2-config --flavor=$(FLAVOR) --libdir puk)"
endif

SCHED_NAME=marcel

MOD_HSPLITS_COMMON:=scripts/splitall_section_def.h
MOD_HSPLITS_SOURCES:=$(shell echo `pwd` 1>&2 ; cd include ; \
	grep -l '^\#section' *.h sys/*.h)

#MOD_HSPLITS_GENDIR ?= $(MOD_GEN_INC)
MOD_HSPLITS_GENDIR = $(PM2_ROOT)/marcel/autogen-include

MOD_SRC += source/scheduler-$(SCHED_NAME)
MOD_SRC += source/arch-$(MOD_ARCH)
MOD_SRC += source/arch-generic

$(eval $(call MOD_HSPLITS_ADD_MASTER,,,))
#$(warning $(call MOD_HSPLITS_ADD_MASTER,,,))

define ADD_SUBDIR # subdir SUBNAME base
 $$(eval $$(call MOD_HSPLITS_ADD_MASTER,-$(1),_$(2),$(1)))
 MOD_HSPLITS_PARTS_$(2) += MOD_HSPLITS_$(2)_section MOD_HSPLITS_$(2)_nosection
 MOD_HSPLITS_$(2)_section_COMMON:=scripts/splitall_section_def.h
 MOD_HSPLITS_$(2)_section_SRCDIR:=include/$(1)
 MOD_HSPLITS_$(2)_section_GENSUBDIR:=$(1)
 MOD_HSPLITS_$(2)_section_DEFAULT_SECTION:=null
 MOD_HSPLITS_$(2)_SOURCES:=$$(patsubst include/$(1)/%,%,$$(wildcard include/$(1)/*.h))
 MOD_HSPLITS_$(2)_section_SOURCES=$$(shell cd include/$(1); grep </dev/null -l '^\#section' $$(MOD_HSPLITS_$(2)_SOURCES))
 MOD_HSPLITS_$(2)_nosection_COMMON:=scripts/splitall_nosection_def.h
 MOD_HSPLITS_$(2)_nosection_SRCDIR:=include/$(1)
 MOD_HSPLITS_$(2)_nosection_GENSUBDIR:=$(1)
 MOD_HSPLITS_$(2)_nosection_DEFAULT_SECTION:=all
 MOD_HSPLITS_$(2)_nosection_SOURCES=$$(filter-out $$(MOD_HSPLITS_$(2)_section_SOURCES),$$(MOD_HSPLITS_$(2)_SOURCES))
endef

#ifeq ($(MOD_ARCH),)
#$(warning MOD_ARCH not defined)
#endif
#ifeq ($(SCHED_NAME),)
#$(warning SCHED_NAME not defined)
#endif

# Décommenter lorsque make 3.81 sortira
#ifeq ($(findstring dot_h_all, $(MAKECMDGOALS)),dot_h_all)
#$(foreach ASM,$(subst include/,,$(wildcard include/asm-*)),\
#	$(eval $(call ADD_SUBDIR,$(ASM),ARCH-$(ASM))))
#else
$(eval $(call ADD_SUBDIR,asm-generic,ARCH_GENERIC))
$(if $(MOD_ARCH),$(eval $(call ADD_SUBDIR,asm-$(MOD_ARCH),ARCH)))
#endif

$(if $(SCHED_NAME),$(eval $(call ADD_SUBDIR,scheduler-$(SCHED_NAME),SCHED)))

.PHONY: symlinks

# Make sure `marcel_abi.h' is a prerequisite for all `.o' files.
MOD_GEN_C_INC += $(MOD_GEN_INC)/marcel_abi.h

# Building `marcel_abi.h'.  Conceptually, `marcel_abi.h' depends on all the
# generated `.h' files, and it turns out that they are generated at the same
# time as $(MOD_HSPLITS_MAKEFILES) by `pm2-split-h-file'.
$(MOD_GEN_INC)/marcel_abi.h: $(MOD_HSPLITS_MAKEFILES)
	$(COMMON_BUILD)
	@: > "$@"
	$(info computing header hash for flavor `$(FLAVOR)')
	@( hash=`$(PM2_ROOT)/marcel/scripts/cpp-hash.sh "$(FLAVOR)"	\
	             $(PM2_ROOT)/marcel/include/marcel.h` ;		\
	echo "/* Automatically generated, do not edit.  */" ;		\
	echo "#ifndef MARCEL_HEADER_HASH" ;				\
	echo "/** \\brief A hash of Marcel's headers for flavor" ;	\
	echo "    \`$(FLAVOR)'.  */" ;					\
	echo "# define MARCEL_HEADER_HASH \"$$hash\"" ;			\
	echo "#endif" ) > "$@"


# The `marcel_bubble_sched_lookup.h' file is included solely by
# `marcel_bubble_sched.c'.
$(MOD_GEN_OBJ)/marcel_bubble_sched.o $(MOD_GEN_OBJ)/marcel_bubble_sched.pic: $(MOD_GEN_INC)/marcel_bubble_sched_lookup.h
$(MOD_GEN_CPP)/marcel_bubble_sched.i: $(MOD_GEN_INC)/marcel_bubble_sched_lookup.h

$(MOD_GEN_INC)/marcel_bubble_sched_lookup.h: $(wildcard source/scheduler-marcel/marcel_bubble_*.c)
	$(COMMON_BUILD)
	@( schedulers=`cat $^							\
	 | sed -n -e 's/.*MARCEL_DEFINE_BUBBLE_SCHEDULER_CLASS *( *\([^,]*\).*/\1/p' \
	 | sort | uniq` ;							\
	echo "/* Automatically generated, do not edit.  */" ;			\
	echo "struct ma_bubble_sched_desc {" ;					\
	echo "  const char *name; const marcel_bubble_sched_class_t *klass;" ;	\
	echo "};" ;								\
	echo "static const struct ma_bubble_sched_desc" ;			\
	echo "ma_bubble_schedulers[] = {" ;					\
	for sched in $$schedulers ;						\
	do									\
	  echo "  { \"$$sched\", &marcel_bubble_$${sched}_sched_class }," ;		\
	done ;									\
	echo "  { NULL, NULL }" ;						\
	echo "};" ) > "$@"

dot_h: symlinks $(MOD_GEN_INC)/marcel_abi.h

symlinks: $(MOD_GEN_INC)/asm $(MOD_GEN_INC)/scheduler

$(MOD_GEN_INC)/asm:
	$(COMMON_BUILD)
	-$(COMMON_MAIN) ln -sf $(MOD_HSPLITS_GENDIR)/asm-$(MOD_ARCH) $@
$(MOD_GEN_INC)/scheduler:
	$(COMMON_BUILD)
	-$(COMMON_MAIN) ln -sf $(MOD_HSPLITS_GENDIR)/scheduler-$(SCHED_NAME) $@

.PHONY: clean_autogen
clean: clean_autogen

clean_autogen:
	$(COMMON_CLEAN) $(RM) -r autogen-include

#$(foreach rep,$(patsubst include/%,%, \
#		$(wildcard include/asm-* include/scheduler-*)), \
#	$(eval $(call ADD_SUBDIR,$(rep),$(rep))))

#sys/marcel_sched.h scheduler/sched_marcel.h marcel_alias.h \
#	marcel_threads.h

# Inclusion des makefiles generiques
#---------------------------------------------------------------------
include $(PM2_ROOT)/make/libs-rules.mak

#---------------------------------------------------------------------
#---------------------------------------------------------------------
# Règles locales pour maintenir à jour les fichiers provenant
# des sources du noyaux linux

# Récupération du nom des fichiers originaux
KFILES=$(shell find include source -iname "linux_*.[ch]" | \
	sed	-e 's,^.*/old/.*$$,,' \
		-e 's,\(asm[^/]*/\)linux_,\1,' \
		-e 's,include/\(asm-.*\)/linux_,include/\1/,' \
		-e 's,include/.*linux_,include/linux/,' \
		-e 's,source/.*linux_,kernel/,' \
		-e 's,lwp,cpu,' \
		-e 's,kernel/notifier.c,kernel/sys.c,')

# Répertoire contenant (ou lien vers) les sources du noyau 
KDIR= linux_tree

# Répertoire contenant la copie des fichiers du noyau que l'on a adapté à marcel
COPYDIR=linux_archive

.PHONY: linux
linux:
	$(COMMON_MAIN)set -e ; LST="" ; \
		for f in $(KFILES) ; do \
			if [ ! -f $(COPYDIR)/$$f ]; then \
				echo "Copying new file $$f"; \
				mkdir -p $$(dirname $(COPYDIR)/$$f); \
				bk get $(KDIR)/$$f 2>/dev/null || true ; \
				if [ ! -f $(KDIR)/$$f ]; then \
					echo "No $$f in $(KDIR) !!!" ;\
				else \
					cp -a $(KDIR)/$$f $(COPYDIR)/$$f ; \
				fi ; \
			elif cmp --quiet $(KDIR)/$$f $(COPYDIR)/$$f; then \
				: ;\
			else \
				echo "$$f has been updated in kernel sources" ; \
			fi ; \
		done
	$(COMMON_MAIN)set -e ; LST=""; KFILES="$(KFILES)" ; \
		for f in $$(find $(COPYDIR) -type d -name SCCS -prune -o -type f -printf "%P\n") ; do \
			case " $$KFILES " in \
			*\ $$f\ *) ;; \
			*) echo "$$f: present in archive but not in sources (to remove ?)" ; \
			   LST="$$LST $(COPYDIR)/$$f" ;; \
			esac ; \
		done ; \
		if [ ! -z "$$LST" ]; then \
			echo "You should verify and execute : " ; \
			echo "rm -v $$LST" ; \
		fi

source/marcel_init.c-cflags=-fno-omit-frame-pointer
