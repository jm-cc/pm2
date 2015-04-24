
NUMNODES = 4
NODES   = william0,william1
DRIVER  = ibverbs
TIMEOUT = 120

MPICHTEST_ROOT = ./mpich/test/mpi
BUILDDIR = ./build

TESTS_BASIC := \
  basic/simple basic/srtest basic/wtime basic/self basic/netmpi

TESTS_DATATYPE := \
  datatype/localpack datatype/unpack datatype/simple-pack \
  datatype/hindexed_block datatype/hindexed-zeros datatype/indexed-misc \
  datatype/contigstruct datatype/struct-pack datatype/struct-no-real-types datatype/struct-verydeep datatype/struct-zero-count \
  datatype/get-elements datatype/getpartelm datatype/simple-size-extent datatype/simple-resized datatype/tresized datatype/tresized2 \
  datatype/lots-of-types datatype/gaddress datatype/zeroblks \
  datatype/tfree
# datatype/zeroparms (1 node)
#  datatype/large-count datatype/sizedtypes

TESTS_GROUP := \
  group/groupcreate group/grouptest group/grouptest2

TESTS_COLL := \
  coll/alltoall1 coll/alltoallv0 coll/bcast2 coll/allred  \
  coll/opmax coll/opmin coll/opbor coll/opband coll/oplor coll/opland coll/opsum coll/opprod coll/opmaxloc coll/opminloc coll/reduce  \
  coll/coll9 coll/coll5 coll/coll10 coll/coll12 coll/red3

TESTS_IO := \
  io/getextent io/resized

TEST_P2P := \
  pt2pt/sendself pt2pt/large_message pt2pt/sendall pt2pt/sendrecv1 pt2pt/sendrecv2 pt2pt/sendrecv3

TESTS_ALL = $(TESTS_BASIC) $(TESTS_DATATYPE) $(TESTS_GROUP) $(TESTS_COLL) $(TESTS_IO) $(TESTS_P2P)

all: buildtests

buildtests: $(patsubst %, $(BUILDDIR)/%, $(TESTS_ALL))

$(BUILDDIR)/%: $(MPICHTEST_ROOT)/%.c
	-mkdir -p $(dir $(BUILDDIR)/$*)
	mpicc -g -I$(MPICHTEST_ROOT)/include $^ $(MPICHTEST_ROOT)/util/mtest.c $(MPICHTEST_ROOT)/basic/GetOpt.c -o $@

clean:
	-rm -r ./build

run: TESTS = $(TESTS_ALL)
run: runtests

run-basic: TESTS = $(TESTS_BASIC)
run-basic: runtests

run-datatype: TESTS = $(TESTS_DATATYPE)
run-datatype: runtests

run-group: NUMNODES = 8
run-group: TESTS = $(TESTS_GROUP)
run-group: runtests

run-coll: TESTS = $(TESTS_COLL)
run-coll: runtests

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


