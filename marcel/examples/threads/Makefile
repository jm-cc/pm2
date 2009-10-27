
.PHONY: clean
all:

OBJ_SOURCES = $(wildcard *.c)

PROGS_ALL := create contsw paracreate
PROGS_THREAD := sumtime semblock mutexunblock prodmat

CC := gcc

CFLAGS = -Wall -O3 -D_GNU_SOURCE=1 -g
LDLIBS = -lpthread

TYPES_ALL=nptl linuxthread unix clone pth

TYPES=nptl unix #linuxthread #unix #clone #pth

FLAVORS=marcel-mono marcel-smp marcel-numa
#FLAVORS=marcel-smp marcel-numa

DEPENDS= thread.h utils.h


PROGS=$(PROGS_ALL) $(PROGS_THREAD)
NO_GEN=$(foreach ext,-clone -unix, $(addsuffix $(ext), $(PROGS_THREAD)))

################################################"

TYPES += $(FLAVORS)
TYPES_ALL += $(FLAVORS)

define FLAVOR_template
CFLAGS_$(1) = $(shell pm2-config --flavor=$(1) --cflags)
LDLIBS_$(1) = $(shell pm2-config --flavor=$(1) --libs)
flavors: flavor-$(1)
flavor-$(1):
	$$(MAKE) -C $$(PM2_OBJROOT) FLAVOR=$(1)
endef

$(foreach flav, $(FLAVORS), \
	$(eval $(call FLAVOR_template,$(flav))))

CFLAGS_pthread     = -D_REENTRANT -DPTHREAD
LDLIBS_pthread     = -lpthread

CFLAGS_pth         = -D_REENTRANT -DPTH
LDLIBS_pth         = -lpth


CFLAGS_linuxthread = $(CFLAGS_pthread)
LDLIBS_linuxthread = $(LDLIBS_pthread)
CFLAGS_nptl        = $(CFLAGS_pthread)
LDLIBS_nptl        = $(LDLIBS_pthread)
CFLAGS_ngpt        = $(CFLAGS_pthread)
LDLIBS_ngpt        = -L/home/vdanjean/usr/ngpt/lib $(LDLIBS_pthread)

CFLAGS_clone = -DLINUX_CLONE

CFLAGS_unix = -DREGULAR_UNIX

TYPES_EXT=$(addprefix -,$(TYPES))
TYPES_EXT_ALL=$(addprefix -,$(TYPES_ALL))
GEN_PROGS=$(filter-out $(NO_GEN), \
	$(foreach ext, $(TYPES_EXT), $(addsuffix $(ext), $(PROGS))))
GEN_PROGS_ALL=$(filter-out $(NO_GEN), \
	$(foreach ext, $(TYPES_EXT_ALL), $(addsuffix $(ext), $(PROGS))))


all: $(GEN_PROGS)

clean::
	$(RM) -- *.o  $(GEN_PROGS_ALL) core*

define PROGRAM_template
$(1)-$(2): LDFLAGS += $$($(1)-ldflags) $$(LDLIBS_$(2))
$(1)-$(2): TYPE=$(2)
$(1)-$(2): $$(patsubst %.o, %-$(2).o, \
		$$(if $$($(1)-objs), $$($(1)-objs), $(1).o))
	$$(CC) $$(filter %.o, $$^) $$(LDFLAGS) -o $$@
endef
define PROGRAM_TYPES_template
$(1): $(foreach type, $(TYPES), $(1)-$(type))
endef
define OBJECT_template
$(1)-$(2).o: CFLAGS += $$($(1)-cflags) $$(CFLAGS_$(2))
$(1)-$(2).o: TYPE=$(2)
$(1)-$(2).o: $(1).c $(DEPENDS)
	$$(CC) $$(CPPFLAGS) $$(CFLAGS) -c $$< -o $$@
endef


$(foreach prog, $(PROGS), \
	$(foreach type, $(TYPES), \
		$(eval $(call PROGRAM_template,$(prog),$(type)))) \
	$(eval $(call PROGRAM_TYPES_template, $(prog))))

$(foreach file, $(OBJ_SOURCES:%.c=%), \
	$(foreach type, $(TYPES), \
		$(eval $(call OBJECT_template,$(file),$(type)))))


send:
	rsync -av . joe.labri:travail/EXPERIENCES/threads/.

receive:
	rsync -av joe.labri:travail/EXPERIENCES/threads/. .
