
# Variables communes
#---------------------------------------------------------------------
include $(PM2_ROOT)/make/common-vars.mak

SUBDIRS=sigmund supertrace xsigmund sigmundps

TARGETS_SIGMUND=sigmund
TARGETS_SUPERTRACE=supertrace
TARGETS_SIGMUNDPS=sigmundps
TARGETS_XSIGMUND=xsigmund
TARGETS_ALL=clean cleanall

all: sigmund supertrace xsigmund sigmundps


$(TARGETS_SIGMUND):
	$(COMMON_HIDE) set -e; \
	$(MAKE) -C sigmund $@

$(TARGETS_SUPERTRACE):
	$(COMMON_HIDE) set -e; \
	$(MAKE) -C supertrace $@

$(TARGETS_SIGMUNDPS):
	$(COMMON_HIDE) set -e; \
	$(MAKE) -C sigmundps $@

$(TARGETS_XSIGMUND):
	$(COMMON_HIDE) set -e; \
	$(MAKE) -C xsigmund $@

$(TARGETS_ALL):
	$(COMMON_HIDE) set -e; \
	for i in $(SUBDIRS) ; do \
		$(MAKE) -C $$i $@ ; \
	done

.PHONY: all $(TARGETS_SIGMUND) $(TARGETS_SUPERTRACE) $(TARGETS_ALL) $(TARGETS_XSIGMUND) $(TARGETS_SIGMUNDPS)
