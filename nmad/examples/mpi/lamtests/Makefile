
NODES   = william0,william1
DRIVER  = ibverbs
TIMEOUT = 15

TESTS := \
  info/00_create info/10_set info/20_delete info/30_get info/40_getvaluelen info/50_getnkeys \
  ccl/allgather ccl/allgatherv ccl/allreduce ccl/alltoall ccl/barrier ccl/bcast ccl/bcast_struct ccl/gather ccl/op ccl/reduce ccl/reduce_scatter ccl/scatter ccl/struct_gatherv \
  comm/attr comm/commdup comm/commfree  comm/compare comm/mpisplit comm/intercomm \
  dtyp/bottom dtyp/fint2_f dtyp/fint4_f dtyp/fint8_f dtyp/freal16_f dtyp/freal4_f dtyp/freal8_f dtyp/loop dtyp/paktest dtyp/strangest1 dtyp/structsr dtyp/structsr2 dtyp/transp dtyp/transp2 dtyp/transp3 dtyp/transpa \
  env/abort env/err env/final env/initialized env/init_thread env/pcontrol env/procname env/wtime \
  group/group group/groupfree group/range \
  pt2pt/free pt2pt/getcount pt2pt/interf pt2pt/iprobe pt2pt/probe pt2pt/rsend pt2pt/send pt2pt/send2 pt2pt/sendrecv pt2pt/sendrecv_f pt2pt/seq pt2pt/ssend pt2pt/test1 pt2pt/test2 pt2pt/test3 pt2pt/testall pt2pt/testany pt2pt/testsome pt2pt/waitany pt2pt/waitnull pt2pt/waitsome pt2pt/wildcard pt2pt/sendrecv_rep

all: buildlam

buildlam:
	( cd ./lamtests ; ./configure 'CFLAGS=-g' ; make -k )

run:
	@( for t in $(TESTS); do                           \
	     echo "# ## test: $$t";                        \
	     echo "# build.";                              \
             make --quiet -C lamtests $$t;                 \
	     echo "# run.";                                \
	     mpirun --timeout $(TIMEOUT) -n 8 -nodelist $(NODES) -DNMAD_DRIVER=$(DRIVER) ./lamtests/$$t; \
	     rc=$$?;                                       \
	     if [ $$rc == 0 ]; then success="$$success $$t"; else failed="$$failed $$t"; fi; \
	     echo "# done- $$t; rc=$$rc."; echo; echo;     \
           done;                                           \
           echo; echo;                                     \
	   echo "# success: [ `echo $$success | wc -w` ] $$success" ;  \
	   echo "# failed:  [ `echo $$failed  | wc -w` ] $$failed" ;   \
	   echo                                            \
	)


