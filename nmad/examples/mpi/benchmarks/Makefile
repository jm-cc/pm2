PROGS = pingpong mpi_para_ping

all: $(PROGS)

pingpong: pingpong.o structType.o vectorType.o indexType.o toolbox.o
	../mpicc $^ -o $@ -lm

mpi_para_ping: mpi_para_ping.o
	../mpicc $^ -o $@

%.o: %.c
	../mpicc -c $< -o $@


clean:
	rm -f *.o $(PROGS)
