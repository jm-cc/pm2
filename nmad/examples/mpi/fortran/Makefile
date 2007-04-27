ifeq ($(MPIF77),)
MPIF77	:=	../mpif77
endif

PROGS		:=	fpi basics

modules 	:=	$(shell pm2-config --modules)
libs		:=	$(shell for i in ${modules} ; do pm2-config --stampfile $$i ; done)

all: $(PROGS)

%: %.o
	$(MPIF77) $< -o $@ ${${@}_LIBS}

%.o: %.f %.h
	$(MPIF77) -c $< -o $@

%.o: %.f
	$(MPIF77) -c $< -o $@

fpi: fpi.o ${libs}
basics: basics.o ${libs}

clean:
	rm -f *.o $(PROGS)

lib:
	make -C $(PM2_ROOT)/mad4 FLAVOR=mad4-mpi

clean-lib:
	make -C $(PM2_ROOT)/mad4 FLAVOR=mad4-mpi clean
