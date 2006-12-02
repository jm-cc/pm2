MPICC	:=	../mpicc
#MPICC	:=	/opt/mx/bin/mpicc

PROGS		:=	pingpong mpi_para_ping index
pingpong_LIBS	:=	-lm

all: $(PROGS)

%: %.o
	$(MPICC) $^ -o $@ ${${@}_LIBS}

%.o: %.c %.h
	../mpicc -c $< -o $@

%.o: %.c
	../mpicc -c $< -o $@

mpi_para_ping: mpi_para_ping.o
pingpong: pingpong.o structType.o vectorType.o indexType.o toolbox.o
index: index.o indexType.o toolbox.o

clean:
	rm -f *.o $(PROGS)
