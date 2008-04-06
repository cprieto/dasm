#!/bin/bash

# $Id$
#
# Simple test script adapted from Matt Dillon's 2.16  release.
# It's harder to do this in a Makefile, so let's use a script.

for i in *.asm
do
  NAME=`basename $i .asm`
  echo "----- $NAME -----"
  ../bin/dasm $i -f1 -o$NAME.bin -DINEEPROM
  diff $NAME.bin $NAME.bin.ref
  ../bin/ftohex 1 $NAME.bin $NAME.hex
  diff $NAME.hex $NAME.hex.ref
done
