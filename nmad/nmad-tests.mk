
TESTS += $(patsubst $(srcdir)/%.out, %, $(wildcard $(srcdir)/*.out) )

NMAD_DRIVER = tcp
NMAD_NODES = 2
NMAD_HOSTS = localhost

TARGET_TESTS = $(patsubst %, test-%, $(TESTS))


tests: $(TARGET_TESTS)


$(TARGET_TESTS): test-%: %
	@echo "  [TEST]   $*"
	@echo "           running $(NMAD_NODES) nodes on hosts: $(NMAD_HOSTS); network: $(NMAD_DRIVER)"
	@( \
          t="$*"; \
          testid="$${USER}-$$$$" ; \
          if [ -r $(srcdir)/$*.out ]; then \
            padico-launch -q -n $(NMAD_NODES) -nodelist $(NMAD_HOSTS) -DNMAD_DRIVER=$(NMAD_DRIVER) ./$$t | cat > /tmp/result-$${testid} ; \
            rc=$$? ; \
          else \
            padico-launch -q -n $(NMAD_NODES) -nodelist $(NMAD_HOSTS) -DNMAD_DRIVER=$(NMAD_DRIVER) ./$$t ; \
            rc=$$? ; \
          fi ; \
          if [ "x$${rc}" != "x0" ]; then \
            echo "           FAILED- rc=$rc" ; \
            if [ -r /tmp/result-$${testid} ]; then rm /tmp/result-$${testid}; fi ;\
            exit $${rc}; \
          fi ; \
          if [ -r $(srcdir)/$*.out ]; then \
            echo "           checking output" ; \
            diff  --strip-trailing-cr -u /tmp/result-$${testid} $(srcdir)/$*.out > /dev/null ; \
            rc=$$? ; \
            if [ "x$${rc}" != "x0" ]; then \
	      echo "           FAILED- wrong output" ; \
              diff  --strip-trailing-cr -u /tmp/result-$${testid} $(srcdir)/$*.out ; \
	      rm /tmp/result-$${testid}; \
              exit $${rc}; \
            fi ; \
	    rm /tmp/result-$${testid}; \
          fi ; \
          exit $$rc \
        )
	@echo "           SUCCESS."

