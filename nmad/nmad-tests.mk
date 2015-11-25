
TESTS += $(patsubst $(srcdir)/%.out, %, $(wildcard $(srcdir)/*.out) )

NMAD_DRIVER = tcp
NMAD_NODES = 2
NMAD_HOSTS = localhost

TEST_TIMEOUT = 60
BENCH_TIMEOUT = 180

TARGET_TESTS = $(patsubst %, test-%, $(TESTS))

TARGET_BENCH = $(patsubst %, bench-%, $(BENCH))

tests:
	$(Q)( for t in $(TARGET_TESTS); do \
                $(MAKE) $${t} ; \
                rc=$$? ; \
                if [ $${rc} != 0 ]; then \
                  if [ "x$(TESTS_RESULTS)" != "x" -a -w "$(TESTS_RESULTS)" ]; then \
                    echo "TESTS_FAILED += $$t"  >> "$(TESTS_RESULTS)" ; \
                  fi ; \
                  echo "  [TEST]   $$t compilation FAILED" ; \
                fi ; \
              done \
            )

bench:
	$(Q)( for b in $(TARGET_BENCH); do \
                $(MAKE) $${b} ; \
                rc=$$? ; \
                if [ $${rc} != 0 ]; then \
                    echo "  [BENCH]  $$b compilation FAILED" ; \
                fi ; \
              done \
            )

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
            padico-launch $(NMAD_EXTRA_ARGS) -q --timeout $(TEST_TIMEOUT) -n $(NMAD_NODES) -nodelist "$(NMAD_HOSTS)" -DNMAD_DRIVER=$(NMAD_DRIVER) $(CURDIR)/$$t | grep -v '^#' | sort  > /tmp/result-$${testid} ; \
            rc=$$? ; \
          else \
            padico-launch $(NMAD_EXTRA_ARGS) -q --timeout $(TEST_TIMEOUT) -n $(NMAD_NODES) -nodelist "$(NMAD_HOSTS)" -DNMAD_DRIVER=$(NMAD_DRIVER) $(CURDIR)/$$t ; \
            rc=$$? ; \
          fi ; \
          if [ "x$${rc}" != "x0" ]; then \
            echo "           FAILED- rc=$${rc}" ; \
            if [ -r /tmp/result-$${testid} ]; then rm /tmp/result-$${testid}; fi ;\
            if [ "x$(TESTS_RESULTS)" != "x" -a -w "$(TESTS_RESULTS)" ]; then \
              echo "TESTS_FAILED += $*" >> "$(TESTS_RESULTS)" ; \
            fi ; \
          elif [ -r $(srcdir)/$*.out ]; then \
            echo "           checking output" ; \
            sort $(srcdir)/$*.out > /tmp/expected-$${testid}; \
            diff  --strip-trailing-cr -u /tmp/result-$${testid} /tmp/expected-$${testid} > /dev/null ; \
            rc=$$? ; \
            if [ "x$${rc}" != "x0" ]; then \
	      echo "           FAILED- wrong output" ; \
              diff  --strip-trailing-cr -u /tmp/result-$${testid} $(srcdir)/$*.out ; \
              if [ "x$(TESTS_RESULTS)" != "x" -a -w $(TESTS_RESULTS) ]; then \
                echo "TESTS_FAILED += $*" >> "$(TESTS_RESULTS)" ; \
              fi ; \
            fi ; \
          fi ; \
          if [ -r /tmp/result-$${testid} ]; then rm /tmp/result-$${testid}; fi ;\
          if [ -r /tmp/expected-$${testid} ]; then rm /tmp/expected-$${testid}; fi ;\
          if [ "x$${rc}" = "x0" -a "x$(TESTS_RESULTS)" != "x" -a -w "$(TESTS_RESULTS)" ]; then \
             echo "TESTS_SUCCESS += $*" >> "$(TESTS_RESULTS)" ; \
             echo "           SUCCESS." ; \
          fi ; \
        )

$(TARGET_BENCH): bench-%: %
	@echo "  [BENCH]  $*"
	@echo "           running $(NMAD_NODES) nodes on hosts: $(NMAD_HOSTS); network: $(NMAD_DRIVER)"
	@( if [ -r /tmp/bench-$${USER}-$$$$ ]; then \
             rm /tmp/bench-$${USER}-$$$$; \
           fi ; \
           if [ "x$(BENCH_RESULTS)" = "x" ]; then \
             out=/dev/null ;\
           else \
             out=$(BENCH_RESULTS) ; \
           fi ; \
           echo "# starting bench $*" >> $${out} ; \
           padico-launch $(NMAD_EXTRA_ARGS) -q --timeout $(BENCH_TIMEOUT) -n $(NMAD_NODES) -nodelist "$(NMAD_HOSTS)" -DNMAD_DRIVER=$(NMAD_DRIVER) ./$* | tee /tmp/bench-$${USER}-$$$$ ; \
           cat /tmp/bench-$${USER}-$$$$ >> $${out} ; \
           rm /tmp/bench-$${USER}-$$$$ )
	@echo "           done."

