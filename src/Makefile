
#   DASM    -small systems cross assembler
#   DASM    -small systems cross assembler.  The source is compilable on
#	    Amiga's, UNIX systems, and any other system with ISO-C compiler.
#   (C)Copyright 1988-1989 Matthew Dillon, All Rights Reserved.
#
#   Modifications Copyright 1995 by Olaf Seibert. All Rights Reserved.

GOPTIM= -O#3
GWARN=	-ansi -std=c99 -pedantic -Wall -Wstrict-prototypes -Wdeclaration-after-statement -Wold-style-definition -Wmissing-declarations -Wmissing-prototypes -Wshadow -Wwrite-strings #-Wcast-qual -Wconversion -Wextra
GDB=	# -g
CC=	gcc $(GDB) $(GOPTIM) $(GWARN)
#CC=	dcc
CFLAGS= #-D_FORTIFY_SOURCE=2

OBJS= main.o ops.o globals.o exp.o symbols.o \
      mne6303.o mne6502.o mne68705.o mne6811.o mnef8.o
SRCS= main.c ops.c globals.c exp.c symbols.c \
      mne6303.c mne6502.c mne68705.c mne6811.c mnef8.c

all: dasm ftohex

dasm: $(OBJS)
	$(CC) $(OBJS) -o dasm

ftohex: ftohex.o
	$(CC) ftohex.o -o ftohex

example:
	dasm example.asm -oram:example.out -lram:example.list -f2
	ftohex 2 ram:example.out ram:example.hex

obj: $(OBJS)

$(OBJS): asm.h

clean:
	rm -rf *.o dasm ftohex
