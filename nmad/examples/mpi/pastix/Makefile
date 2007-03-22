ifeq ($(MPICC),)
MPICC	:=	../mpicc -g -Wall
endif

#MPICC	:=	/opt/mpich-mx/bin/mpicc

PROGS		:=	mpi_pastix

all: $(PROGS)

%: %.o
	$(MPICC) $^ -o $@ ${${@}_LIBS}

%.o: %.c %.h
	$(MPICC) -c $< -o $@

%.o: %.c
	$(MPICC) -c $< -o $@

mpi_pastix: mpi_pastix.o

clean:
	rm -f *.o $(PROGS)

lib:
	make -C $(PM2_ROOT)/mad4 FLAVOR=mad4-mpi

clean-lib:
	make -C $(PM2_ROOT)/mad4 FLAVOR=mad4-mpi clean
