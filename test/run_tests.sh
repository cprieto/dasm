#!/bin/bash

# $Id$
#
# Simple test script adapted from Matt Dillon's 2.16  release.
# It's harder to do this in a Makefile, so let's use a script.

for i in *.asm
do
  NAME=`basename $i .asm`
  echo
  echo "===== Testing $NAME ====="

  # First try to assemble the test case.
  ../bin/dasm $i -f1 -o$NAME.bin -DINEEPROM
  if [ $? == 0 ]
  then
    echo "-- assembled"
  else
    echo "---------- DASM failed, skipped test"
    continue
  fi

  # Then try to produce the hex version.
  ../bin/ftohex 1 $NAME.bin $NAME.hex
  if [ $? == 0 ]
  then
    echo "-- hexed"
  else
    echo "---------- FTOHEX failed"
  fi

  # Next check if we have a reference binary.
  if ! [ -f "$NAME.bin.ref" ]
  then
    echo "---------- no reference binary, skipped test"
    continue
  fi

  # We have one so check if it matches.
  cmp -s $NAME.bin $NAME.bin.ref
  if [ $? == 0 ]
  then
    echo "-- binaries match"
  else
    echo "---------- broken binary, skipped test"
  fi

  # Next check if we have a reference hex dump.
  if ! [ -f "$NAME.hex.ref" ]
  then
    echo "---------- no reference hexdump, skipped test"
    continue
  fi

  # We have one so check if it matches.
  cmp -s $NAME.hex $NAME.hex.ref
  if [ $? == 0 ]
  then
    echo "-- hexdump matches"
  else
    echo "---------- broken hexdump"
  fi
done

../bin/dasm define_echo.asm -DEXTERNAL_D_DEFAULT -DEXTERNAL_D_VALUE=127 -MEXTERNAL_M_DEFAULT -MEXTERNAL_M_VALUE=127
