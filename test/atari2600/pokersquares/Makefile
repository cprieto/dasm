# Makefile for Poker Squares, public release version.

# Assembler to use. Tested with DASM 2.20.09.
DASM=dasm

# Define PAL=1 to build PAL version (untested on real hardware)
PAL=

# Level of verbosity for dasm (0 to 3)
VERBOSE=1

# shouldn't need to edit or redefine anything below this point.
PROJ=pokersquares
INCLUDES=pftitle.inc pokersol.inc vcs.h

ifeq ($(PAL),)
PALFLAG=
COMPARE=release
else
PALFLAG=-DPAL
COMPARE=pal
endif

all: $(PROJ).bin check

$(PROJ).bin: $(PROJ).dasm $(INCLUDES)
	$(DASM) $(PROJ).dasm $(PALFLAG) \
		-o$(PROJ).bin -f3 -v$(VERBOSE) -l$(PROJ).list -s$(PROJ).syms

# cmp is silent, add echoes to proclaim our results to all and sundry:
check: $(PROJ).bin
	@cmp $(PROJ).bin $(PROJ).$(COMPARE).bin && \
		( echo ; echo '**********'; \
		echo "* OK: $(PROJ).bin and $(PROJ).$(COMPARE).bin are identical" ; \
		echo '**********' ; echo ) || \
		( echo ; echo '!!!!!!!!!!'; \
		echo "! ERROR: $(PROJ).bin and $(PROJ).$(COMPARE).bin are different" ; \
		echo '!!!!!!!!!!' ; echo ; exit 1 )

clean:
	rm -f $(PROJ).bin $(PROJ).syms $(PROJ).list

pal: clean
	$(MAKE) PAL=1
