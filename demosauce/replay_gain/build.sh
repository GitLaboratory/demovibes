#!/bin/sh

SOURCE_FILES='gain_analysis.c replay_gain.c'
OUTPUT='libreplay_gain.a'

echo -n "libreplay_gain configuration: "
if test "$1" = "debug"; then
	echo "debug"
	FLAGS_RELEASE='-g -DDEBUG' #-pedantic
else
	echo "release"
	FLAGS_RELEASE='-s -O3'
fi

FLAGS="-Wall -Wfatal-errors -c -std=gnu99 $FLAGS_RELEASE" 
gcc $FLAGS $SOURCE_FILES

if test $? -eq 0; then
	rm -f $OUTPUT
	ar rs $OUTPUT *.o
	rm *.o
	echo "done"
fi