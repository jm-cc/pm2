MPICC	:=	../mpicc
#MPICC	:=	/opt/mpich-mx/bin/mpicc

PROGS		:=	pingpong mpi_para_ping index index_hetero_size
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

index_hetero_size: index_hetero_size.o indexType_hetero_size.o toolbox.o 

clean:
	rm -f *.o $(PROGS)
