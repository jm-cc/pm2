GCC_DIR	= "$(shell pwd)/../install"
PROGS	= parallel task taskwait
CC	= $(GCC_DIR)/bin/gcc
CFLAGS	= -fopenmp -g
LDFLAGS	= -fopenmp

.PHONY:	all clean vars

all:	$(PROGS)

vars:
	echo LD_LIBRARY_PATH=\"$(GCC_DIR)/lib:\${LD_LIBRARY_PATH}\" > vars

clean:
	rm -f $(PROGS) vars core*

