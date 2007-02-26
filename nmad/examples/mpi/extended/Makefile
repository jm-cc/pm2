ifeq ($(MPICC),)
MPICC	:=	../mpicc
endif

#MPICC	:=	/opt/mpich-mx/bin/mpicc

PROGS		:=	mpi_extended

all: $(PROGS)

%: %.o
	$(MPICC) $^ -o $@ ${${@}_LIBS}

%.o: %.c %.h
	$(MPICC) -c $< -o $@

%.o: %.c
	$(MPICC) -c $< -o $@

mpi_extended: mpi_extended.o

clean:
	rm -f *.o $(PROGS)

lib:
	make -C $(PM2_ROOT)/mad4 FLAVOR=mad4-mpi

clean-lib:
	make -C $(PM2_ROOT)/mad4 FLAVOR=mad4-mpi clean
