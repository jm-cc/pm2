ifeq ($(MPICC),)
MPICC	:=	../mpicc -g -Wall
endif

#MPICC	:=	/opt/mpich-mx/bin/mpicc

modules 	:=	$(shell pm2-config --modules)
libs		:=	$(shell for i in ${modules} ; do pm2-config --stampfile $$i ; done)

PROGS		:=	mpi_pastix

all: $(PROGS)

%: %.o
	$(MPICC) $< -o $@ ${${@}_LIBS}

%.o: %.c %.h
	$(MPICC) -c $< -o $@

%.o: %.c
	$(MPICC) -c $< -o $@

mpi_pastix: mpi_pastix.o ${libs}

clean:
	rm -f *.o $(PROGS)

lib:
	make -C $(PM2_ROOT)/mad4 FLAVOR=mad4-mpi

clean-lib:
	make -C $(PM2_ROOT)/mad4 FLAVOR=mad4-mpi clean
