
NUMNODES = 8
NODES   = william0,william1
DRIVER  = ibverbs
TIMEOUT = 120

MPICHTEST_ROOT = ./mpich/test/mpi
BUILDDIR = ./build

TESTS_DATATYPE := \
  datatype/localpack datatype/unpack datatype/simple-pack datatype/simple-resized \
  datatype/hindexed_block datatype/hindexed-zeros datatype/indexed-misc \
  datatype/contigstruct datatype/struct-pack datatype/struct-verydeep datatype/struct-zero-count \
  datatype/simple-resized datatype/tresized datatype/tresized2 \
  datatype/lots-of-types datatype/gaddress datatype/zeroblks datatype/zeroparms
#  datatype/large-count datatype/sizedtypes

TESTS_GROUP := \
  group/groupcreate group/grouptest group/grouptest2

TESTS_COLL := \
  coll/alltoall1 coll/alltoallv0 coll/bcast2 coll/allred  \
  coll/opbor coll/opband coll/oplor coll/opland coll/opsum coll/reduce coll/coll5

TESTS_IO := \
  io/getextent io/resized

TEST_P2P := \
  pt2pt/sendself pt2pt/large_message pt2pt/sendall pt2pt/sendrecv1 pt2pt/sendrecv2 pt2pt/sendrecv3

TESTS_ALL = $(TESTS_DATATYPE) $(TESTS_GROUP) $(TESTS_COLL) $(TESTS_IO) $(TESTS_P2P)

all: buildtests

buildtests: $(patsubst %, $(BUILDDIR)/%, $(TESTS_ALL))

$(BUILDDIR)/%: $(MPICHTEST_ROOT)/%.c
	-mkdir -p $(dir $(BUILDDIR)/$*)
	mpicc -g -I$(MPICHTEST_ROOT)/include $^ $(MPICHTEST_ROOT)/util/mtest.c -o $@

clean:
	-rm -r ./build

run: TESTS = $(TESTS_ALL)
run: runtests

run-datatype: TESTS = $(TESTS_DATATYPE)
run-datatype: runtests

runtests: buildtests
	@( for t in $(TESTS); do                           \
	     echo "# ## test: $$t";                        \
	     mpirun --timeout $(TIMEOUT) -n $(NUMNODES) -nodelist $(NODES) -DNMAD_DRIVER=$(DRIVER) $(BUILDDIR)/$$t; \
	     rc=$$?;                                       \
	     if [ $$rc == 0 ]; then success="$$success $$t"; else failed="$$failed $$t"; fi; \
	     echo "# done- $$t; rc=$$rc."; echo; echo;     \
           done;                                           \
           echo; echo;                                     \
	   echo "# success: [ `echo $$success | wc -w` ] $$success" ;  \
	   echo "# failed:  [ `echo $$failed  | wc -w` ] $$failed" ;   \
	   echo                                            \
	)


