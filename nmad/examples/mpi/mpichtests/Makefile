
NUMNODES = 4
NODES    = william0,william1
DRIVER   = ibverbs
TIMEOUT  = 300

MPICHTEST_ROOT = ./mpich/test/mpi
BUILDDIR = ./build

TESTS_BASIC := \
  basic/simple basic/srtest basic/wtime basic/self basic/netmpi \
  attr/baseattrcomm attr/baseattr2 attr/fkeyval attr/fkeyvalcomm attr/attrdeleteget attr/attrend attr/attrerr attr/attrerrcomm attr/attrorder attr/attrordercomm attr/attrt attr/attrend2 attr/keyval_double_free
# attr/attric (intercomm)


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

TESTS_P2P := \
  pt2pt/sendself pt2pt/large_message pt2pt/sendall pt2pt/sendrecv1 pt2pt/sendrecv2 pt2pt/sendrecv3 pt2pt/rcancel pt2pt/cancelrecv # pt2pt/scancel pt2pt/pscancel

TESTS_RMA := \
rma/allocmem rma/attrorderwin rma/baseattrwin rma/fkeyvalwin rma/getgroup rma/nullpscw rma/pscw_ordering rma/epochtest rma/wincall rma/window_creation rma/win_info rma/winname rma/win_flavors rma/win_shared rma/wintest rma/flush rma/locknull rma/nb_test rma/rmazero rma/mixedsync rma/selfrma rma/putfidx rma/putfence1 rma/putpscw1 rma/getfence1 rma/get_accumulate rma/compare_and_swap rma/fetch_and_op rma/fetchandadd rma/fetchandadd_am rma/accpscw1 rma/accfence1 rma/accfence2 rma/accfence2_am rma/get_acc_local rma/badrma rma/test1 rma/test1_am rma/test1_dt rma/test2_am rma/test2 rma/test3_am rma/test3 rma/test4 rma/test4_am rma/test5 rma/test5_am rma/transpose1 rma/transpose2 rma/transpose3 rma/transpose4 rma/transpose5 rma/transpose6 rma/transpose7 rma/lockcontention rma/lockcontention2 rma/lockcontention3 rma/contention_put rma/req_example rma/adlb_mimic1 rma/contention_putget rma/contig_displ rma/put_base rma/put_bottom rma/strided_acc_indexed rma/manyrma2 rma/manyrma3 rma/lockopts rma/strided_acc_subarray rma/strided_get_indexed rma/strided_getacc_indexed rma/strided_acc_onelock rma/strided_putget_indexed rma/fetchandadd_tree_am rma/fetchandadd_tree rma/linked_list_bench_lock_all rma/linked_list_bench_lock_excl rma/linked_list rma/linked_list_fop rma/linked_list_lockall rma/win_dynamic_acc  rma/strided_putget_indexed_shared rma/linked_list_bench_lock_shr rma/mutex_bench rma/strided_getacc_indexed_shared rma/win_large_shm rma/rma-contig

## tests from mpich-3.1
#  rma/ircpi <- needs user input
#  rma/reqops <- uses MPI_PROC_NULL (which is not implemented)
#  rma/rmanull <- uses MPI_PROC_NULL (which is not implemented)

TESTS_ALL = $(TESTS_BASIC) $(TESTS_DATATYPE) $(TESTS_GROUP) $(TESTS_COLL) $(TESTS_IO) $(TESTS_P2P) $(TESTS_RMA)

all: buildtests

buildtests: $(patsubst %, $(BUILDDIR)/%, $(TESTS_ALL))
$(BUILDDIR)/rma/mutex_bench: $(MPICHTEST_ROOT)/rma/mcs-mutex.c
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

run-p2p: TESTS = $(TESTS_P2P)
run-p2p: runtests

run-rma: TESTS = $(TESTS_RMA)
run-rma: runtests

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


