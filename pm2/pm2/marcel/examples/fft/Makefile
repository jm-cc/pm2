include $(MARCEL_ROOT)/make/common.mak

CFLAGS = $(MAR_CFLAGS) -I. 
LDFLAGS = $(MAR_LFLAGS) $(MAR_LLIB) -lm


TARGET = fft
OBJS = fft.o marcel_np.o 
MACROS = c.m4.marcel

x = *

$(TARGET): $(OBJS) $(MAR_LIB)
	gcc $(OBJS) $(CFLAGS) -o $(TARGET) $(LDFLAGS)

.SUFFIXES:
.SUFFIXES:	.o .c .C .h .H

.H.h:
	m4 ${MACROS} $*.H > $*.h

.C.c:
	m4 $(MACROS) $*.C > $*.c

.c.o:
	gcc -c $(CFLAGS) $*.c

.C.o:
	m4 $(MACROS) $*.C > $*.c
	gcc -c $(CFLAGS) $*.c

clean:
	rm -f $(OBJS) $(TARGET) fft.c *~ #*
