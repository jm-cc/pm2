
TESTS += $(patsubst $(srcdir)/%.out, %, $(wildcard $(srcdir)/*.out) )

NMAD_DRIVER = tcp
NMAD_NODES = 2
NMAD_HOSTS = localhost

TEST_FILTER = cat

TEST_TIMEOUT = 60

TARGET_TESTS = $(patsubst %, test-%, $(TESTS))

TARGET_BENCH = $(patsubst %, bench-%, $(BENCH))

tests: $(TARGET_TESTS)

bench: $(TARGET_BENCH)

$(TARGET_TESTS): test-%: %
	@echo "  [TEST]   $*"
	@echo "           running $(NMAD_NODES) nodes on hosts: $(NMAD_HOSTS); network: $(NMAD_DRIVER)"
	@( \
          t="$*"; \
          testid="$${USER}-$$$$" ; \
          if [ "x$(TESTS_RESULTS)" != "x" -a -w "$(TESTS_RESULTS)" ]; then \
             echo "# running test $* in $(srcdir)" >> "$(TESTS_RESULTS)" ; \
          fi ; \
          if [ -r $(srcdir)/$*.out ]; then \
            padico-launch -q --timeout $(TEST_TIMEOUT) -n $(NMAD_NODES) -nodelist "$(NMAD_HOSTS)" -DNMAD_DRIVER=$(NMAD_DRIVER) ./$$t | $(TEST_FILTER) > /tmp/result-$${testid} ; \
            rc=$$? ; \
          else \
            padico-launch -q --timeout $(TEST_TIMEOUT) -n $(NMAD_NODES) -nodelist "$(NMAD_HOSTS)" -DNMAD_DRIVER=$(NMAD_DRIVER) ./$$t ; \
            rc=$$? ; \
          fi ; \
          if [ "x$${rc}" != "x0" ]; then \
            echo "           FAILED- rc=$rc" ; \
            if [ -r /tmp/result-$${testid} ]; then rm /tmp/result-$${testid}; fi ;\
            if [ "x$(TESTS_RESULTS)" != "x" -a -w "$(TESTS_RESULTS)" ]; then \
              echo "TESTS_FAILED += $*" >> "$(TESTS_RESULTS)" ; \
            fi ; \
          fi ; \
          if [ -r $(srcdir)/$*.out ]; then \
            echo "           checking output" ; \
            diff  --strip-trailing-cr -u /tmp/result-$${testid} $(srcdir)/$*.out > /dev/null ; \
            rc=$$? ; \
            if [ "x$${rc}" != "x0" ]; then \
	      echo "           FAILED- wrong output" ; \
              diff  --strip-trailing-cr -u /tmp/result-$${testid} $(srcdir)/$*.out ; \
	      rm /tmp/result-$${testid}; \
              if [ "x$(TESTS_RESULTS)" != "x" -a -w $(TESTS_RESULTS) ]; then \
                echo "TESTS_FAILED += $*" >> "$(TESTS_RESULTS)" ; \
              fi ; \
            fi ; \
	    rm /tmp/result-$${testid}; \
          fi ; \
          if [ "x$${rc}" = "x0" -a "x$(TESTS_RESULTS)" != "x" -a -w "$(TESTS_RESULTS)" ]; then \
             echo "TESTS_SUCCESS += $*" >> "$(TESTS_RESULTS)" ; \
             echo "           SUCCESS." ; \
          fi ; \
        )

$(TARGET_BENCH): bench-%: %
	@echo "  [BENCH]  $*"
	@echo "           running $(NMAD_NODES) nodes on hosts: $(NMAD_HOSTS); network: $(NMAD_DRIVER)"
	@padico-launch -q --timeout $(TEST_TIMEOUT) -n $(NMAD_NODES) -nodelist "$(NMAD_HOSTS)" -DNMAD_DRIVER=$(NMAD_DRIVER) ./$* 
	@echo "           done."

