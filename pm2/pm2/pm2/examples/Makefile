
SUBDIR := acteurs  gimp-plugin  migration  simple  tree-rpc \
	X     dsm      isomalloc    rpc        tree

examples:

include $(PM2_ROOT)/make/common-rules.mak

.PHONY: all clean distclean examples
all clean examples:
	$(COMMON_HIDE) set -e;\
	for rep in $(SUBDIR); do \
		$(MAKE) -C $$rep $@; \
	done
all:
clean:
#distclean:
examples:

SUBDIRSLASH := $(foreach DIR, $(SUBDIR), $(DIR)/)
.PHONY: $(SUBDIR) $(SUBDIRSLASH)
$(SUBDIR) $(SUBDIRSLASH):
	$(COMMON_HIDE) $(MAKE) -C $@ examples

.PHONY: help bannerhelpapps targethelpapps
help: globalhelp

bannerhelp: bannerhelpapps

bannerhelpapps:
	@echo "This is PM2 Makefile for examples repertory"

targethelp: targethelpapps

DIRLIST:=$(foreach DIR,$(SUBDIR),$(DIR))
targethelpapps:
	@echo "  all|examples: build the examples in each directory"
	@echo "  help: this help"
	@echo "  clean: clean examples source tree for current flavor"
#	@echo "  distclean: clean examples source tree for all flavors"
	@echo
	@echo "Repertories with examples:"
	@echo "  $(DIRLIST)"


