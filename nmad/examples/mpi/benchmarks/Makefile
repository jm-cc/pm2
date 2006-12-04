ifeq ($(MPICC),)
MPICC	:=	../mpicc
endif

#MPICC	:=	/opt/mpich-mx/bin/mpicc

PROGS		:=	pingpong mpi_para_ping index indexType_hetero_size
pingpong_LIBS	:=	-lm

all: $(PROGS)

%: %.o
	$(MPICC) $^ -o $@ ${${@}_LIBS}

%.o: %.c %.h
	$(MPICC) -c $< -o $@

%.o: %.c
	$(MPICC) -c $< -o $@

mpi_para_ping: mpi_para_ping.o
pingpong: pingpong.o structType.o vectorType.o indexType.o toolbox.o
index: index.o indexType.o toolbox.o

indexType_hetero_size: indexType_hetero_size.o

clean:
	rm -f *.o $(PROGS)
