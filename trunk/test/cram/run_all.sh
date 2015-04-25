#!/bin/sh

if [ ! -x ../../bin/dasm ]
then
	echo "error: you must build dasm before trying to test dasm"
fi

cp ../../bin/dasm .
cram *.t
rm ./dasm
