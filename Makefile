# $Id$
#
# the DASM macro assembler (aka small systems cross assembler)
#
# Copyright (c) 1988-2002 by Matthew Dillon.
# Copyright (c) 1995 by Olaf "Rhialto" Seibert.
# Copyright (c) 2003-2008 by Andrew Davie.
# Copyright (c) 2008 by Peter H. Froehlich.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

# Simple hack to make a beta or final distribution.
# TODO: complete the dist target, beta works already...

all:
	echo "No default target!"
	echo "Read the Makefile and decide what you want..."

# release version to use for archive name
# supply this to make when you run it as
#   RELEASE=2.20.11
# on the command line, that's the easiest
# thing to do
RELEASE=unknown-version-number
# architecture for including binaries/executables
# supply this to make when you run it as
#   BINARY=windows  or  BINARY=beos  or  ...
# on the command line, that's the easiest
# thing to do; note that this only affects
# the name of the archive, the binaries
# are always put into a trunk/bin folder
# if left empty, source distribution is assumed...
BINARY=

# binaries
BINS=bin/
# documentation
DOCS=AUTHORS ChangeLog COPYING NEWS README svn-eol-style.txt doc/* # HACKING?
# support files for various machines
MACS=machines/atari2600/* machines/channel-f/*
# source files for dasm and ftohex
SRCS=src/*.h src/*.c src/Makefile # src/TODO? src/HEADER? src/PATCHES?
# test files for dasm and ftohex
TSTS=test/*.asm test/*.bin test/*.hex test/Makefile test/atari2600/*
# other files
OTHS=Makefile

# just build, don't archive anything...

build:
	(cd src; make; cd ..)
	mkdir -p bin
	cp src/dasm bin/dasm
	cp src/ftohex bin/ftohex
	echo "Done!"

# just run all the tests
test: build
	(cd test; make; cd ..)

dist:
	echo tar zcvf dasm-$(RELEASE).tar.gz $(BINS) $(DOCS) $(MACS) $(SRCS) $(TSTS) $(OTHS)

# prepare a beta release containing source code and tests;
# machine files are included since tests may need them in
# the future; nothing else is in the archive since it is
# not intended for the public, just designated volunteers

beta:
	echo "This is an incomplete beta release of the DASM assembler." >README.BETA
	echo "The purpose is to identify regressions, nothing more." >>README.BETA
	echo "Please do *not* re-distribute this release in any form!" >>README.BETA
	echo "Please do *not* distribute binaries derived from it either!" >>README.BETA
	echo "See http://dasm-dillon.sf.net/ for details on DASM." >>README.BETA
	-tar zcvf dasm-beta-`date +%F`.tar.gz README.BETA $(SRCS) $(TSTS) $(MACS) $(OTHS)
	rm -rf README.BETA

# remove beta archives and bin/ directory created by
# regular build from this Makefile; TODO need to do
# documentation stuff as well, then we should remove
# whatever documentation we generate automatically
# as well...

clean:
	(cd src; make clean; cd ..)
	(cd test; make clean; cd ..)
	-rm -rf dasm-beta-*.tar.gz bin
