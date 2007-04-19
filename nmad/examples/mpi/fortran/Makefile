ifeq ($(MPIF77),)
MPIF77	:=	../mpif77
endif

PROGS		:=	fpi

all: $(PROGS)

%: %.o
	$(MPIF77) $^ -o $@ ${${@}_LIBS}

%.o: %.f %.h
	$(MPIF77) -c $< -o $@

%.o: %.f
	$(MPIF77) -c $< -o $@

fpi: fpi.o

clean:
	rm -f *.o $(PROGS)

lib:
	make -C $(PM2_ROOT)/mad4 FLAVOR=mad4-mpi

clean-lib:
	make -C $(PM2_ROOT)/mad4 FLAVOR=mad4-mpi clean
