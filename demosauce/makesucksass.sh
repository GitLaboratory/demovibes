#!/bin/sh
#builds the backend

echo -n "configuration: "
if test "$1" = "debug"; then
	echo -n "debug"
	FLAGS_RELEASE='-g -DDEBUG' #-pedantic
else
	echo -n "release"
	FLAGS_RELEASE='-O2'
fi

if test `uname -m` = "x86_64"; then
	echo " 64 bit "
	BASS_BIN='bass/bin_linux64'
else
	echo " 32 bit"
	BASS_BIN='bass/bin_linux'
fi

FLAGS="-Wall -Wfatal-errors $FLAGS_RELEASE" 
FLAGS_BOOST='-lboost_system -lboost_thread -lboost_filesystem -lboost_program_options -lboost_date_time'
FLAGS_BASS="-L$BASS_BIN -lbass -lbassenc -Wl,-rpath=.$BASS_BIN"
SOURCE_FILES='logror.cpp basssource.cpp basscast.cpp sockets.cpp settings.cpp demosauce.cpp'

g++ $FLAGS $FLAGS_BOOST $FLAGS_BASS -o demosauce $SOURCE_FILES

echo "done"
