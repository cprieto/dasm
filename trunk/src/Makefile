# Executables will be generated in the trunk/src/ directory (the
# "current directory" as far as *this* Makefile is concerned). If
# you want to move them somewhere, trunk/bin/ say, do so in the
# trunk/ Makefile!
#
# The entire build process needs a serious redesign, it's horrid.
# For example it should be possible to compile for release, for
# plain debugging, for coverage analysis/profiling, etc. None of
# that is possible easily right now.

GOPTI= -O2 # -O0 # -O2 for more warnings, -O0 for debugging/coverage

# Warnings are our first line of defense against many things,
# here are some of our reasons for using so many...
#
# Our baseline is C11 with a set of fairly standard warnings.
GWARN= -std=c11 -Wall -Wextra -Wpedantic
#
# We do a ton of string formatting so let's be safe about it.
GWARN+= -Wformat-nonliteral -Wformat-security -Wformat-y2k
#
# Although -Wall does some of this already, let's be doubly sure.
GWARN+= -Wuninitialized -Winit-self
#
# We don't like switch/case in the first place, but the explicit
# range is the ONE THING they have going for themselves. So we
# enforce it.
GWARN+= -Wswitch-default -Wswitch-enum
#
# Several minor issues that it's nice to have assurances about.
GWARN+= -Wshadow -Wundef \
	-Wbad-function-cast -Wcast-qual -Wcast-align \
	-Wwrite-strings -Wlogical-op \
	-Wstrict-prototypes -Wold-style-definition \
	-Wmissing-declarations -Wmissing-prototypes \
	-Wnested-externs
#
# We really want this eventually because the chaos of signed
# and unsigned types in DASM is horrifyingly bad. However, as
# of right now, it's just too much. :-/
#GWARN+=-Wconversion
#
# Seem like a good idea at first (to notice where old code and
# new code collide) but produces too many warnings and we want
# to be C11 anyway, darn it.
#GWARN+= -Wtraditional -Wtraditional-conversion
#
# We used this when we were still SPLINT addicts because that
# things has a C89 parser while claiming C99 features on their
# out-of-date website. No more!
#GWARN+= -Wdeclaration-after-statement

GDEBG= -g # --coverage # -fsanitize=address # -pg # -D_FORTIFY_SOURCE=2

CFLAGS= $(GOPTI) $(GWARN) $(GDEBG) # -D_BSD_SOURCE # -D_XOPEN_SOURCE=500
CC= gcc # clang

OBJS= main.o ops.o globals.o exp.o symbols.o \
      mne6303.o mne6502.o mne68705.o mne6811.o mnef8.o \
      errors.o util.o dalloc.o gentest.o
SRCS= main.c ops.c globals.c exp.c symbols.c \
      mne6303.c mne6502.c mne68705.c mne6811.c mnef8.c \
      errors.c util.c dalloc.c gentest.c

TEST= test_errors test_util

ALL= dasm ftohex ftobin $(TEST)

all: $(ALL)

dasm: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o dasm

symbols.o: symbols.c symbols.h
errors.o: errors.c errors.h
util.o: util.c util.h errors.h

# TODO: are these two lines really needed? GNU make
# seems to build ftohex/ftobin fine without them...
ftohex: ftohex.c
ftobin: ftobin.c

# produce executables for simple unit tests of certain
# modules
test_util: test_util.c util.o errors.o dalloc.o
test_errors: test_errors.c errors.o util.o dalloc.o

obj: $(OBJS)

$(OBJS): asm.h

clean:
	rm -rf *.o $(ALL) \
	dasm-alpha-*.tar.gz \
	*.gcda *.gcno *.gcov gmon.out coverage.info ../lcov-html

cscope:
	ls *.h *.c >cscope.files

splint:
	-splint $(SRCS) | grep "Finished"

cppcheck:
	-cppcheck --enable=all --inconclusive --std=c11 --language=c $(SRCS)

coverage:
	echo "Let's hope you have coverage data..."
	lcov --capture --directory . --output-file coverage.info
	genhtml coverage.info --output-directory ../lcov-html
	firefox-bin ../lcov-html/index.html

# target to test compiles when neither __BSD__ nor __APPLE__ are
# defined
nobsd:
	gcc -U__BSD__ -U__APPLE__ util.c -o util.o -Wall -Wextra -c

# prepare an alpha release containing just the source code,
# nothing else is in the archive since it is not intended
# for the public, just designated volunteers

alpha:
	echo "This is an incomplete alpha release of DASM source code." >README.ALPHA
	echo "The purpose is to identify build problems, nothing more." >>README.ALPHA
	echo "Please do *not* re-distribute this release in any form!" >>README.ALPHA
	echo "Please do *not* distribute binaries derived from it either!" >>README.ALPHA
	echo "See http://dasm-dillon.sf.net/ for details on DASM." >>README.ALPHA
	tar zcvf dasm-alpha-`date +%F`.tar.gz README.ALPHA Makefile *.h *.c *.x
	rm -rf README.ALPHA
